
#ifndef SEMAVORE_H
#define SEMAVORE_H
// SEMAPHORE

typedef struct {
    volatile int count;
} sync_semaphore_t;

static inline void sync_semaphore_init(sync_semaphore_t *sem, int initial_count) {
    __atomic_store_n(&sem->count, initial_count, __ATOMIC_RELEASE);
}

static inline void sync_semaphore_wait(sync_semaphore_t *sem) {
    while (1) {
        int current = __atomic_load_n(&sem->count, __ATOMIC_ACQUIRE);
        if (current > 0) {
            // parm weak = false, sets strong behaviour (guarantees success for )
            if (__atomic_compare_exchange_n(&sem->count, &current, current - 1, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED) != 0) {
                return;
            }
        }
        else {
            cpu_pause();
        }
    }
}