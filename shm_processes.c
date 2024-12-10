#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>

#define NUM_ITERATIONS 25

void DearOldDad(sem_t *mutex, int *BankAccount);
void PoorStudent(sem_t *mutex, int *BankAccount, int ID);
void LovableMom(sem_t *mutex, int *BankAccount);

int  main(int  argc, char *argv[])
{
     if (argc != 3) {
        printf("Usage: %s <Number of Parents> <Number of Children>\n", argv[0]);
        exit(1);
    }
     int numParents = atoi(argv[1]);
     int numPoorStudents = atoi(argv[2]);
     int    ShmID;
     int    *ShmPTR;
     pid_t  pid;

     //creates shared memory for bank account
     ShmID = shmget(IPC_PRIVATE, 2*sizeof(int), IPC_CREAT | 0666);
     if (ShmID < 0) {
          printf("*** shmget error (server) ***\n");
          exit(1);
     }

     ShmPTR = (int *) shmat(ShmID, NULL, 0);
     if (ShmPTR == (int *)-1) {
          printf("*** shmat error (server) ***\n");
          exit(1);
     }

     ShmPTR[0] = 0; //BankAccount

     //creates semaphores
     sem_t *mutex = sem_open("/mutex", O_CREAT, 0644, 1);
     if(mutex == SEM_FAILED){
          perror("sem_open");
          exit(1);
     }
     
     //fork Dear Old Dad process
     pid = fork();
     if(pid < 0){
          printf("*** for error (Dad) ***\n");
          exit(1);
     }
     else if(pid == 0){
          DearOldDad(mutex, ShmPTR);
          exit(0);
     } 


     //fork the poor Student processes
     for(int i = 0; i < numPoorStudents; i++){
          pid = fork();
          if (pid < 0){
               printf("*** fork error (Poor Student) ***\n");
               exit(1);
          }
          else if(pid == 0){
               PoorStudent(mutex, ShmPTR, i + 1);
               exit(0);
          } 
     }

     //fork the Lovable Mom process
     if(numParents >= 1){
          pid = fork();
          if (pid < 0){
               printf("*** fork error (Mom) ***\n");
               exit(1);
          }
          else if(pid == 0){
               LovableMom(mutex, ShmPTR);
               exit(0);
          } 
     }
     //wait for child to finish processing
     while (wait(NULL)>0);
     shmdt((void *) ShmPTR);
     shmctl(ShmID, IPC_RMID, NULL);
     sem_close(mutex);
     sem_unlink("/mutex");

     printf("All process have completed.\n");
     return 0;
}

void LovableMom(sem_t  *mutex, int *BankAccount){
     srand(time(NULL + getpid()));
     for(int i = 0; i < NUM_ITERATIONS; i++){
          sleep(rand() % 11); //sleep for 0-10 secs
          printf("Lovable Mom: Attempting to Check Balance\n");

          sem_wait(mutex);//lock semaphore
          int localBalance = *BankAccount; // copy bank account to local variable

          if(localBalance <= 100){
               int deposit = rand() % 126; //random deposit amount thats within 0-125
               localBalance += deposit; //updates localBalance with deposit
               printf("Lovable Mom: Deposits $%d / Balance = $%d\n", deposit, localBalance);
               *BankAccount = localBalance; //updates the shared bank account
          }
          sem_post(mutex);//unlock semaphore
     }
}

void DearOldDad(sem_t *mutex, int *BankAccount){
     srand(time(NULL) + getpid());
     for (int i = 0; i < NUM_ITERATIONS; i++){
          sleep(rand() % 6); //Sleep for 0-5 seconds
          printf("Dear Old Dad: Attempting to Check Balance\n");
     
          sem_wait(mutex);//lock semaphore
          int account = *BankAccount; // copy bank account to local variable

          if(account < 100){
               int depositAmount = rand() % 101; // random deposit amount between 0-100
               if (depositAmount %2 == 0) { //even number
                    account += depositAmount; //deposit the money
                    printf("Dear Old Dad: Deposits $%d / Balance = $%d\n", depositAmount, account);
               }
               else{
                    printf("Dear Old Dad: Doesn't have any money to give\n");
               }
          }
          else{
               printf("Dear Old Dad: Thinks Student has enough Cash ($%d)\n", account);
          }

          *BankAccount = account; // updates the shared BankAccount
          sem_post(mutex); //Unlock the semaphore 
     }
}

void PoorStudent(sem_t *mutex, int *BankAccount, int ID){
     srand(time(NULL) + getpid());
     for(int i = 0; i < NUM_ITERATIONS; i++){
          sleep(rand() % 6);//sleep for 0-5 secs
     
          sem_wait(mutex);//lock semaphore
          int account = *BankAccount; // copy bank account to local variable

          printf("Poor Student %d: Attempting to Check Balance\n", ID);
          int need = rand() % 51; //random withdrawal amount (0-50)
          if(need % 2 == 0){
               printf("Poor Student %d needs $%d\n", ID, need);

               if(need <= account){
                    account -= need;//withdrawal
                    printf("Poor Student %d: Withdraws $%d / Balance = $%d\n", ID, need, account);
               }
               else{
                    printf("Poor Student %d: Not Enough Cash ($%d)\n", ID, account);
               }
          }
          else{
               printf("Poor Student %d: Last Checking Balance = $%d\n", ID, account); //if random number is odd, only check the balance
          }

          *BankAccount = account; //update shared BankAccount
          sem_post(mutex); //unlock the semaphore
     }
     printf("Poor Student %d is about to exit\n", ID);
}