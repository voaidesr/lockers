#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include<unistd.h>
#include "vutex.h"
#include "rvlock.h"

#define NUM_THREADS 4
#define ITERATIONS 100000

// Mutex
int mutex_shared_counter = 0;
vutex_t mutex;

void *mutex_thread(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        vutex_lock(&mutex);
        mutex_shared_counter++;
        vutex_unlock(&mutex);
    }
    printf("Mutex thread %d completed\n", id);
    return NULL;
}

void test_mutex(void) {
    printf("\n=== MUTEX TEST ===\n");
    vutex_init(&mutex);
    mutex_shared_counter = 0;

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
    printf("Actual counter: %d\n", mutex_shared_counter);
    printf("Test %s\n", (mutex_shared_counter == NUM_THREADS * ITERATIONS) ? "PASSED" : "FAILED");
}

// Read-Write Lock
int rwlock_shared_data = 0;
rvlock_t rwlock;

void *reader_thread(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 5; i++) {
        rvlock_rdlock(&rwlock);
        printf("Reader %d read value: %d\n", id, rwlock_shared_data);
        usleep(500); // Simulate work
        rvlock_rdunlock(&rwlock);
    }
    printf("Reader thread %d completed\n", id);
    return NULL;
}

void *writer_thread(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 3; i++) {
        rvlock_wrlock(&rwlock);
        rwlock_shared_data++;
        printf("Writer %d updated value to: %d\n", id, rwlock_shared_data);
        sleep(1); // Simulate work
        rvlock_wrunlock(&rwlock);
        sleep(rand() % 3); // Random wait before next write
    }
    printf("Writer thread %d completed\n", id);
    return NULL;
}

void test_rwlock(void) {
    printf("\n=== READ-WRITE LOCK TEST ===\n");
    rvlock_init(&rwlock);
    rwlock_shared_data = 0;

    pthread_t readers[4];
    pthread_t writers[2];
    int reader_ids[] = {0, 1, 2, 3};
    int writer_ids[] = {0, 1};

    for (int i = 0; i < 4; i++) {
        pthread_create(&readers[i], NULL, reader_thread, &reader_ids[i]);
    }
    for (int i = 0; i < 2; i++) {
        pthread_create(&writers[i], NULL, writer_thread, &writer_ids[i]);
    }

    for (int i = 0; i < 4; i++) {
        pthread_join(readers[i], NULL);
    }
    for (int i = 0; i < 2; i++) {
        pthread_join(writers[i], NULL);
    }

    printf("Final shared data value: %d (expected: 6)\n", rwlock_shared_data);
    printf("RWLock test %s\n", (rwlock_shared_data == 6) ? "PASSED" : "FAILED");
}

int main(void) {
    test_mutex();
    test_rwlock();
    return 0;
}
