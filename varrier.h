#ifndef VARRIER_H
#define VARRIER_H

#ifndef cpu_pause
    #define cpu_pause() do { } while (0)
#endif

typedef struct {
    volatile unsigned int count;
    volatile unsigned int generation;
    unsigned int total;
} varrier_t;

static inline void varrier_init(varrier_t* barrier, unsigned int count) {
    __atomic_store_n(&barrier->count, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&barrier->generation, 0, __ATOMIC_RELEASE);
    barrier->total = count;
}

static inline bool varrier_wait(varrier_t *barrier) {
    unsigned int gen = __atomic_load_n(&barrier->generation, __ATOMIC_ACQUIRE);
    unsigned int arrived = __atomic_fetch_add(&barrier->count, 1, __ATOMIC_ACQ_REL) + 1;

    if (arrived == barrier->total) {
        __atomic_store_n(&barrier->count, 0, __ATOMIC_RELEASE);
        __atomic_fetch_add(&barrier->generation, 1, __ATOMIC_RELEASE);
        return true;
    }

    while (__atomic_load_n(&barrier->generation, __ATOMIC_ACQUIRE) == gen) {
        cpu_pause();
    }

    return false;
}

#endif // VARRIER_H