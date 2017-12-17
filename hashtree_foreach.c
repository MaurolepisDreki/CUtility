/* implimentation of foreach on the hashtree using POSIX threads */

#include "hashtree.h"
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

typedef struct HashTree_Foreach_Stack_Node {
	HashTree_Node *current_node;
	unsigned char last_subnode;
	struct HashTree_Foreach_Stack_Node *parrent;
} HashTree_Foreach_Stack_Node;

typedef struct {
	bool started;
	HashTree_Foreach_Stack_Node *htStack;
	pthread_mutex_t mutex;
} HashTree_Foreach_Queue;

typedef struct {
	HashTree_Foreach_Queue htQueue;
	void (*cb)(void *, const void *, size_t, void *);
	void *cb_data;
} HashTree_Foreach_JobMeta;

void HashTree_Foreach_Stack_Push( HashTree_Foreach_Stack_Node **stack, HashTree_Node *htn ) {
	HashTree_Foreach_Stack_Node *tmp;
	
	/* Create Node */
	tmp = malloc( sizeof( HashTree_Foreach_Stack_Node ) );
	/* Would perform this check, but need the program to crash in the next step
		for want of a better "out of memory" notification:
	if( stack == NULL ) return stack; */
	
	tmp->current_node = htn;
	tmp->last_subnode = 0;
	tmp->parrent = *stack;
	
	*stack = tmp;
}

void HashTree_Foreach_Stack_Pop( HashTree_Foreach_Stack_Node **stack ) {
	HashTree_Foreach_Stack_Node *tmp;
	
	tmp = *stack;
	*stack = tmp->parrent;
	free( tmp );
}

int HashTree_Foreach_Stack_Count( HashTree_Foreach_Stack_Node *stack ) {
	int count = 0;
	HashTree_Foreach_Stack_Node *cur = stack;

	while( cur != NULL ) {
		cur = cur->parrent;
		count++;
	}

	return count;
}

void HashTree_Foreach_Queue_Setup( HashTree_Foreach_Queue *htfq, HashTree ht ) {
	htfq->htStack = NULL;
	htfq->started = false;
	HashTree_Foreach_Stack_Push( &htfq->htStack, ht );
	pthread_mutex_init( &htfq->mutex, NULL );
}

void HashTree_Foreach_Queue_Teardown( HashTree_Foreach_Queue *htfq ) {
	while( htfq->htStack != NULL ) {
		HashTree_Foreach_Stack_Pop( &htfq->htStack );
	}
	pthread_mutex_destroy( &htfq->mutex );
}

void HashTree_Foreach_JobMeta_Setup( HashTree_Foreach_JobMeta *htfjm, HashTree ht, void (*cb)(void *, const void*, size_t, void *), void *cb_data ) {
	htfjm->cb = cb;
	htfjm->cb_data = cb_data;
	HashTree_Foreach_Queue_Setup( &htfjm->htQueue, ht );
}

void HashTree_Foreach_JobMeta_Teardown( HashTree_Foreach_JobMeta *htfjm ) {
	HashTree_Foreach_Queue_Teardown( &htfjm->htQueue );
}

/* WARNING: failing to free( hash ) when you are done with it will result in a memory leak! */
void HashTree_Foreach_Queue_Next( HashTree_Foreach_Queue *htfq, void **hash,  int *hashlen, void **data) {
	pthread_mutex_lock( &htfq->mutex );
	
	/* Goto next/first entry in the stack */
	do {
		if( htfq->started ) {
			HashTree_Foreach_Stack_Pop( &htfq->htStack );
			if(htfq->htStack->last_subnode < htfq->htStack->current_node->subnode_count - 1 ) {
				htfq->htStack->last_subnode++;
				while( htfq->htStack->current_node->subnode_count != 0 ) {
					HashTree_Foreach_Stack_Push( &htfq->htStack, htfq->htStack->current_node->subnode[ htfq->htStack->last_subnode ] );
				}
			} 
		} else {
			while( htfq->htStack->current_node->subnode_count != 0 ) {
				HashTree_Foreach_Stack_Push( &htfq->htStack, htfq->htStack->current_node->subnode[ htfq->htStack->last_subnode ] );
			}
			htfq->started = true;
		}
		
		/* check for work (validate stack); short-circuit if no work (invalid stack) */
		if( htfq->htStack == NULL ) {
			pthread_mutex_unlock( &htfq->mutex );
			*hash = NULL;
			*data = NULL;
			*hashlen = 0;
			return;
		}
	} while( htfq->htStack->current_node->data != NULL );
	
	/* Get data and hash */
	*hashlen = HashTree_Foreach_Stack_Count( htfq->htStack );
	*hash = malloc( sizeof( unsigned char ) * *hashlen );
	
	HashTree_Foreach_Stack_Node *tmp = htfq->htStack;
	int stackItr = *hashlen - 1;
	while( tmp != NULL ) {
		memcpy( *hash + sizeof( unsigned char ) * stackItr, &htfq->htStack, sizeof( unsigned char ) );
		tmp = tmp->parrent;
		stackItr--;
	}
	
	*data = htfq->htStack->current_node->data;
	
	pthread_mutex_unlock( &htfq->mutex );
}

void *HashTree_Foreach_Worker( void *data ) {
	HashTree_Foreach_JobMeta *htfjm = (HashTree_Foreach_JobMeta *)data;
	void *htHash, *htData;
	int htHashLen;
	
	do {
		HashTree_Foreach_Queue_Next( &htfjm->htQueue, &htHash, &htHashLen, &htData );
		
		/* Work as long as there is work to be done! */
		if( htHash != NULL ) {
			htfjm->cb( htfjm->cb_data, htHash, htHashLen, htData );
			free( htHash );
			htHash = (void *)0b1;
		} else {
			htHash = (void *)0b0;
		}
	} while( htHash == (void *)0b1 );
}

void HashTree_Foreach( HashTree ht, void (*callback)(void * /* data */, const void * /* hash */, size_t /* hash_len */, void * /* entry */), void *data, int threads ) {
	pthread_t thread_list[threads - 1];
	void *result;
	
	HashTree_Foreach_JobMeta htfjm;
	HashTree_Foreach_JobMeta_Setup( &htfjm, ht, callback, data );
	
	/* Spin up workers with this thread as the last worker */
	for( int ix = 0; ix < threads - 1; ix++ ) {
		pthread_create( &thread_list[ix], NULL, &HashTree_Foreach_Worker, &htfjm );
	}
	result = HashTree_Foreach_Worker( &htfjm );
	
	/* make sure all workers die before continuing */
	for( int ix = 0; ix < threads - 1; ix++ ) {
		pthread_join( thread_list[ix], result );
	}
	
	HashTree_Foreach_JobMeta_Teardown( &htfjm );
}
