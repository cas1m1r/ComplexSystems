#include <pthread.h>
#include <stdio.h>

#define THREADS 8
#define TICKETS 100
#define ATTEMPTS 100000

int remaining = TICKETS;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg) {
    (void)arg;
    for (int i = 0; i < ATTEMPTS; i++) {
        pthread_mutex_lock(&lock);

        if (remaining > 0)
            remaining--;

        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

int main(void) {
    pthread_t threads[THREADS];

    for (int i = 0; i < THREADS; i++)
        pthread_create(&threads[i], NULL, worker, NULL);

    for (int i = 0; i < THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("Remaining: %d\n", remaining);
    return 0;
}
