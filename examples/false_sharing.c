#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

#ifndef ITERATIONS
#define ITERATIONS 10000000L
#endif
#define CACHE_LINE 64

_Alignas(CACHE_LINE) atomic_long shared_counter = 0;
struct Together {
    _Alignas(CACHE_LINE) atomic_long a;
    atomic_long b;
} together;
struct Apart {
    _Alignas(CACHE_LINE) atomic_long a;
    _Alignas(CACHE_LINE) atomic_long b;
} apart;

_Static_assert(offsetof(struct Together, b) + sizeof(atomic_long) <= CACHE_LINE, "Together must fit within 64 bytes");
_Static_assert(offsetof(struct Apart, b) >= CACHE_LINE, "Apart needs separate assumed lines");

void *worker(void *arg) {
    atomic_long *counter = arg;
    for (long i = 0; i < ITERATIONS; i++)
        atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);
    return NULL;
}

double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void run(const char *label, atomic_long *a, atomic_long *b) {
    pthread_t ta, tb;
    atomic_store(a, 0);
    if (a != b) atomic_store(b, 0);
    double start = now();
    pthread_create(&ta, NULL, worker, a);
    pthread_create(&tb, NULL, worker, b);
    pthread_join(ta, NULL);
    pthread_join(tb, NULL);
    double elapsed = now() - start;
    long ca = atomic_load(a), cb = atomic_load(b);
    assert(ca == (a == b ? 2L * ITERATIONS : ITERATIONS));
    assert(cb == (a == b ? 2L * ITERATIONS : ITERATIONS));
    printf("%s: A=%ld B=%ld seconds=%.6f addresses=%p,%p\n", label, ca, cb, elapsed, (void *)a, (void *)b);
}

int main(void) {
    printf("member offsets: together=%zu apart=%zu assumed_line=%d\n", offsetof(struct Together, b), offsetof(struct Apart, b), CACHE_LINE);
    run("same atomic", &shared_counter, &shared_counter);
    run("false sharing", &together.a, &together.b);
    run("separate lines", &apart.a, &apart.b);
    return 0;
}
