#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#ifndef ITERATIONS
#define ITERATIONS 1000000L
#endif

long counter = 0;
atomic_flag lock = ATOMIC_FLAG_INIT;

void spin_lock(void) {
    while (atomic_flag_test_and_set(&lock)) {
        // Busy wait
    }
}

void spin_unlock(void) {
    atomic_flag_clear(&lock);
}

void *worker(void *arg) {
    (void)arg;
    for (long i = 0; i < ITERATIONS; i++) {
        spin_lock();
        counter++;
        spin_unlock();
    }
    return NULL;
}

int main(void) {
    pthread_t a, b;
    pthread_create(&a, NULL, worker, NULL);
    pthread_create(&b, NULL, worker, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("expected=%ld actual=%ld\n", 2L * ITERATIONS, counter);
    return 0;
}
