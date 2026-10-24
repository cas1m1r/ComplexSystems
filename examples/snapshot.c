#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

atomic_int checking = 500, savings = 500;
atomic_uint version = 0;
pthread_mutex_t writer_lock = PTHREAD_MUTEX_INITIALIZER;

void delay_ms(long ms) {
    struct timespec t = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };
    nanosleep(&t, NULL);
}

void *writer(void *arg) {
    (void)arg;
    pthread_mutex_lock(&writer_lock);
    atomic_fetch_add(&version, 1); // Odd: update in progress
    atomic_store(&checking, 400);
    delay_ms(100);
    atomic_store(&savings, 600);
    atomic_fetch_add(&version, 1); // Even: complete version
    pthread_mutex_unlock(&writer_lock);
    return NULL;
}

void *reader(void *arg) {
    (void)arg;
    unsigned long long retries = 0;
    for (;;) {
        unsigned before = atomic_load(&version);
        if (before & 1u) {
            retries++;
            continue;
        }
        int c = atomic_load(&checking);
        int s = atomic_load(&savings);
        unsigned after = atomic_load(&version);
        if (before != after) {
            retries++;
            continue;
        }
        printf("checking=%d savings=%d total=%d version=%u retries=%llu\n", c, s, c + s, after, retries);
        return NULL;
    }
}

int main(void) {
    pthread_t w, r;
    pthread_create(&w, NULL, writer, NULL);
    delay_ms(20); // Encourages overlap; does not guarantee scheduling
    pthread_create(&r, NULL, reader, NULL);
    pthread_join(w, NULL);
    pthread_join(r, NULL);
    pthread_mutex_destroy(&writer_lock);
    return 0;
}
