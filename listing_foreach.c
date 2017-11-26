/* Due to the complexity of threading, Listing_Foreach has it's own file and privately associated routines */
#include "listing.h"
#include <pthread.h>

typedef struct {
	Listing_Node *current;
	pthread_mutex_t mutex;
} Listing_Foreach_Queue;

void Listing_Foreach_Queue_Setup( Listing_Foreach_Queue *lfq, Listing mylisting ) {
	lfq->current = mylisting->head;
	pthread_mutex_init( &lfq->mutex, NULL );
}

void Listing_Foreach_Queue_Teardown( Listing_Foreach_Queue *lfq ) {
	pthread_mutex_destroy( &lfq->mutex );
}

Listing_Node *Listing_Foreach_Queue_Next( Listing_Foreach_Queue *lfq ) {
	Listing_Node *next;

	/* only one thread may have lfq->current, 
	 *   the rest must wait their turn */
	pthread_mutex_lock( &lfq->mutex );
	next = lfq->current;
	if( lfq->current != NULL ) {
		lfq->current == lfq->current->next;
	}
	pthread_mutex_unlock( &lfq->mutex );

	return next;
}

typedef struct {
	Listing_Foreach_Queue lfq;
	void (*cb)(void *, void *);
	void *cb_data;
} Listing_Foreach_JobMeta;

void Listing_Foreach_JobMeta_Setup( Listing_Foreach_JobMeta *lfjm, Listing mylisting, void (*cb)(void *, void *), void *cb_data ) {
	Listing_Foreach_Queue_Setup( &lfjm->lfq, mylisting );
	
	lfjm->cb = cb;
	lfjm->cb_data = cb_data;
}

void Listing_Foreach_JobMeta_Teardown( Listing_Foreach_JobMeta *lfjm ) {
	Listing_Foreach_Queue_Teardown( &lfjm->lfq );
}

void *Listing_Foreach_Worker( void *data ) {
	/* Receives Listing_Foreach_JobMeta *, uses void * for pthread's sake */
	Listing_Foreach_JobMeta *lfjm = (Listing_Foreach_JobMeta *) data;
	Listing_Node *mynode;

	do {
		mynode = Listing_Foreach_Queue_Next( &lfjm->lfq );

		if( mynode != NULL ) {
			lfjm->cb( lfjm->cb_data, mynode->data );
		}
	} while( mynode != NULL );

	return NULL;
}

/* Execute an operation (callback) on every item in the listing */
void Listing_Foreach( Listing mylisting, void (*callback)(void * /* data */, void * /* item */), void *data, int threads ) {
	pthread_t thread_list[threads - 1];
	void *result;

	Listing_Foreach_JobMeta lfjm;
	Listing_Foreach_JobMeta_Setup( &lfjm, mylisting, callback, data );

	/* Spin up workers with this thread as the last worker */
	for( int ix = 0; ix < threads - 1; ix++ ) {
		pthread_create( &thread_list[ix], NULL, &Listing_Foreach_Worker, &lfjm );
	}
	result = Listing_Foreach_Worker( &lfjm );

	/* and make sure to join them before continuing */
	for( int ix = 0; ix < threads -1; ix++ ) {
		pthread_join( thread_list[ix], result );
	}

	Listing_Foreach_JobMeta_Teardown( &lfjm );
}

