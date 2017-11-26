#include "listing.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* swap two values without using extra memory */
#define CUT_SWAP( A, B ) \
	A = A ^ B; \
	B = A ^ B; \
	A = B ^ A;

/* Initialize a Listing object *
 *   Returns NULL on failure   */
Listing Listing_Init() {
	Listing newlisting;
	
	newlisting = malloc( sizeof( Listing_Header ) );
	if( newlisting != NULL ) {
		newlisting->count = 0;
		newlisting->head = NULL;
		newlisting->tail = NULL;
	}
	
	return newlisting;
}

/* Free up the memory used by our listing */
void Listing_Free( Listing *mylisting ) {
	while( (*mylisting)->head != NULL ) {
		Listing_Remove( *mylisting, 0 );
	}
	
	free( *mylisting );
	*mylisting = NULL;
}

/* Sort the cache's index (private maintanance routine) */
void Listing_Cache_Sort( Listing mylisting ) {
	bool sorted;
	do {
		sorted = true;
		for( int cix = 0; cix < LISTING_CACHE_SIZE - 1 && mylisting->cache_index[cix] != NULL && mylisting->cache_index[cix + 1] != NULL; cix++ ) {
			if( *mylisting->cache_index[cix] > *mylisting->cache_index[cix + 1] ) {
				/* Can't use this macro directly due to deficiencies in C relating to bitwise operations on pointers:
				CUT_SWAP( mylisting->cache_index[cix], mylisting->cache_index[cix + 1] );
				 *   But this (almost identicle) code is claimed to be a work-around: */
				mylisting->cache_index[cix] = (unsigned int *) ( (uintptr_t) mylisting->cache_index[cix] ^ (uintptr_t) mylisting->cache_index[cix + 1] );
				mylisting->cache_index[cix + 1] = (unsigned int *) ( (uintptr_t) mylisting->cache_index[cix] ^ (uintptr_t) mylisting->cache_index[cix + 1] );
				mylisting->cache_index[cix] = (unsigned int *) ( (uintptr_t) mylisting->cache_index[cix + 1] ^ (uintptr_t) mylisting->cache_index[cix] );

				sorted = false;
			}	
		}
	} while( !sorted );
}

/* Reset the cache's index (private maintanance routine) */
void Listing_Cache_Reset( Listing mylisting ) {
	/* Hard-reset the cache index */
	for( int cix = 0; cix < LISTING_CACHE_SIZE; cix++ ) 
		if( mylisting->cache[cix] == NULL || mylisting->cacheIDs[cix] <= 0 || mylisting->cacheIDs[cix] >= mylisting->count - 1 )
			mylisting->cache_index[cix] = NULL;
		else
			mylisting->cache_index[cix] = &mylisting->cacheIDs[cix];
	
	/* Move unused cache space to the end of the index */
	for( int cix = 0; cix < LISTING_CACHE_SIZE - 1; cix++ )
		if( mylisting->cache_index[cix] == NULL ) {
			memmove( &mylisting->cache_index[cix], &mylisting->cache_index[cix + 1], sizeof( unsigned int * ) * (LISTING_CACHE_SIZE - cix ) );
			mylisting->cache_index[LISTING_CACHE_SIZE - 1] = NULL;
		}
	
	Listing_Cache_Sort( mylisting );
}

/* Purge an entry from the cache */
void Listing_Cache_Del( Listing mylisting, unsigned int index ) {
	/* Find the cache entry in the index */
	int cis = 0, cie = LISTING_CACHE_SIZE - 1, cim = (LISTING_CACHE_SIZE - 1) / 2;
	while( cis != cim ) {
		if( mylisting->cache_index[cim] == NULL || index < *mylisting->cache_index[cim] ) {
			cie = cim;
		} else {
			cis = cim;
		}
		
		cim = ( cis + cie ) / 2;
	}
	
	/* If entry is in the cache, remove it */
	if( *mylisting->cache_index[cis] == index ) {
		/* Shift the index to keep Null entries at the end */
		int cid = (mylisting->cache_index[cim] - &mylisting->cacheIDs[0]) / sizeof( unsigned int * );
		
		if( cid < LISTING_CACHE_SIZE - 1 ) {
			memmove( &mylisting->cache_index[cim], &mylisting->cache_index[cim + 1], sizeof( unsigned int * ) * (LISTING_CACHE_SIZE - cim - 1) );
			memmove( &mylisting->cacheIDs[cid], &mylisting->cacheIDs[cid + 1], sizeof( unsigned int ) * (LISTING_CACHE_SIZE - cid - 1) );
			memmove( &mylisting->cache[cid], &mylisting->cache[cid + 1], sizeof( Listing_Node * ) * (LISTING_CACHE_SIZE - cid - 1) );
		}
		
		/* Nullify the last entry */
		mylisting->cache[LISTING_CACHE_SIZE - 1] = NULL;
		mylisting->cache_index[LISTING_CACHE_SIZE - 1] = NULL;
	}
}

