#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>

atomic_int checking = 500;
atomic_int savings = 500;
atomic_uint version = 0;

pthread_mutex_t writer_lock = PTHREAD_MUTEX_INITIALIZER;

void *writer(void *arg) {
    pthread_mutex_lock(&writer_lock);

    atomic_fetch_add(&version, 1); // 0 -> 1, writer active

    atomic_store(&checking, 400);
    usleep(100000);                // exaggerate update window
    atomic_store(&savings, 600);

    atomic_fetch_add(&version, 1); // 1 -> 2, stable again

    pthread_mutex_unlock(&writer_lock);
    return NULL;
}

void *reader(void *arg) {
    int c, s;
    unsigned v1, v2;
    int retries = 0;

    do {
        v1 = atomic_load(&version);

        if (v1 & 1) {
            retries++;
            continue;
        }

        c = atomic_load(&checking);
        s = atomic_load(&savings);

        v2 = atomic_load(&version);

        if (v1 != v2)
            retries++;

    } while ((v1 & 1) || v1 != v2);

    printf("checking: %d\n", c);
    printf("savings:  %d\n", s);
    printf("total:    %d\n", c + s);
    printf("version:  %u\n", v2);
    printf("retries:  %d\n", retries);

    return NULL;
}

int main(void) {
    pthread_t w, r;

    pthread_create(&w, NULL, writer, NULL);

    usleep(20000); // reader arrives during transfer

    pthread_create(&r, NULL, reader, NULL); 

    pthread_join(w, NULL);
    pthread_join(r, NULL);

    return 0;
}
// gcc -O2 -pthread snapshot.c -o snapshot
