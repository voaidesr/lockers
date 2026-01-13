#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include<unistd.h>
#include "vutex.h"
#include "semavore.h"
#include "rvlock.h"
#include "varrier.h"

#define NUM_THREADS 4
#define ITERATIONS 100000
#define SEM_BUFFER_SIZE 10
#define SEM_PRODUCERS 2
#define SEM_CONSUMERS 2
#define SEM_ITEMS_PER_THREAD 20

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

// Semaphore
typedef struct {
    int id;
    int is_producer;
} sem_thread_arg_t;

int buffer[SEM_BUFFER_SIZE];
int buffer_in = 0;
int buffer_out = 0;
int produced_total = 0;
int consumed_total = 0;
semavore_t empty_slots;
semavore_t full_slots;
vutex_t buffer_mutex;

void *semaphore_thread(void *arg) {
    sem_thread_arg_t *ctx = (sem_thread_arg_t *)arg;
    if (ctx->is_producer) {
        for (int i = 0; i < SEM_ITEMS_PER_THREAD; i++) {
            semavore_wait(&empty_slots);
            vutex_lock(&buffer_mutex);

            buffer[buffer_in] = ctx->id * 100 + i;
            buffer_in = (buffer_in + 1) % SEM_BUFFER_SIZE;
            produced_total++;

            vutex_unlock(&buffer_mutex);
            semavore_signal(&full_slots);
        }
        printf("Semaphore producer %d completed\n", ctx->id);
    } else {
        for (int i = 0; i < SEM_ITEMS_PER_THREAD; i++) {
            semavore_wait(&full_slots);
            vutex_lock(&buffer_mutex);

            int item = buffer[buffer_out];
            (void)item;
            buffer_out = (buffer_out + 1) % SEM_BUFFER_SIZE;
            consumed_total++;

            vutex_unlock(&buffer_mutex);
            semavore_signal(&empty_slots);
        }
        printf("Semaphore consumer %d completed\n", ctx->id);
    }
    return NULL;
}

void test_semaphore(void) {
    printf("\n=== SEMAPHORE TEST ===\n");

    vutex_init(&buffer_mutex);
    semavore_init(&empty_slots, SEM_BUFFER_SIZE);
    semavore_init(&full_slots, 0);
    buffer_in = 0;
    buffer_out = 0;
    produced_total = 0;
    consumed_total = 0;

    pthread_t producers[SEM_PRODUCERS];
    pthread_t consumers[SEM_CONSUMERS];
    sem_thread_arg_t prod_args[SEM_PRODUCERS];
    sem_thread_arg_t cons_args[SEM_CONSUMERS];

    for (int i = 0; i < SEM_PRODUCERS; i++) {
        prod_args[i].id = i;
        prod_args[i].is_producer = 1;
        pthread_create(&producers[i], NULL, semaphore_thread, &prod_args[i]);
    }

    for (int i = 0; i < SEM_CONSUMERS; i++) {
        cons_args[i].id = i;
        cons_args[i].is_producer = 0;
        pthread_create(&consumers[i], NULL, semaphore_thread, &cons_args[i]);
    }

    for (int i = 0; i < SEM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }

    for (int i = 0; i < SEM_CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }

    int expected_total = SEM_PRODUCERS * SEM_ITEMS_PER_THREAD;
    int ok = (produced_total == expected_total &&
              consumed_total == expected_total &&
              buffer_in == buffer_out);

    printf("Expected produced: %d\n", expected_total);
    printf("Actual produced: %d\n", produced_total);
    printf("Expected consumed: %d\n", expected_total);
    printf("Actual consumed: %d\n", consumed_total);
    printf("Test %s\n", ok ? "PASSED" : "FAILED");
}

// Barrier
varrier_t barrier;
int generation = 0;

void *barrier_thread(void *arg) {
    int id = *(int *)arg;
    printf("Thread %d waiting at barrier. %d generations\n", id, generation);
    bool is_last = varrier_wait(&barrier);
    if (is_last) {
        generation++;
        printf("Thread %d was the last to arrive at barrier. Generation %d completed.\n", id, generation);
    }
    printf("Thread %d passed the barrier.\n", id);
    return NULL;
}

void test_barrier(void) {
    printf("\n=== BARRIER TEST ===\n");
    varrier_init(&barrier, NUM_THREADS);

    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, barrier_thread, &ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Barrier test completed\n");
}

int main(void) {
    test_mutex();
    test_semaphore();
    test_rwlock();
    test_barrier();
    return 0;
}