/* Shift the cache to put the given value in the front (private)*/
void Listing_Cache_Add( Listing mylisting, unsigned int index, Listing_Node *inode ) {
	/* If this index is already cached, remove it */
	Listing_Cache_Del( mylisting, index );
	
	/* Add this entry to the front of the list*/
	memmove( &mylisting->cacheIDs[1], &mylisting->cacheIDs[0], sizeof( unsigned int ) * (LISTING_CACHE_SIZE - 1) );
	memmove( &mylisting->cache[1], &mylisting->cache[0], sizeof( Listing_Node * ) * (LISTING_CACHE_SIZE - 1) );
	mylisting->cacheIDs[0] = index;
	mylisting->cache[0] = inode;
	
	/* Ensure the cacheID is indexed */
	if( mylisting->cache_index[LISTING_CACHE_SIZE - 1] == NULL ) {
		memmove( &mylisting->cache_index[1], &mylisting->cache_index[0], sizeof( unsigned int * ) * (LISTING_CACHE_SIZE - 1) );
		mylisting->cache_index[0] = &mylisting->cacheIDs[0];
	}
	
	/* Perform maintainance on the cache index */
	Listing_Cache_Sort( mylisting );
}

/* Sort a list according to a user-defined routine; 
 *   arrangement of list is such that callback always returns true where itemA and itemB are adjacent. */
void Listing_Sort( Listing mylisting, bool (*callback)(void * /* itemA */, void * /* itemB */) ) {
	Listing_Node *current, *end;
	bool sorted;
	unsigned int cid;
	
	end = mylisting->tail;
	
	do {
		sorted = true;
		current = mylisting->head; cid = 0;
		while( current != end ) {
			if( !callback( current->data, current->next->data ) ) {
				/* Incriment data position */
				Listing_Node *prev, *next, *node1, *node2;
				prev = current->prev;
				next = current->next->next;
				node1 = current;
				node2 = current->next;
				
				node2->prev = prev;
				node1->next = next;
				node2->next = node1;
				node1->prev = node2;
				prev->next = node2;
				next->prev = node1;
				
				if( node1 == mylisting->head ) {
					mylisting->head = node2;
				}
				
				if( node2 == mylisting->tail ) {
					mylisting->tail = node1;
				}
				
				/* Update cache */
				for( int cix = 0; cix < LISTING_CACHE_SIZE; cix++ )
					if( mylisting->cacheIDs[cix] == cid )
						mylisting->cacheIDs[cix]++;
				
				/* Signal changed */
				sorted = false;
			} else {
				/* Goto next data point */
				current = current->next;
			}
			cid++;
		}
	} while( !sorted );
	
	Listing_Cache_Sort( mylisting );
}

/* Use the cache to find a node in the lisitng 
 *   (private routine for DRYing the code)
 */
