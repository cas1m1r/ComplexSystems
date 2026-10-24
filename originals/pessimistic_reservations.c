#include <pthread.h>
#include <stdio.h>

int seats = 1;
int tickets_sold = 0;

pthread_barrier_t barrier;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *reserve(void *arg) {
    char *name = arg;

    pthread_mutex_lock(&lock);

    if (seats > 0) {
        printf("%s sees %d seat(s)\n", name, seats);

        seats--;
        tickets_sold++;

        printf("%s bought a ticket\n", name);
    } else {
        printf("%s found no seats\n", name);
    }

    pthread_mutex_unlock(&lock);

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