#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "vutex.h"

#define NUM_THREADS 4
#define ITERATIONS 100000

static int shared_counter = 0;
static vutex_t mutex;

void *mutex_thread(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        vutex_lock(&mutex);
        shared_counter++;
        vutex_unlock(&mutex);
    }
    printf("Mutex thread %d completed\n", id);
    return NULL;
}

void test_mutex(void) {
    printf("\n=== MUTEX TEST ===\n");
    vutex_init(&mutex);
    shared_counter = 0;

    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, mutex_thread, &ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Expected counter: %d\n", NUM_THREADS * ITERATIONS);
    printf("Actual counter: %d\n", shared_counter);
    printf("Test %s\n", (shared_counter == NUM_THREADS * ITERATIONS) ? "PASSED" : "FAILED");
}

int main(void) {
    test_mutex();
    return 0;
}
