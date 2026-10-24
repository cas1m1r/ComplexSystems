
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <semaphore.h>

#define REPLICAS 4
#define BYZANTINE 3
#define QUORUM 3
#define UNVOTED -1

struct Replica {
    atomic_int voted_for;
    pthread_barrier_t barrier;
};

struct Proposal {
    int id;
    int amount;
    int approvals;
    int votes[REPLICAS];
};

struct Replica replicas[REPLICAS];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int request_vote(int replica_id, int proposal) {
	
	// try to lock returns with 
    if (replica_id == BYZANTINE)
        return 1; // Faulty replica approves both proposals

    struct Replica *r = &replicas[replica_id];

    int observed = atomic_load(&r->voted_for);

    // Force both proposers to reach this point
    // pthread_barrier_wait(&r->barrier);
	printf("Inside Proposal: %d, Replica: %d\n",proposal,replica_id);
    if (observed == UNVOTED) {
        atomic_store(&r->voted_for, proposal);
        return 1;
    }

    return observed == proposal;
}

void *propose(void *arg) {
    struct Proposal *p = arg;

    for (int i = 0; i < REPLICAS; i++) {
		pthread_mutex_lock(&lock);
		p->votes[i] = request_vote(i, p->id);
		
        if (p->votes[i])
            p->approvals++;
		
		pthread_mutex_unlock(&lock);
	}
	
    return NULL;
}

int main(void) {
    pthread_t threads[2];

    struct Proposal a = { .id = 0, .amount = 300 };
    struct Proposal b = { .id = 1, .amount = 400 };

    for (int i = 0; i < REPLICAS; i++) {
        atomic_init(&replicas[i].voted_for, UNVOTED);
		
        if (i != BYZANTINE)
            pthread_barrier_init(&replicas[i].barrier, NULL, 2);
    }
	
	
    pthread_create(&threads[0], NULL, propose, &a);
    pthread_create(&threads[1], NULL, propose, &b);

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
	
	
	
    printf("\n--- REPLICA VOTES ---\n");

    for (int i = 0; i < REPLICAS; i++) {
        printf("R%d: A=%s B=%s\n", i,
            a.votes[i] ? "YES" : "NO",
            b.votes[i] ? "YES" : "NO");
    }

    int commit_a = a.approvals >= QUORUM;
    int commit_b = b.approvals >= QUORUM;

    printf("\n--- CONSENSUS RESULTS ---\n");
    printf("A: %d votes -> %s\n", a.approvals, commit_a ? "COMMIT" : "REJECT");
    printf("B: %d votes -> %s\n", b.approvals, commit_b ? "COMMIT" : "REJECT");

    if (commit_a && commit_b) {
        printf("\nSAFETY FAILURE: conflicting transactions finalized!\n");
        printf("Balance if both honored: $%d\n", 500 - a.amount - b.amount);
    }

	
    for (int i = 0; i < REPLICAS; i++) {
        if (i != BYZANTINE)
            pthread_barrier_destroy(&replicas[i].barrier);
    }

    return 0;
}
//gcc -O2 -pthread bft_flawed.c -o bft_flawed
