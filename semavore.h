
#ifndef SEMAVORE_H
#define SEMAVORE_H

#define cpu_pause() do { } while(0)

typedef struct {
    volatile int count;
} semavore_t;

static inline void semavore_init(semavore_t *sem, int initial_count) {
    __atomic_store_n(&sem->count, initial_count, __ATOMIC_RELEASE);
}

static inline void semavore_wait(semavore_t *sem) {
    while (1) {
        int current = __atomic_load_n(&sem->count, __ATOMIC_ACQUIRE);
        if (current > 0) {
            // param weak = false, sets strong behaviour (guarantees success for conditions guaranteed)
            if (__atomic_compare_exchange_n(&sem->count, &current, current - 1, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED) != 0) {
                return;
            }
        }
        else {
            cpu_pause();
        }
    }
}

static inline bool semavore_trywait(semavore_t *sem) {
    int current = __atomic_load_n(&sem->count, __ATOMIC_ACQUIRE);
    if (current > 0) {
        return __atomic_compare_exchange_n(&sem->count, &current, current - 1, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
    }
    return false;
}

static inline void semavore_signal(semavore_t *sem) {
    __atomic_fetch_add(&sem->count, 1, __ATOMIC_RELEASE);
}

static inline int semavore_getvalue(semavore_t *sem) {
    __atomic_load_n(&sem->count, __ATOMIC_ACQUIRE);
}

#endif // SEMAVORE_H