#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

atomic_int slot = 0;

void *worker(void *arg) {
    int id = *(int *)arg;
    int expected = 0;
    if (atomic_compare_exchange_strong(&slot, &expected, id))
        printf("Thread %d CLAIMED the slot\n", id);
    else
        printf("Thread %d LOST; observed owner %d\n", id, expected);
    return NULL;
}

int main(void) {
    pthread_t a, b;
    int id_a = 1, id_b = 2;
    pthread_create(&a, NULL, worker, &id_a);
    pthread_create(&b, NULL, worker, &id_b);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("Final slot owner: %d\n", atomic_load(&slot));
    return 0;
}
