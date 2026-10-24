#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

atomic_int seats = 1;
atomic_int tickets_sold = 0;

pthread_barrier_t barrier;

void *reserve(void *arg) {
    char *name = arg;

    int expected = atomic_load(&seats);

    printf("%s sees %d seat(s)\n", name, expected);

    pthread_barrier_wait(&barrier);

    if (expected <= 0) return NULL;

    int desired = expected - 1;

    if (atomic_compare_exchange_strong(&seats, &expected, desired)) {
        atomic_fetch_add(&tickets_sold, 1);
        printf("%s bought the ticket\n", name);
    } else {
        printf("%s failed: state changed underneath them (now %d)\n",
            name, expected);
    }

    return NULL;
}


int main(void) {
    pthread_t a, b;

    pthread_barrier_init(&barrier, NULL, 2);

    pthread_create(&a, NULL, reserve, "A");
    pthread_create(&b, NULL, reserve, "B");

    pthread_join(a, NULL);
    pthread_join(b, NULL);

    printf("\nSeats remaining: %d\n", seats);
    printf("Tickets sold:    %d\n", tickets_sold);

    pthread_barrier_destroy(&barrier);
}