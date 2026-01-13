#pragma once

#include <stdint.h>
#include <stdbool.h>

#define cpu_pause() do { } while (0)

typedef struct {
    volatile int locked;
} sync_mutex_t;

static inline void sync_mutex_init(sync_mutex_t* mutex) {
    __atomic_store_n(&mutex->locked, 0, __ATOMIC_RELEASE);
}

static inline void sync_mutex_lock(sync_mutex_t* mutex) {
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

static inline bool sync_mutex_trylock(sync_mutex_t* mutex) {
    int expected = 0;
    return __atomic_compare_exchange_n(&mutex->locked, &expected, 1,
                                       false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}

static inline void sync_mutex_unlock(sync_mutex_t* mutex) {
    __atomic_store_n(&mutex->locked, 0, __ATOMIC_RELEASE);
}