
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/stat.h> 


#define INITIAL_BALANCE 100
#define MAX_CHILDREN 10

void DearOldDad(sem_t *, sem_t *, int *);
void PoorStudent(sem_t *, sem_t *, int *);
void LovableMom(sem_t *, sem_t *, int *);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <Number of Parents> <Number of Children>\n", argv[0]);
        exit(1);
    }

    int numParents = atoi(argv[1]);
    int numChildren = atoi(argv[2]);

    if (numParents < 1 || numChildren < 1 || numChildren > MAX_CHILDREN) {
        printf("Invalid input. Number of Parents >= 1, Number of Children between 1 and %d.\n", MAX_CHILDREN);
        exit(1);
    }

    int fd, *BankAccount;
    sem_t *mutex, *turn;

    // Open a memory-mapped file for the shared BankAccount
    fd = open("bankaccount.txt", O_RDWR | O_CREAT, S_IRWXU);
    int initial_balance = INITIAL_BALANCE;
    write(fd, &initial_balance, sizeof(int));
    BankAccount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    // Initialize semaphores
    mutex = sem_open("mutex", O_CREAT, 0644, 1);
    turn = sem_open("turn", O_CREAT, 0644, 0);

    // Fork additional parent processes
    for (int i = 1; i < numParents; i++) {
        if (fork() == 0) {
            LovableMom(mutex, turn, BankAccount);
            exit(0);
        }
    }

    // Fork child processes
    for (int i = 0; i < numChildren; i++) {
        if (fork() == 0) {
            PoorStudent(mutex, turn, BankAccount);
            exit(0);
        }
    }

    // Main parent process (Dear Old Dad)
    DearOldDad(mutex, turn, BankAccount);

    // Wait for all children to finish
    while (wait(NULL) > 0);

    // Clean up
    sem_close(mutex);
    sem_close(turn);
    sem_unlink("mutex");
    sem_unlink("turn");
    munmap(BankAccount, sizeof(int));
    unlink("bankaccount.txt");

    return 0;
}

void DearOldDad(sem_t *mutex, sem_t *turn, int *BankAccount) {
    srand(time(NULL));
    for (int i = 0; i < 10; i++) {
        sleep(rand() % 6);
        printf("Dear Old Dad: Attempting to Check Balance\n");

        sem_wait(mutex);
        int balance = *BankAccount;
        if (balance < 100) {
            int deposit = rand() % 101;
            if (deposit % 2 == 0) {
                *BankAccount += deposit;
                printf("Dear Old Dad: Deposits $%d / Balance = $%d\n", deposit, *BankAccount);
            } else {
                printf("Dear Old Dad: Doesn't have any money to give\n");
            }
        } else {
            printf("Dear Old Dad: Thinks Student has enough Cash ($%d)\n", balance);
        }
        sem_post(mutex);

        sem_post(turn);
    }
}

void PoorStudent(sem_t *mutex, sem_t *turn, int *BankAccount) {
    srand(time(NULL) * getpid());
    for (int i = 0; i < 10; i++) {
        sem_wait(turn);

        sleep(rand() % 6);
        printf("Poor Student: Attempting to Check Balance\n");

        sem_wait(mutex);
        int balance = *BankAccount;
        int need = rand() % 51;
        printf("Poor Student needs $%d\n", need);

        if (need <= balance) {
            *BankAccount -= need;
            printf("Poor Student: Withdraws $%d / Balance = $%d\n", need, *BankAccount);
        } else {
            printf("Poor Student: Not Enough Cash ($%d)\n", balance);
        }
        sem_post(mutex);
    }
}

void LovableMom(sem_t *mutex, sem_t *turn, int *BankAccount) {
    srand(time(NULL) * getpid());
    for (int i = 0; i < 10; i++) {
        sleep(rand() % 11);
        printf("Lovable Mom: Attempting to Check Balance\n");

        sem_wait(mutex);
        int balance = *BankAccount;
        int deposit = rand() % 126;
        *BankAccount += deposit;
        printf("Lovable Mom: Deposits $%d / Balance = $%d\n", deposit, *BankAccount);
        sem_post(mutex);

        sem_post(turn);
    }
}