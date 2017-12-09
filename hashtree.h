/* AUTHOR:   Nile Aagard
 * CREATED:  2017-10-21
 * MODIFIED: 2017-10-21
 * DESCRIPTION: A Hashtree Structure Implimentation;
 *   A Hashtree is a key-value pair structure for storing and retriving data
 *    where the key and the value can be of any type, like a primitive form of C++'s std::map.
 */

#include <stdlib.h>

#ifndef INCLUDED_HASHTREE_H
#define INCLUDED_HASHTREE_H

typedef struct HashTree_Node {
	/* Branch Identifier */
	unsigned char bid;
	
	/* Subnode Listing */
	unsigned char subnode_count;
	struct HashTree_Node **subnode;
	
	/* Data Attachment */
	void *data;
} HashTree_Node;

typedef HashTree_Node *HashTree;

/* Constructor */
HashTree HashTree_Init();

/* Descrtuctor */
void HashTree_Free( HashTree *ht );

/* Data (Re)Assignment */
void HashTree_Assign( HashTree ht, void *hash, size_t hash_len, void *data );

/* Data Retreival */
void *HashTree_Retrieve( HashTree ht, void *hash, size_t hash_len );

/* Data Release (Unassignment) */
void HashTree_Release( HashTree ht, void *hash, size_t hash_len );

/* Foreach Entry */
void HashTree_Foreach( HashTree ht, void (*callback)(void * /* data */, void * /* hash */, size_t /* hash_len */, void * /* entry */), void *data, int threads );

#endif
