#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <time.h>

#define REPLICAS 4

struct Account {
    int checking;
    int savings;
};

struct Reply {
    int replica_id;
    struct Account state;
    struct timespec arrived;
};

struct Reply replies[REPLICAS];

void delay_ms(long ms) {
    struct timespec t = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };
    nanosleep(&t, NULL);
}

void *replica(void *arg) {
    int id = *(int *)arg;
    struct Account local = {500, 500};

    if (id == 3) {
        // Byzantine replica: deliberately reports a false transition
        local.checking = 300;
        local.savings = 700;
        delay_ms(1);
    } else {
        // Honest replicas execute the requested $100 transfer
        local.checking -= 100;
        local.savings += 100;
        delay_ms(50 + id * 10);
    }

    replies[id].replica_id = id;
    replies[id].state = local;
    clock_gettime(CLOCK_MONOTONIC, &replies[id].arrived);

    return NULL;
}

int arrived_before(struct timespec a, struct timespec b) {
    if (a.tv_sec != b.tv_sec) return a.tv_sec < b.tv_sec;
    return a.tv_nsec < b.tv_nsec;
}

int main(void) {
    pthread_t threads[REPLICAS];
    int ids[REPLICAS];

    for (int i = 0; i < REPLICAS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, replica, &ids[i]);
    }

    for (int i = 0; i < REPLICAS; i++)
        pthread_join(threads[i], NULL);

    // FLAW: trust whichever replica responded first
    int winner = 0;

    for (int i = 1; i < REPLICAS; i++) {
        if (arrived_before(replies[i].arrived, replies[winner].arrived))
            winner = i;
    }

    for (int i = 0; i < REPLICAS; i++) {
        printf("Replica %d: checking=%d savings=%d total=%d\n",
            i, replies[i].state.checking, replies[i].state.savings,
            replies[i].state.checking + replies[i].state.savings);
    }

    struct Account accepted = replies[winner].state;

    printf("\nAccepted replica: %d\n", winner);
    printf("Final checking:  %d\n", accepted.checking);
    printf("Final savings:   %d\n", accepted.savings);

    return 0;
}
