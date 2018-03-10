#include "task.h"
#include <stdlib.h>
#include "util.h"
#include <sys/sysinfo.h>

/* Local Dependency Untangling */
struct depStack_Node { Task *t; unsigned int currdep; };
#define DEPSTACK_BEGIN() Listing depStack = Listing_Init()
#define DEPSTACK_EMPTY() Listing_isEmpty( depStack )
#define DEPSTACK_ITEM( index ) ((struct depStack_Node *)Listing_At( depStack, index ))
#define DEPSTACK_TOP() DEPSTACK_ITEM( 0 )
#define DEPSTACK_CURRENT() DEPSTACK_TOP()->t
#define DEPSTACK_PUSH( task ) { struct depStack_Node *tmp; fmalloc( tmp, sizeof( struct depStack_Node ) ); tmp->t = task; tmp->currdep = 0; Listing_PushFront( depStack, tmp ); }
#define DEPSTACK_POP() { free( DEPSTACK_TOP() ); Listing_PopFront( depStack ); }
#define DEPSTACK_CLEAR() while( ! DEPSTACK_EMPTY() ) { DEPSTACK_POP(); }
#define DEPSTACK_END() DEPSTACK_CLEAR(); Listing_Free( &depStack )
#define DEPSTACK_CONTAINS( task ) depStack_hasTask( depStack, task )
inline bool depStack_hasTask( Listing ds, Task *t ) {
	bool found = false;
	
	for( unsigned int ix = 0; ix < Listing_Count( ds ) && !found; ix++ ) {
		found = (((struct depStack_Node *)Listing_At( ds, ix ))->t == t);
	}
	
	return found;
}

/* Task Init & Free */
Task * Task_Init( task_func_t func, void *data, short pri ) {
	Task *newt;
	newt = malloc( sizeof( Task ) );
	
	if( newt != NULL ) {
		newt->cb = func;
		newt->datapack = data;
		newt->priority = pri;
		newt->enable = true;
		newt->exec = false;
		newt->depend = Listing_Init();
		newt->blocks = Listing_Init();
		pthread_mutex_init( &newt->mutex, NULL );
	}
	
	return newt;
}

void Task_Free_depend_Foreach_cb( void *this_task, void *depend ) {
	Task_Depend_Del( (Task *)this_task, (Task *)depend );
}

void Task_Free_blocks_Foreach_cb( void * this_task, void *blocks ) {
	Task_Depend_Del( (Task *)blocks, (Task *)this_task );
}

void Task_Free( Task *t ) {
	t->enable = false;
	Listing_Foreach( t->depend, &Task_Free_blocks_Foreach_cb, t, 1 );
	Listing_Foreach( t->blocks, &Task_Free_depend_Foreach_cb, t, 1 );
	Listing_Free( &(t->depend) );
	Listing_Free( &(t->blocks) );
	pthread_mutex_destroy( &t->mutex );
	free( t );
}

/* Task Dependencies */
void Task_Depend_Add( Task *target, Task *depend ) {
	if( target == depend ) return; /* << Invalid Case: a task cannot be it's own dependency! */
	Listing_PushBack( target->depend, depend );
	Listing_PushBack( depend->blocks, target );
}

void Task_Depend_Del( Task *target, Task *depend ) {
	int index;
	
	/* Delete depends from target */
	do {
		index = Listing_IndexOf( target->depend, depend );
		if( index >= 0 ) {
			Listing_Remove( target->depend, index );
		}
	} while( index >= 0 );
	
	/* Delete target form depends */
	do {
		index = Listing_IndexOf( depend->blocks, target );
		if( index >= 0 ) {
			Listing_Remove( depend->blocks, index );
		}
	} while( index >= 0 );
}

