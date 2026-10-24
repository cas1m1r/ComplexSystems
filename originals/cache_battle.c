#include <pthread.h>
#include <stdatomic.h>

#define N 100000000

atomic_long a = 0;
atomic_long b = 0;

void *worker_a(void *arg)
{
    for (long i = 0; i < N; i++){
        atomic_fetch_add_explicit(&a, 1, memory_order_relaxed);
		printf("B: %ld\n", b);
	}
    return NULL;
}

void *worker_b(void *arg)
{
    for (long i = 0; i < N; i++){
        atomic_fetch_add_explicit(&b, 1, memory_order_relaxed);
		printf("A: %ld\n", a);
	}
    return NULL;
}



int main(void){
    pthread_t a, b;

    int id_a = 1;
    int id_b = 2;

    pthread_create(&a, NULL, worker_a, &id_a);
    pthread_create(&b, NULL, worker_b, &id_b);

    pthread_join(a, NULL);
    pthread_join(b, NULL);

    printf("A: %ld\n", a);
	printf("B: %ld\n", b);
    return 0;
}