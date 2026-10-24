#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

#define N 10

long counter = 0;

sem_t turn_a;
sem_t turn_b;

pthread_barrier_t finish_line;

void *worker_a(void *arg){
    //
	for (int i = 0; i < N; i++) {
        sem_wait(&turn_a);
        counter++;
        printf("A: %ld\n", counter);
        sem_post(&turn_b);
    }

    printf("A reached finish barrier\n");
    pthread_barrier_wait(&finish_line);
    printf("A exits\n");

    return NULL;
}

void *worker_b(void *arg){
	//
    for (int i = 0; i < N; i++) {
        sem_wait(&turn_b);
        counter++;
        printf("B: %ld\n", counter);
        sem_post(&turn_a);
    }

    printf("B reached finish barrier\n");
    pthread_barrier_wait(&finish_line);
    printf("B exits\n");

    return NULL;
}

int main(void){
    pthread_t thread_a;
    pthread_t thread_b;

    /*
     * A starts with one permit.
     * B starts with zero.
     */
    sem_init(&turn_a, 0, 1);
    sem_init(&turn_b, 0, 0);

    pthread_barrier_init(&finish_line, NULL, 2);
	
    pthread_create(&thread_a,NULL, worker_a, NULL);
    pthread_create(&thread_b, NULL, worker_b,NULL);

    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);

    printf("Final counter: %ld\n", counter);

    sem_destroy(&turn_a);
    sem_destroy(&turn_b);
    pthread_barrier_destroy(&finish_line);

    return 0;
}