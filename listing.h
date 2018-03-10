/* AUTHOR: Nile Aagard
 * CREATED: 2017-11-23
 * MODIFIED: 2017-11-23
 * DESCRIPTION: A List structural extension for C with cache-based optimization
 */

#include <stdbool.h>

#ifndef INCLUDED_LISTING_H
#define INCLUDED_LISTING_H

#define LISTING_CACHE_SIZE 10

/* Node structure for a linked list */
typedef struct Listing_Node {
	struct Listing_Node *prev, *next;
	void *data;
} Listing_Node;

/* Header structure for cached linked list */
typedef struct {
	unsigned int count, cacheIDs[LISTING_CACHE_SIZE], *cache_index[LISTING_CACHE_SIZE];
	Listing_Node *head, *tail, *cache[LISTING_CACHE_SIZE];
} Listing_Header;

/* A type to make some C pointer concepts transparent to the user */
typedef Listing_Header *Listing;

/* Constructor and Destructor routines */
Listing Listing_Init();
void Listing_Free( Listing * );

/* Modifier Routines */
void Listing_Insert( Listing mylisting, unsigned int index, void *data );
void Listing_Remove( Listing mylisting, unsigned int index );

/* Access Routine */
void *Listing_At( Listing mylisting, unsigned int index );

/* Misc helper routines */
void Listing_Foreach( Listing mylisting, void (*callback)(void * /* data */, void * /* item */), void *data, int threads );
void Listing_Sort( Listing mylisting, bool (*callback)(void * /* itemA */, void * /* itemB */) );

unsigned int Listing_Count( Listing mylisting );
bool Listing_isEmpty( Listing mylisting );
void Listing_Merge( Listing destination, Listing origin ); /* Moves items in origin to destination */
void Listing_Clone( Listing destination, Listing origin ); /* Copies items in origin to destination */
Listing Listing_Find( Listing mylisting, bool (*test)( void * /* user data */, void * /* listing entry */ ), void *userData, int threads ); /* Returns a new list of items where test returns true */
int Listing_IndexOf( Listing mylisting, void *data ); /* returns -1 if not found */

/* Macro wrappers to simplify common operations without additional abuse of the call-stack */
#define Listing_PushFront( list, entry ) Listing_Insert( list, 0, entry )
#define Listing_PushBack( list, entry ) Listing_Insert( list, Listing_Count( list ), entry )
#define Listing_PopFront( list ) Listing_Remove( list, 0 )
#define Listing_PopBack( list ) Listing_Remove( list, Listing_Count( list ) - 1 )
#define Listing_AtFront( list ) Listing_At( list, 0 )
#define Listing_AtBack( list ) Listing_At( list, Listing_Count( list ) - 1 )

#endif 
