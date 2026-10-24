#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

enum { NAIVE, MUTEX, CAS };
atomic_int seats = 1, sold = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t gate;
int mode;

void *reserve(void *arg) {
    const char *name = arg;
    if (mode == MUTEX) {
        pthread_barrier_wait(&gate); // Rendezvous before taking the lock
        pthread_mutex_lock(&lock);
        if (atomic_load(&seats) > 0) {
            atomic_fetch_sub(&seats, 1);
            atomic_fetch_add(&sold, 1);
            printf("%s bought a ticket\n", name);
        }
        pthread_mutex_unlock(&lock);
    } else {
        int expected = atomic_load(&seats);
        pthread_barrier_wait(&gate); // Both observed the original seat count
        if (expected > 0) {
            if (mode == NAIVE) {
                atomic_store(&seats, expected - 1);
                atomic_fetch_add(&sold, 1);
                printf("%s bought a ticket from a stale snapshot\n", name);
            } else if (atomic_compare_exchange_strong(&seats, &expected, expected - 1)) {
                atomic_fetch_add(&sold, 1);
                printf("%s bought the ticket\n", name);
            } else {
                printf("%s lost the race; seats now %d\n", name, expected);
            }
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2 || (strcmp(argv[1], "naive") && strcmp(argv[1], "mutex") && strcmp(argv[1], "cas"))) {
        fprintf(stderr, "usage: %s naive|mutex|cas\n", argv[0]);
        return 2;
    }
    mode = !strcmp(argv[1], "naive") ? NAIVE : !strcmp(argv[1], "mutex") ? MUTEX : CAS;
    pthread_t a, b;
    pthread_barrier_init(&gate, NULL, 2);
    pthread_create(&a, NULL, reserve, "A");
    pthread_create(&b, NULL, reserve, "B");
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("remaining=%d sold=%d\n", atomic_load(&seats), atomic_load(&sold));
    pthread_barrier_destroy(&gate);
    pthread_mutex_destroy(&lock);
    return 0;
}
