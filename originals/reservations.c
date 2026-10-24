#include <pthread.h>
#include <stdio.h>

int seats = 1;
int tickets_sold = 0;

pthread_barrier_t barrier;

void *reserve(void *arg) {
    char *name = arg;

    int snapshot = seats;

    printf("%s sees %d seat(s)\n", name, snapshot);

    pthread_barrier_wait(&barrier);

    if (snapshot > 0) {
        seats = snapshot - 1;
        tickets_sold++;

        printf("%s bought a ticket\n", name);
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