Listing_Node *Listing_Node_Select( Listing mylisting, unsigned int index ) {
	/* Find the closest starting point in the cache */
	int csx = 0, cex = LISTING_CACHE_SIZE - 1, cmx = (LISTING_CACHE_SIZE - 1) / 2 ;
	while( csx != cmx ) {
		if( mylisting->cache_index[cmx] == NULL || index <= *mylisting->cache_index[cmx] ) {
			cex = cmx;
		} else {
			csx = cmx;
		}
		
		cmx = (csx + cex) / 2;
	}
	
	Listing_Node *current;
	unsigned int tx;
	if( index < *mylisting->cache_index[cmx] / 2 ) {
		/* The target index is closer to the begining than any point in the cache */
		tx = 0;
		current = mylisting->head;
	} else if( index > (mylisting->count - 1 - *mylisting->cache_index[cmx]) / 2 ) {
		/* The taerget is closer to the end than to any point in the cache */
		tx = mylisting->count - 1;
		current = mylisting->tail;
	} else {
		tx = *mylisting->cache_index[cmx];
		current = mylisting->cache[ (mylisting->cache_index[cmx] - mylisting->cacheIDs) / sizeof( unsigned int ) ];
	}
	
	/* Traverse to target index
	 * NOTE:
	 *   Ultimately we must linearly traverse a linked list to get to a target index,
	 *   however by caching recently used indexs we can cut the time required in 
	 *   traversing lenthy linked-lists by finding the closest starting poiint for traversal
	 *   between the known locations in the cache and the begining an end of the list.
	 */
	while( index != tx ) {
		if( index > tx ) {
			current = current->next;
			tx++;
		} else {
			current = current->prev;
			tx--;
		}
	}
	
	return current;
}

/* Get data from listing */
void *Listing_At( Listing mylisting, unsigned int index ) {
	/* Short-Circuit: Index out of tange */
	if( index < 0 || index >= mylisting->count ) {
		return NULL;
	} 
	
	Listing_Node *tmp;
	tmp = Listing_Node_Select( mylisting, index );
	
	Listing_Cache_Add( mylisting, index, tmp );
	return tmp->data;
}

/* Add data to the listing */
void Listing_Insert( Listing mylisting, unsigned int index, void *data ) {
	Listing_Node *newnode;
	
	if( index < 0 || index > mylisting->count ) return;
	
	newnode = malloc( sizeof( Listing_Node ) );
	if( newnode == NULL ) return;
	
	newnode->next = NULL;
	newnode->prev = NULL;
	newnode->data = data;
	
	if( index == 0 ) {
		/* Insert new node at begining of list */
		if( mylisting->head == NULL ) {
			mylisting->tail == newnode;
			mylisting->head == newnode;
		} else {
			newnode->next = mylisting->head;
			mylisting->head->prev = newnode;
			mylisting->head = newnode;
		}
	} else if( index == mylisting->count ) {
		/* Insert new node at end of list */
		newnode->prev = mylisting->tail;
		mylisting->tail->next = newnode;
		mylisting->tail = newnode;
	} else {
		/* Insert new node in middle of list */
		Listing_Node *current;
		current = Listing_Node_Select( mylisting, index );
		
		newnode->next = current->next;
		current->next->prev = newnode;
		newnode->prev = current;
		current->next = newnode;
	}
	
	/* Update count and cached indexes */
	mylisting->count++;
	for( int ix = 0; ix < LISTING_CACHE_SIZE; ix++ )
		if( mylisting->cacheIDs[ix] >= index )
			mylisting->cacheIDs[ix]++;
	Listing_Cache_Add( mylisting, index, newnode );
	/* Calling this routine is redundent with the call to Listing_Cache_Add:
	Listing_Cache_Sort( mylisting );
	*/
}

/* Remove data from the listing */
void Listing_Remove( Listing mylisting, unsigned int index ) {
	Listing_Node *current;
	
	if( index < 0 || index >= mylisting->count ) return;
	
	/* Find the Node */
	if( index == 0 ) {
		current = mylisting->head;
	} else if( index == mylisting->count - 1 ) {
		current == mylisting->tail;
	} else {
		current = Listing_Node_Select( mylisting, index );
	}
	
	/* Disconnect the Node */
	if( current->next != NULL )
		current->next->prev = current->prev;
	
	if( current->prev != NULL )
		current->prev->next = current->next;
	
	if( mylisting->head == current )
		mylisting->head = current->next;
	
	if( mylisting->tail == current )
		mylisting->tail = current->prev;
	
	/* Free the Node and update the cache */
	Listing_Cache_Del( mylisting, index );
	free( current );
	mylisting->count--;
	for( int ix = 0; ix < LISTING_CACHE_SIZE; ix++ )
		if( mylisting->cacheIDs[ix] > index )
			mylisting->cacheIDs[ix]--;
}
