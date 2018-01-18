#include "batch.h"
#include <sys/sysinfo.h>
#include "util.h"

/* Destructor Routine */
void Batch_Free_Foreach_cb( void *unused, void *node ) {
    free( node );
}

void Batch_Free( Batch *bat ) {
    Listing_Foreach( *bat, &Batch_Free_Foreach_cb, NULL, get_nprocs() );
    Listing_Free( bat );
}

/* Add Routine to Batch */
void Batch_Add( Batch bat, batfunc_t func, void * data ) {
    Batch_Node *tmp;
    fmalloc(tmp, sizeof( Batch_Node ));

    tmp->batcb = func;
    tmp->sdata = data;

    Listing_PushFront( bat, tmp );
}

/* Delete Routine form Batch */
bool Batch_Del_Find_cb( void *target, void *test ) {
    return ((Batch_Node *)target)->batcb == ((Batch_Node *)test)->batcb && ((Batch_Node *)target)->sdata == ((Batch_Node *)test)->sdata;
}

void Batch_Del_Foreach_cb( void *list, void *item ) {
    int ix;

    ix = Listing_IndexOf( (Listing)list, item );
    Listing_Remove( (Listing)list, ix );
}

void Batch_Del( Batch bat, batfunc_t func, void *data ){
    Batch_Node tmp;
    Listing matches;

    tmp.batcb = func;
    tmp.sdata = data;
    matches = Listing_Find( bat, &Batch_Del_Find_cb, &tmp, get_nprocs() );
    Listing_Foreach( matches, &Batch_Del_Foreach_cb, bat, 1 );

    Listing_Free( &matches );
}

bool Batch_Del_byData_Find_cb( void *data, void *test ) {
    return ((Batch_Node *)test)->sdata == data;
}

void Batch_Del_byData( Batch bat, void *data ) {
    Listing matches;

    matches = Listing_Find( bat, &Batch_Del_byData_Find_cb, data, get_nprocs() );
    Listing_Foreach( matches, &Batch_Del_Foreach_cb, bat, 1 );

    Listing_Free( &matches );
}

bool Batch_Del_byFunc_Find_cb( void *func, void *test ) {
    return ((Batch_Node *)test)->batcb == (batfunc_t)func;
}

void Batch_Del_byFunc(Batch bat, batfunc_t func ) {
    Listing matches;

    matches = Listing_Find( bat, &Batch_Del_byFunc_Find_cb, func, get_nprocs() );
    Listing_Foreach( matches, &Batch_Del_Foreach_cb, bat, 1 );

    Listing_Free( &matches );
}

/* Batch Execute */
void Batch_Run_Foreach_cb( void *input, void *node ){
    ((Batch_Node *)node)->batcb(((Batch_Node *)node)->sdata, input);
}

void Batch_Run( Batch bat, void *cdata, int threads ) {
    Listing_Foreach( bat, &Batch_Run_Foreach_cb, cdata, threads );
}
