#include <stdatomic.h>
#include <pthread.h>
#include <stdio.h>

#ifndef ITERATIONS
#define ITERATIONS 1000000L
#endif

atomic_long counter = 0;

void *worker(void *arg) {
    (void)arg;
    for (long i = 0; i < ITERATIONS; i++) {
        atomic_fetch_add(&counter, 1);
    }
    return NULL;
}

int main(void) {
    pthread_t a, b;
    pthread_create(&a, NULL, worker, NULL);
    pthread_create(&b, NULL, worker, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("expected=%ld actual=%ld\n", 2L * ITERATIONS, atomic_load(&counter));
    return 0;
}
