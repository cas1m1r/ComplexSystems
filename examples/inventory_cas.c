#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define THREADS 8
#define TICKETS 100
#define ATTEMPTS 100000

atomic_int remaining = TICKETS;

int try_buy_ticket(void) {
    int current = atomic_load_explicit(&remaining, memory_order_relaxed);

    while (current > 0) {
        if (atomic_compare_exchange_weak_explicit(&remaining, &current, current - 1, memory_order_relaxed, memory_order_relaxed)) {
            return 1;
        }
    }

    return 0;
}

void *worker(void *arg) {
    (void)arg;
    for (int i = 0; i < ATTEMPTS; i++)
        try_buy_ticket();

    return NULL;
}

int main(void) {
    pthread_t threads[THREADS];

    for (int i = 0; i < THREADS; i++)
        pthread_create(&threads[i], NULL, worker, NULL);

    for (int i = 0; i < THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("Remaining: %d\n", atomic_load(&remaining));
    return 0;
}
