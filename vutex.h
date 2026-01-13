#ifndef VUTEX_H
#define VUTEX_H

#include <stdint.h>
#include <stdbool.h>

#ifndef cpu_pause
    #define cpu_pause() do { } while (0)
#endif

typedef struct {
    volatile int locked;
} vutex_t;

static inline void vutex_init(vutex_t* mutex) {
    __atomic_store_n(&mutex->locked, 0, __ATOMIC_RELEASE);
}

static inline void vutex_lock(vutex_t* mutex) {
    while (1) {
        int expected = 0;
        if (__atomic_compare_exchange_n(&mutex->locked, &expected, 1,
                                        false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
            return;
        }
        while (__atomic_load_n(&mutex->locked, __ATOMIC_RELAXED) != 0) {
            cpu_pause();
        }
    }
}

static inline bool vutex_trylock(vutex_t* mutex) {
    int expected = 0;
    return __atomic_compare_exchange_n(&mutex->locked, &expected, 1,
                                       false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}

static inline void vutex_unlock(vutex_t* mutex) {
    __atomic_store_n(&mutex->locked, 0, __ATOMIC_RELEASE);
}

#endif // VUTEX_H