/* Task Queries */
bool Task_isEnabled( Task *t ) {
	if( t->enable ) {
		bool enable = true;
		DEPSTACK_BEGIN();
		DEPSTACK_PUSH( t );
		
		while( ! DEPSTACK_EMPTY() && enable ) {
			if( Listing_Count( DEPSTACK_CURRENT()->depend ) > DEPSTACK_TOP()->currdep ) {
				if( DEPSTACK_CONTAINS( Listing_At( DEPSTACK_CURRENT()->depend, DEPSTACK_TOP()->currdep ) ) ) {
					DEPSTACK_TOP()->currdep++;
				} else {
					DEPSTACK_PUSH( Listing_At( DEPSTACK_CURRENT()->depend, DEPSTACK_TOP()->currdep++ ) );
				}
			} else {
				enable = DEPSTACK_CURRENT()->enable;
				DEPSTACK_POP();
			}
		}
		
		DEPSTACK_END();
		return enable;
	} else {
		return false;
	}
}

bool Task_hasCircDeps( Task *t ) {
	bool circFound = false;
	DEPSTACK_BEGIN();
	DEPSTACK_PUSH( t );
	
	while( ! DEPSTACK_EMPTY() && ! circFound ) {
		if( Listing_Count( DEPSTACK_CURRENT()->depend ) > DEPSTACK_TOP()->currdep ) {
			if( DEPSTACK_CONTAINS( Listing_At( DEPSTACK_CURRENT()->depend, DEPSTACK_TOP()->currdep ) ) ) {
				circFound = true;
				DEPSTACK_TOP()->currdep++;
			} else {
				DEPSTACK_PUSH( Listing_At( DEPSTACK_CURRENT()->depend, DEPSTACK_TOP()->currdep++ ) );
			}
		} else {
			DEPSTACK_POP();
		}
	}
	
	DEPSTACK_END();
	return circFound;
}

bool Task_isDependent( Task *start, Task *target ) {
	bool depFound = false;
	DEPSTACK_BEGIN();
	DEPSTACK_PUSH( start );
	
	while( ! DEPSTACK_EMPTY() && ! depFound ) {
		if( Listing_Count( DEPSTACK_CURRENT()->depend ) > DEPSTACK_TOP()->currdep ) {
			if( DEPSTACK_CONTAINS( Listing_At( DEPSTACK_CURRENT()->depend, DEPSTACK_TOP()->currdep ) ) ) {
				DEPSTACK_TOP()->currdep++;
			} else {
				DEPSTACK_PUSH( Listing_At( DEPSTACK_CURRENT()->depend, DEPSTACK_TOP()->currdep++ ) );
			}
		} else {
			depFound = DEPSTACK_CURRENT() == target;
			DEPSTACK_POP();
		}
	}
	
	DEPSTACK_END();
	return depFound;
}

bool Task_Ready( Task *t ) {
	if( t->enable && ! t->exec ) {
		bool allExec = true;
		
		for( unsigned int ix = 0; ix < Listing_Count( t->depend ) && allExec; ix++ ) {
			allExec = allExec && ((Task *)Listing_At( t->depend, ix ))->exec;
		}
		
		return allExec;
	} else {
		return false;
	}
}

/* Set the enable flag of a single Task */
void Task_setEnabled( Task *t, bool val ) {
	t->enable = val;
}

/* Reset tasks for execution */
void Task_Reset( Task *t ) {
	t->exec = false;
}

void Task_ResetTree( Task *t ) {
	DEPSTACK_BEGIN();
	DEPSTACK_PUSH( t );
	
	while( ! DEPSTACK_EMPTY() ) {
		if( Listing_Count( DEPSTACK_CURRENT()->depend ) > DEPSTACK_TOP()->currdep ) {
			if( DEPSTACK_CONTAINS( Listing_At( DEPSTACK_CURRENT()->depend, DEPSTACK_TOP()->currdep ) ) ) {
				DEPSTACK_TOP()->currdep++;
			} else {
				DEPSTACK_PUSH( Listing_At( DEPSTACK_CURRENT()->depend, DEPSTACK_TOP()->currdep++ ) );
			}
		} else {
			Task_Reset( DEPSTACK_CURRENT() );
			DEPSTACK_POP();
		}
	}
	
	DEPSTACK_END();
}

/* Perform the selected task */
bool Task_Do( Task *t ) {
	if( Task_Ready( t ) ) {
		t->cb( t->datapack );
		t->exec = true;
		return true;
	} else {
		return false;
	}
}
