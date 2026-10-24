#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#define REPLICAS 4
#define QUORUM 3
#define UNVOTED -1

struct Replica {
    int voted_for;
    pthread_mutex_t lock;
} replicas[REPLICAS];

struct Proposal {
    int id;
    int votes[REPLICAS];
    int approvals;
};

pthread_barrier_t step_gate;

int request_vote(int replica, int proposal) {
    if (replica == 3) return 0; // One faulty participant withholds its vote
    struct Replica *r = &replicas[replica];
    pthread_mutex_lock(&r->lock);
    int approved = (r->voted_for == UNVOTED || r->voted_for == proposal);
    if (r->voted_for == UNVOTED) r->voted_for = proposal;
    pthread_mutex_unlock(&r->lock);
    return approved;
}

void *propose(void *arg) {
    struct Proposal *p = arg;
    const int orders[2][3] = {{0, 1, 2}, {1, 0, 2}};
    for (int step = 0; step < 3; step++) {
        int replica = orders[p->id][step];
        p->votes[replica] = request_vote(replica, p->id);
        p->approvals += p->votes[replica];
        pthread_barrier_wait(&step_gate); // Same stage gate; no replica lock held
    }
    return NULL;
}

int main(void) {
    struct Proposal a = {.id = 0}, b = {.id = 1};
    pthread_t ta, tb;
    pthread_barrier_init(&step_gate, NULL, 2);
    for (int i = 0; i < REPLICAS; i++) {
        replicas[i].voted_for = UNVOTED;
        pthread_mutex_init(&replicas[i].lock, NULL);
    }
    pthread_create(&ta, NULL, propose, &a);
    pthread_create(&tb, NULL, propose, &b);
    pthread_join(ta, NULL);
    pthread_join(tb, NULL);
    for (int i = 0; i < REPLICAS; i++)
        printf("R%d: A=%d B=%d\n", i, a.votes[i], b.votes[i]);
    printf("A=%d B=%d quorum=%d\n", a.approvals, b.approvals, QUORUM);
    assert(a.approvals < QUORUM && b.approvals < QUORUM);
    assert(a.approvals + b.approvals == 3);
    assert(a.votes[0] == 1 && b.votes[1] == 1);
    for (int i = 0; i < 3; i++) assert(a.votes[i] + b.votes[i] == 1);
    for (int i = 0; i < REPLICAS; i++) pthread_mutex_destroy(&replicas[i].lock);
    pthread_barrier_destroy(&step_gate);
    return 0;
}
