#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

atomic_int counter = 0;
pthread_barrier_t after_load;

void *worker(void *arg) {
    (void)arg;
    int observed = atomic_load(&counter);
    pthread_barrier_wait(&after_load);
    atomic_store(&counter, observed + 1);
    return NULL;
}

int main(void) {
    pthread_t a, b;
    pthread_barrier_init(&after_load, NULL, 2);
    pthread_create(&a, NULL, worker, NULL);
    pthread_create(&b, NULL, worker, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("logical increments=2 final counter=%d\n", atomic_load(&counter));
    assert(atomic_load(&counter) == 1);
    pthread_barrier_destroy(&after_load);
    return 0;
}
