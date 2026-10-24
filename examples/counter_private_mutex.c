#include <pthread.h>
#include <stdio.h>

#ifndef ITERATIONS
#define ITERATIONS 1000000L
#endif

long counter = 0;

void *worker(void *arg) {
    (void)arg;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // Different object per worker
    for (long i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&lock);
        counter++; // Still a data race: workers use different locks
        pthread_mutex_unlock(&lock);
    }
    pthread_mutex_destroy(&lock);
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
