#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

atomic_int slot = 0;



void *worker(void *arg){
    int id = *(int *)arg;
    int expected = 0;
	spinner();
    if (atomic_compare_exchange_strong( &slot, &expected, id)) { 
		printf("Thread %d CLAIMED the slot\n", id); 
	} else {
        printf("Thread %d LOST. slot was already %d\n", id, expected);
    }

    return NULL;
}


void *spinner(void *arg){
	// literally just a way to introduce jitter in execution time 
	srand(time(NULL));
	int random_jitter = 100*rand();
	for (int i = 0; i < 1000*random_jitter; i++){
		int j = i*random_jitter; // just useless computation to help expand the jitter
	}
	return NULL; 
}



int main(void){
    pthread_t a, b;

    int id_a = 1;
    int id_b = 2;

    pthread_create(&a, NULL, worker, &id_a);
    pthread_create(&b, NULL, worker, &id_b);

    pthread_join(a, NULL);
    pthread_join(b, NULL);

    printf("Final slot owner: %d\n", slot);

    return 0;
}