#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define THREADS 8
#define TICKETS 100
#define ATTEMPTS 100000

atomic_int remaining = TICKETS;

void *worker(void *arg){
    for (int i = 0; i < ATTEMPTS; i++) {
        if (atomic_load_explicit(&remaining, memory_order_relaxed) > 0) {
            atomic_fetch_sub_explicit(&remaining, 1, memory_order_relaxed);
        }
    }

    return NULL;
}

int main(void){
    pthread_t threads[THREADS];

    for (int i = 0; i < THREADS; i++)
        pthread_create(&threads[i], NULL, worker, NULL);

    for (int i = 0; i < THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("Remaining: %d\n", atomic_load(&remaining));
    return 0;
}