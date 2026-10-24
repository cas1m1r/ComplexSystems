#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

#define N 100000000L
#define CACHE_LINE 64

/*
 * Case 1:
 * Both threads touch exactly the same atomic.
 */
_Alignas(CACHE_LINE)
atomic_long shared_counter = 0;


/*
 * Case 2:
 * a and b are deliberately placed together.
 *
 * Because the struct itself starts at a 64-byte boundary,
 * these two longs should occupy the same cache line on the
 * typical 64-byte-cache-line x86 machine we're studying.
 */
typedef struct {
    _Alignas(CACHE_LINE) atomic_long a;
    atomic_long b;
} false_shared_pair;

false_shared_pair false_pair;


/*
 * Case 3:
 * Force a and b onto separate 64-byte boundaries.
 */
typedef struct {
    _Alignas(CACHE_LINE) atomic_long a;
    _Alignas(CACHE_LINE) atomic_long b;
} separated_pair;

separated_pair separate_pair;


typedef struct {
    atomic_long *counter;
} worker_args;


void *worker(void *arg){
    worker_args *args = arg;
	
    for (long i = 0; i < N; i++)
        atomic_fetch_add_explicit(args->counter, 1, memory_order_relaxed);

    return NULL;
}


double elapsed_seconds(struct timespec start,struct timespec end){
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}


void run_test(const char *name, atomic_long *counter_a, atomic_long *counter_b){
    pthread_t thread_a;
    pthread_t thread_b;

    worker_args args_a = {  .counter = counter_a};
    worker_args args_b = {  .counter = counter_b};
    atomic_store_explicit(counter_a, 0, memory_order_relaxed);

    /*
     * Don't reset twice if both pointers refer
     * to the same atomic.
     */
    if (counter_b != counter_a)
        atomic_store_explicit(counter_b, 0,memory_order_relaxed);

    struct timespec start;
    struct timespec end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_create(&thread_a, NULL, worker, &args_a);
    pthread_create(&thread_b, NULL, worker, &args_b);

    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);

    long result_a = atomic_load_explicit(counter_a,memory_order_relaxed);

    long result_b;

    if (counter_a == counter_b) {
        result_b = result_a;
    } else {
        result_b = atomic_load_explicit(counter_b, memory_order_relaxed);
    }

    printf("\n%s\n", name);
    printf("  address A: %p\n", (void *)counter_a);
    printf("  address B: %p\n", (void *)counter_b);
    printf("  distance : %ld bytes\n", (long)((char *)counter_b - (char *)counter_a));
    printf("  A = %ld\n", result_a);

    if (counter_a != counter_b) {
        printf("  B = %ld\n", result_b);
    }
    printf("  elapsed = %.3f seconds\n", elapsed_seconds(start, end));
}


int main(void){
    run_test("1. SAME ATOMIC", &shared_counter, &shared_counter);

    run_test("2. FALSE SHARING", &false_pair.a, &false_pair.b);

    run_test("3. SEPARATE CACHE LINES", &separate_pair.a, &separate_pair.b);

    return 0;
}