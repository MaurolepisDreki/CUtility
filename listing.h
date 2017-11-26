/* AUTHOR: Nile Aagard
 * CREATED: 2017-11-23
 * MODIFIED: 2017-11-23
 * DESCRIPTION: A List structural extention for C with cache-based optomization
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

#endif 
