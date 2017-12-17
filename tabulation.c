#include <string.h>
#include "tabulation.h"

typedef struct {
	Table_Index_Hasher hashgen;
	Table_Index_Hasher_Clean hashclean;
	HashTree data;
} Table_Index;

Table_Index *Table_Index_Init( Table_Index_Hasher hasher, Table_Index_Hasher_Clean cleaner ) {
	Table_Index *newti;
	
	newti = malloc( sizeof( Table_Index ) );
	if( newti != NULL ) {
		newti->data = HashTree_Init();
		newti->hashgen = hasher;
		newti->hashclean = cleaner;
	}
	
	return newti;
}

void Table_Index_Hashtree_Foreach_Listing_Free_Wrapper( void *data, const void *hash, size_t hashlen, void *listing ) {
	Listing tmpl = (Listing)listing;
	Listing_Free( &tmpl );
}

void Table_Index_Free( Table_Index **tabind ) {
	HashTree_Foreach( (*tabind)->data, &Table_Index_Hashtree_Foreach_Listing_Free_Wrapper, NULL, 1 );
	HashTree_Free( &((*tabind)->data) );
	free( *tabind );
	*tabind = NULL;
}

void Table_Hashtree_Foreach_Table_Index_Free_Wrapper( void *data, const void *hash, size_t hashlen, void *tableIndex ) {
	Table_Index *tti = (Table_Index *)tableIndex;
	Table_Index_Free( &tti );
}

void Table_Free( Table *tab ) {
	HashTree_Foreach( *tab, &Table_Hashtree_Foreach_Table_Index_Free_Wrapper, NULL, 1 );
	HashTree_Free( tab );
}

/* Add an entry to an index */
void Table_Index_Add( Table_Index *index, void *data ) {
	Listing dl;
	void *dh;
	size_t dhl;
	
	/* Generate the data's hash for the target listing */
	index->hashgen( data, &dh, &dhl );
	
	/* Retrieve the listing of datas with the same hash */
	dl = HashTree_Retrieve( index->data, dh, dhl );
	if( dl == NULL ) {
		/* or create one if it does not exist */
		dl = Listing_Init();
		HashTree_Assign( index->data, dh, dhl, dl );
	}
	
	/* Append the data, if it does not exist */
	if( Listing_IndexOf( dl, data ) < 0 )
		Listing_PushBack( dl, data );
	
	/* Clean the hash */
	index->hashclean( &data );
}

/* Remove an entry from an index */
void Table_Index_Del( Table_Index *index, void *data ) {
	Listing dl;
	void *dh;
	size_t dhl;
	
	/* Generate the data's hash for the target listing */
	index->hashgen( data, &dh, &dhl );
	
	/* Retrieve the listing of datas with the same hash */
	dl = HashTree_Retrieve( index->data, dh, dhl );
	if( dl != NULL ) {
		int di;
		di = Listing_IndexOf( dl, data );
		
		/* Delete entry if it exists */
		if( di >= 0 )
			Listing_Remove( dl, di );
		
		/* Cleanup after deletion */
		if( Listing_Count( dl ) == 0 ) {
			Listing_Free( &dl );
			HashTree_Release( index->data, dh, dhl );
		}
	}
	
	/* Cleanup hash */
	index->hashclean( &dh );
}

/* Create a new index for the table; replaces any old index with the same identifier */
typedef struct {
	int targetIndexNum;
	Table_Index *destinationIndex;
} Table_CreateIndex_HashTree_Foreach_Metadata;

void Table_Index_HashTree_Listing_Foreach_Wrapper( void *destIndex, void *data ) {
	Table_Index_Add( (Table_Index *)destIndex, data );
}

void Table_Index_HashTree_Foreach_Wrapper( void *destIndex, const void *hash, size_t hashlen, void *dataListing ) {
	Listing_Foreach( (Listing)dataListing, &Table_Index_HashTree_Listing_Foreach_Wrapper, destIndex, 1 );
}

void Table_CreateIndex_HashTree_Foreach_Wrapper( void *data, const void *hash, size_t hashlen, void *tableIndex ) {
	Table_CreateIndex_HashTree_Foreach_Metadata *meta = (Table_CreateIndex_HashTree_Foreach_Metadata *)data;
	if( meta->targetIndexNum < 1 ) {
		HashTree_Foreach( meta->destinationIndex->data, &Table_Index_HashTree_Foreach_Wrapper, &meta->destinationIndex, 1 );
		meta->targetIndexNum++;
	}
}

void Table_CreateIndex( Table tab, const char *id, Table_Index_Hasher hash_generator, Table_Index_Hasher_Clean hash_cleaner ) {
	Table_Index *newti, *oldti;
	
	newti = Table_Index_Init( hash_generator, hash_cleaner );
	
	/* Populate our new index with our existing data */
	if( HashTree_Count( tab ) != 0 ) {
		Table_CreateIndex_HashTree_Foreach_Metadata meta;
		meta.targetIndexNum = 0;
		meta.destinationIndex = newti;
		HashTree_Foreach( tab, &Table_CreateIndex_HashTree_Foreach_Wrapper, &meta, 1 );
	}
	
	/* Do not forget the old index with the same name, if any; doing so will cause a memory leak. */
	oldti = HashTree_Retrieve( tab, id, strlen( id ) );
	
	HashTree_Assign( tab, id, strlen( id ), newti );
	
	if( oldti != NULL )
		Table_Index_Free( &oldti );
}

/* Remove an index from the table by id */
void Table_DropIndex( Table tab, const char *indexId ) {
	Table_Index *ti;
	
	ti = HashTree_Retrieve( tab, indexId, strlen( indexId ) );
	HashTree_Release( tab, indexId, strlen( indexId ) );
	
	if( ti != NULL )
		Table_Index_Free( &ti );
}

/* Add an entry to the Table */
void Table_Hashtree_Foreach_Table_Index_Add_Wrapper( void *data, const void *hash, size_t hashlen, void *tableIndex ) {
	Table_Index_Add( tableIndex, data );
}

void Table_Add( Table tab, void *data ) {
	HashTree_Foreach( tab, &Table_Hashtree_Foreach_Table_Index_Add_Wrapper, data, 1 );
}

/* Delete an entry from the Table */
void Table_Hashtree_Foreach_Table_Index_Del_Wrapper( void *data, const void *hash, size_t hashlen, void *tableIndex ) {
	Table_Index_Del( tableIndex, data );
}

void Table_Del( Table tab, void *data ) {
	HashTree_Foreach( tab, &Table_Hashtree_Foreach_Table_Index_Del_Wrapper, data, 1 );
}

/* Get entries from the table that "look like" a given data object by index */
Listing Table_Query( Table tab, const char *index, void *data ) {
	Table_Index *ti;
	void *hash;
	size_t hashlen;
	Listing output;
	
	ti = HashTree_Retrieve( tab, index, strlen( index ) );
	/* if( ti == NULL ) return NULL; */
	
	ti->hashgen( data, &hash, &hashlen );
	output = HashTree_Retrieve( ti->data, hash, hashlen );
	ti->hashclean( &hash );
	
	return output;
}
