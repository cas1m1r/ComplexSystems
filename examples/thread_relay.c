#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#define TURNS 10
long counter = 0;
sem_t turn_a, turn_b;
pthread_barrier_t finish_line;

void wait_turn(sem_t *turn) {
    while (sem_wait(turn) == -1) {
        if (errno == EINTR) continue;
        perror("sem_wait");
        exit(EXIT_FAILURE);
    }
}

void *worker_a(void *arg) {
    (void)arg;
    for (int i = 0; i < TURNS; i++) {
        wait_turn(&turn_a);
        printf("A: %ld\n", ++counter);
        sem_post(&turn_b);
    }
    puts("A reached finish barrier");
    pthread_barrier_wait(&finish_line);
    puts("A exits");
    return NULL;
}

void *worker_b(void *arg) {
    (void)arg;
    for (int i = 0; i < TURNS; i++) {
        wait_turn(&turn_b);
        printf("B: %ld\n", ++counter);
        sem_post(&turn_a);
    }
    puts("B reached finish barrier");
    pthread_barrier_wait(&finish_line);
    puts("B exits");
    return NULL;
}

int main(void) {
    pthread_t a, b;
    sem_init(&turn_a, 0, 1);
    sem_init(&turn_b, 0, 0);
    pthread_barrier_init(&finish_line, NULL, 2);
    pthread_create(&a, NULL, worker_a, NULL);
    pthread_create(&b, NULL, worker_b, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("Final counter: %ld\n", counter);
    sem_destroy(&turn_a);
    sem_destroy(&turn_b);
    pthread_barrier_destroy(&finish_line);
    return 0;
}
