#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define REPLICAS 4
#define BYZANTINE 3
#define QUORUM 3
#define UNVOTED -1

enum { DOUBLE_VOTE, WITHHOLD, OPPOSITE };
int fault_mode = DOUBLE_VOTE;

struct Replica {
    int voted_for;
    pthread_mutex_t lock;
};

struct Proposal {
    int id;
    int amount;
    int approvals;
    int votes[REPLICAS];
};

struct Replica replicas[REPLICAS];

int request_vote(int replica_id, int proposal) {
    if (replica_id == BYZANTINE)
        return fault_mode == DOUBLE_VOTE;
    struct Replica *r = &replicas[replica_id];
    pthread_mutex_lock(&r->lock);
    int observed = r->voted_for;
    int approved = (observed == UNVOTED || observed == proposal);
    if (observed == UNVOTED)
        r->voted_for = proposal;
    pthread_mutex_unlock(&r->lock);
    return approved;
}

void *propose(void *arg) {
    struct Proposal *p = arg;
    for (int i = 0; i < REPLICAS; i++) {
        if (i == BYZANTINE && fault_mode == OPPOSITE)
            p->votes[i] = !p->votes[i - 1];
        else
            p->votes[i] = request_vote(i, p->id);
        p->approvals += p->votes[i];
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc > 2 || (argc == 2 && strcmp(argv[1], "double") && strcmp(argv[1], "withhold") && strcmp(argv[1], "opposite"))) {
        fprintf(stderr, "usage: %s [double|withhold|opposite]\n", argv[0]);
        return 2;
    }
    if (argc == 2)
        fault_mode = !strcmp(argv[1], "withhold") ? WITHHOLD : !strcmp(argv[1], "opposite") ? OPPOSITE : DOUBLE_VOTE;
    pthread_t threads[2];
    struct Proposal a = { .id = 0, .amount = 300 };
    struct Proposal b = { .id = 1, .amount = 400 };
    for (int i = 0; i < REPLICAS; i++) {
        replicas[i].voted_for = UNVOTED;
        pthread_mutex_init(&replicas[i].lock, NULL);
    }
    pthread_create(&threads[0], NULL, propose, &a);
    pthread_create(&threads[1], NULL, propose, &b);
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    for (int i = 0; i < REPLICAS; i++)
        printf("R%d: A=%s B=%s\n", i, a.votes[i] ? "YES" : "NO", b.votes[i] ? "YES" : "NO");
    int commit_a = a.approvals >= QUORUM;
    int commit_b = b.approvals >= QUORUM;
    printf("A: %d votes -> %s\n", a.approvals, commit_a ? "COMMIT" : "REJECT");
    printf("B: %d votes -> %s\n", b.approvals, commit_b ? "COMMIT" : "REJECT");
    for (int i = 0; i < REPLICAS; i++)
        pthread_mutex_destroy(&replicas[i].lock);
    return commit_a && commit_b;
}
