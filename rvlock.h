#ifndef RVLOCK_H
#define RVLOCK_H

#define RVLOCK_WRITER_BIT 0x80000000U // 2^31

#ifndef cpu_pause
    #define cpu_pause() do { } while (0)
#endif

typedef struct {
    volatile unsigned int state;
} rvlock_t;

static inline void rvlock_init(rvlock_t* lock) {
    __atomic_store_n(&lock->state, 0, __ATOMIC_RELEASE);
}

static inline void rvlock_rdlock(rvlock_t *lock) {
    while (1) {
        unsigned int current = __atomic_load_n(&lock->state, __ATOMIC_ACQUIRE);
        if ((current & RVLOCK_WRITER_BIT) == 0) { // No writer present
            unsigned int new_state = current + 1;
            if (__atomic_compare_exchange_n(&lock->state, &current, new_state,
                                            false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
                return;
            }
        } else { // Writer present, wait
            cpu_pause();
        }
    }
}

static inline bool rvlock_tryrdlock(rvlock_t *lock) {
    unsigned int current = __atomic_load_n(&lock->state, __ATOMIC_ACQUIRE);
    if ((current & RVLOCK_WRITER_BIT) == 0) { // No writer present
        unsigned int new_state = current + 1;
        return __atomic_compare_exchange_n(&lock->state, &current, new_state,
                                           false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
    }
    return false; // Writer present
}

static inline void rvlock_rdunlock(rvlock_t *lock) {
    __atomic_fetch_sub(&lock->state, 1, __ATOMIC_RELEASE);
}

static inline void rvlock_wrlock(rvlock_t *lock) {
    while (1) {
        unsigned int current = __atomic_load_n(&lock->state, __ATOMIC_ACQUIRE);
        if ((current & RVLOCK_WRITER_BIT) == 0) { // No writer present
            unsigned int new_state = current | RVLOCK_WRITER_BIT;
            if (__atomic_compare_exchange_n(&lock->state, &current, new_state,
                                            false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
                // Wait for readers to exit
                while ((__atomic_load_n(&lock->state, __ATOMIC_ACQUIRE) & ~RVLOCK_WRITER_BIT) != 0) {
                    cpu_pause();
                }
                return;
            }
        } else { // Writer present, wait
            cpu_pause();
        }
    }
}

static inline bool rvlock_trywrlock(rvlock_t *lock) {
    unsigned int expected = 0;
    return __atomic_compare_exchange_n(&lock->state, &expected, RVLOCK_WRITER_BIT,
                                       false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
}

static inline void rvlock_wrunlock(rvlock_t *lock) {
    __atomic_fetch_sub(&lock->state, RVLOCK_WRITER_BIT, __ATOMIC_RELEASE);
}

#endif // RVLOCK_H