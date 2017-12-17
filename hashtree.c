#include "hashtree.h"
#include <string.h>

/* Create a root node for the tree */
HashTree HashTree_Init() {
	HashTree newht;
	
	newht = malloc( sizeof( HashTree_Node ) );
	if( newht != NULL ) {
		newht->bid = 0x00;
		newht->subnode_count = 0;
		newht->subnode = NULL;
		newht->data = NULL;
	}
	
	return newht;
}

/* Recursivly free nodes in a tree */
void HashTree_Free( HashTree *ht ) {
	for( int ix = 0; ix < (*ht)->subnode_count; ix++ )
		HashTree_Free( (*ht)->subnode + ix * sizeof( HashTree_Node * ) );
	
	if( (*ht)->subnode != NULL )
		free( (*ht)->subnode );
	
	free( *ht );
	*ht = NULL;
}

/* Assign data to the tree recursivly adding nodes as needed */
void HashTree_Assign( HashTree ht,  const void *hash, size_t hash_len, void *data ) {
	int index;
	
	/* Assign data if there are no more bytes in the hash */
	if( hash_len == 0 ) {
		ht->data = data;
		return;
	}
	
	/* Create new subnode list if empty */
	if( ht->subnode == NULL ) {
		ht->subnode = malloc( sizeof( HashTree_Node * ) );
		
		if( ht->subnode == NULL )
			return /* error */;
		
		ht->subnode_count = 1;
		ht->subnode[0] = NULL;
		index = 0;
	} else {
		/* find the index of the next node */
		int start, end, mid;
		start = 0;
		end = ht->subnode_count - 1;
		do {
			mid = (start + end) / 2;
			if( ht->subnode[mid]->bid > ((unsigned char *)(hash))[0] ) {
				end = mid;
			} else if ( ht->subnode[mid]->bid < ((unsigned char *)(hash))[0] ) {
				start = mid; 
			}
		} while( start != end || ht->subnode[mid]->bid != ((unsigned char *)(hash))[0] );
		
		/* open a space for the subnode if one does not exist */
		if( ht->subnode[mid]->bid != ((unsigned char *)(hash))[0] ) {
			HashTree_Node **tmp;
			tmp = malloc( sizeof( HashTree_Node * ) * ht->subnode_count + 1 );
			if( tmp == NULL )
				return /* error */;
			memcpy( tmp, ht->subnode, sizeof( HashTree_Node * ) * (mid - 1) );
			memcpy( tmp + mid + 1, ht->subnode + mid, sizeof( HashTree_Node * ) * (ht->subnode_count - mid - 1) );
			tmp[mid] = NULL;
			free( ht->subnode );
			ht->subnode = tmp;
		}
		
		index = mid;
	}
	
	/* Create new subnode if not exists */
	if( ht->subnode[index] == NULL ) {
		ht->subnode[index] = malloc( sizeof( HashTree_Node ) );
		if( ht->subnode[index] == NULL )
			return /* error */;
		
		ht->subnode[index]->bid = ((unsigned char *)(hash))[0];
		ht->subnode[index]->subnode_count = 0;
		ht->subnode[index]->subnode = NULL;
		ht->subnode[index]->data = NULL;
	}
	
	HashTree_Assign( ht->subnode[index], hash + 1, hash_len - 1, data );
}

/* Recursivly find stored data by it's hash */
void *HashTree_Retrive( HashTree ht, const void *hash, size_t hash_len ) {
	int start, end, mid;
	
	if( hash_len == 0 ) {
		return ht->data;
	}
	
	start = 0;
	end = ht->subnode_count - 1;
	do {
		mid = (start + end) / 2;
		if( ht->subnode[mid]->bid > ((unsigned char *)(hash))[0] ) {
			end = mid;
		} else if ( ht->subnode[mid]->bid < ((unsigned char *)(hash))[0] ) {
			start = mid; 
		}
	} while( start != end || ht->subnode[mid]->bid != ((unsigned char *)(hash))[0] );
	
	if( ht->subnode[mid]->bid != ((unsigned char *)(hash))[0] ) {
		return NULL;
	} else {
		return HashTree_Retrive( ht->subnode[mid], hash + 1, hash_len - 1 );
	}
}

/* Discard data from the tree, recursivly clearing unused nodes */
void HashTree_Release( HashTree ht, const void *hash, size_t hash_len ) {
	int start, end, mid, new_subnode_count;
	HashTree_Node **new_subnode;
	
	/* Find the target data and release it from our memory    *
	 *   It is up to the user to free the actual data stored. */
	if( hash_len == 0 ) {
		ht->data = NULL;
		return;
	}
	
	start = 0;
	end = ht->subnode_count - 1;
	do {
		mid = (start + end) / 2;
		if( ht->subnode[mid]->bid > ((unsigned char *)(hash))[0] ) {
			end = mid;
		} else if ( ht->subnode[mid]->bid < ((unsigned char *)(hash))[0] ) {
			start = mid; 
		}
	} while( start != end || ht->subnode[mid]->bid != ((unsigned char *)(hash))[0] );
	
	if( ht->subnode[mid]->bid != ((unsigned char *)(hash))[0] ) {
		return;
	} else {
		HashTree_Retrive( ht->subnode[mid], hash + 1, hash_len - 1 );
	}
	
	/* Clear unused nodes in this branch */
	new_subnode_count = 0;
	for( int index = 0; index < ht->subnode_count; index++ ) {
		if( ht->subnode[index]->data == NULL && ht->subnode[index]->subnode == NULL ) {
			free( ht->subnode[index] );
			ht->subnode[index] = NULL;
		} else {
			ht->subnode[new_subnode_count++] = ht->subnode[index];
		}
	}
	
	if( new_subnode_count != 0 ) {
		HashTree_Node **new_subnode;
		new_subnode = malloc( sizeof( HashTree_Node * ) * new_subnode_count );
		if( new_subnode != NULL ) {
			memcpy( new_subnode, ht->subnode, sizeof( HashTree_Node * ) * new_subnode_count );
			free( ht->subnode );
			ht->subnode = new_subnode;
		}
	} else {
		free( ht->subnode );
		ht->subnode = NULL;
	}
	
	ht->subnode_count = new_subnode_count;
}

/* Count HashTree Entries */
void HashTree_Count_Foreach_Callback( void *count, const void *hash, size_t hlen, void *entry ) {
	(*((size_t *)count))++;
}

size_t HashTree_Count( HashTree ht ) {
	size_t result = 0;
	HashTree_Foreach( ht, &HashTree_Count_Foreach_Callback, &result, 1 );
	return result;
}
