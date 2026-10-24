#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define TICKETS 100000
#define THREADS 8

enum { PESSIMISTIC, OPTIMISTIC };

_Atomic int owner[TICKETS];
_Atomic int sold = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

struct Stats {
    int id;
    int mode;
    long attempts;
    long occupied;
    long races;
    long wins;
};

uint32_t rng(uint32_t *state) {
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

void *worker(void *arg) {
    struct Stats *s = arg;
    uint32_t seed = 0x9e3779b9u ^ (s->id + 1);

    while (atomic_load_explicit(&sold, memory_order_relaxed) < TICKETS) {
        int ticket = rng(&seed) % TICKETS;
        s->attempts++;

        if (s->mode == PESSIMISTIC) {
            pthread_mutex_lock(&lock);

            if (atomic_load_explicit(&owner[ticket], memory_order_relaxed) == -1) {
                atomic_store_explicit(&owner[ticket], s->id, memory_order_relaxed);
                atomic_fetch_add_explicit(&sold, 1, memory_order_relaxed);
                s->wins++;
            } else {
                s->occupied++;
            }

            pthread_mutex_unlock(&lock);
        } else {
            if (atomic_load_explicit(&owner[ticket], memory_order_relaxed) != -1) {
                s->occupied++;
                continue;
            }

            int expected = -1;

            if (atomic_compare_exchange_strong_explicit(&owner[ticket], &expected, s->id, memory_order_relaxed, memory_order_relaxed)) {

                atomic_fetch_add_explicit(&sold, 1, memory_order_relaxed);
                s->wins++;
            } else {
                s->races++;
            }
        }
    }

    return NULL;
}

double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void run_test(int mode) {
    pthread_t threads[THREADS];
    struct Stats stats[THREADS];

    atomic_store(&sold, 0);

    for (int i = 0; i < TICKETS; i++)
        atomic_store(&owner[i], -1);

    double start = now();

    for (int i = 0; i < THREADS; i++) {
        stats[i] = (struct Stats){
            .id = i,
            .mode = mode
        };

        pthread_create(&threads[i], NULL, worker, &stats[i]);
    }

    for (int i = 0; i < THREADS; i++)
        pthread_join(threads[i], NULL);

    double end = now();
    int claimed = 0;
    for (int i = 0; i < TICKETS; i++) {
        int id = atomic_load(&owner[i]);
        assert(id >= 0 && id < THREADS);
        claimed++;
    }
    assert(claimed == TICKETS && atomic_load(&sold) == TICKETS);

    long attempts = 0;
    long occupied = 0;
    long races = 0;
    long wins = 0;

    for (int i = 0; i < THREADS; i++) {
        attempts += stats[i].attempts;
        occupied += stats[i].occupied;
        races += stats[i].races;
        wins += stats[i].wins;
    }

    assert(wins == TICKETS);
    printf("\n%s\n", mode == PESSIMISTIC ? "PESSIMISTIC" : "OPTIMISTIC");
    printf("tickets sold: %ld\n", wins);
    printf("attempts:     %ld\n", attempts);
    printf("occupied:     %ld\n", occupied);
    printf("lost races:   %ld\n", races);
    printf("time:         %.6f s\n", end - start);
}

int main(void) {
    run_test(PESSIMISTIC);
    run_test(OPTIMISTIC);
}
