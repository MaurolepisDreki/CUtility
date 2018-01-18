/* Batch Processing System */

#ifndef INCLUDED_BATCH_H
#define INCLUDED_BATCH_H

#include "listing.h"

typedef void (*batfunc_t)(void * /* specific static data */, void * /* common dynamic data */);
typedef struct {
	batfunc_t batcb;
	void *sdata;
} Batch_Node;
typedef Listing Batch;

/* Constructor & Destructor
 *   Constructor simply initializes the list.
 *   Destructor has to release the Batch_Nodes.
 */
#define Batch_Init() Listing_Init()
void Batch_Free( Batch * );

/* Add Node to Batch; Nodes are unique by routine and data addresses */
void Batch_Add( Batch, batfunc_t, void * );

/* Remove all Nodes from Batch matching specific patterns */
void Batch_Del( Batch, batfunc_t, void * );
void Batch_Del_byFunc( Batch, batfunc_t );
void Batch_Del_byData( Batch, void * );

/* Execute Batch */
void Batch_Run( Batch, void *cdata, int threads );

#endif /* INCLUDE_BATCH_H */
