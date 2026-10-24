#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

int data = 0;
atomic_int ready = 0;

void *producer(void *arg) {
    (void)arg;
    data = 42;
    atomic_store_explicit(&ready, 1, memory_order_release);
    return NULL;
}

void *consumer(void *arg) {
    (void)arg;
    while (atomic_load_explicit(&ready, memory_order_acquire) == 0) {
    }
    printf("data = %d\n", data);
    return NULL;
}

int main(void) {
    pthread_t a, b;
    pthread_create(&a, NULL, producer, NULL);
    pthread_create(&b, NULL, consumer, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    return 0;
}
