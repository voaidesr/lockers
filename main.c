#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "vutex.h"
#include "semavore.h"

#define NUM_THREADS 4
#define ITERATIONS 100000
#define SEM_BUFFER_SIZE 10
#define SEM_PRODUCERS 2
#define SEM_CONSUMERS 2
#define SEM_ITEMS_PER_THREAD 20

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

typedef struct {
    int id;
    int is_producer;
} sem_thread_arg_t;

static int buffer[SEM_BUFFER_SIZE];
static int buffer_in = 0;
static int buffer_out = 0;
static int produced_total = 0;
static int consumed_total = 0;
static semavore_t empty_slots;
static semavore_t full_slots;
static vutex_t buffer_mutex;

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

int main(void) {
    test_mutex();
    test_semaphore();
    return 0;
}
