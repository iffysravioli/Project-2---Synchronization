#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "BENSCHILLIBOWL.h"

/**
 * By default, we can pick some values, but we can override them using command-line arguments.
 */
int NUM_CUSTOMERS = 90;
int NUM_COOKS = 10;
int ORDERS_PER_CUSTOMER = 3;
int BENSCHILLIBOWL_SIZE = 100;
int EXPECTED_NUM_ORDERS;

BENSCHILLIBOWL *bcb;

void* BENSCHILLIBOWLCustomer(void* tid) {
    int customer_id = (int)(long) tid;
    int i;
    for (i = 0; i < ORDERS_PER_CUSTOMER; i++) {
        Order *order = (Order *)malloc(sizeof(Order));
        order->customer_id = customer_id;
        order->menu_item = PickRandomMenuItem();
        order->next = NULL;

        int order_number = AddOrder(bcb, order);
        if (order_number < 0) {
            // If somehow we couldn't add the order
            free(order);
        }
    }
    return NULL;
}

void* BENSCHILLIBOWLCook(void* tid) {
    int cook_id = (int)(long) tid;
    int orders_fulfilled = 0;

    while (1) {
        Order* order = GetOrder(bcb);
        if (order == NULL) {
            // No more orders to process
            break;
        }
        free(order);
        orders_fulfilled++;
    }

    printf("Cook #%d fulfilled %d orders\n", cook_id, orders_fulfilled);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        NUM_CUSTOMERS = atoi(argv[1]);
    }
    if (argc > 2) {
        NUM_COOKS = atoi(argv[2]);
    }
    if (argc > 3) {
        ORDERS_PER_CUSTOMER = atoi(argv[3]);
    }
    if (argc > 4) {
        BENSCHILLIBOWL_SIZE = atoi(argv[4]);
    }

    EXPECTED_NUM_ORDERS = NUM_CUSTOMERS * ORDERS_PER_CUSTOMER;

    printf("Running scenario: %d customers, %d cooks, %d orders per customer, queue size %d\n",
           NUM_CUSTOMERS, NUM_COOKS, ORDERS_PER_CUSTOMER, BENSCHILLIBOWL_SIZE);

    srand(time(NULL)); // Seed random number generator

    bcb = OpenRestaurant(BENSCHILLIBOWL_SIZE, EXPECTED_NUM_ORDERS);

    pthread_t customer_threads[NUM_CUSTOMERS];
    pthread_t cook_threads[NUM_COOKS];

    int i;
    // Create the cooks
    for (i = 0; i < NUM_COOKS; i++) {
        pthread_create(&cook_threads[i], NULL, BENSCHILLIBOWLCook, (void*)(long)(i+1));
    }

    // Create the customers
    for (i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_create(&customer_threads[i], NULL, BENSCHILLIBOWLCustomer, (void*)(long)(i+1));
    }

    // Wait for customers to finish
    for (i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_join(customer_threads[i], NULL);
    }

    // Wait for cooks to finish
    for (i = 0; i < NUM_COOKS; i++) {
        pthread_join(cook_threads[i], NULL);
    }

    CloseRestaurant(bcb);
    return 0;
}

