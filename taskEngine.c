#include "taskEngine.h"
#include <sys/sysinfo.h>
#include "util.h"

/* Initialize and Destruction */
void TaskEngine_Init( TaskEngine *te ) {
	te->task_queue = Listing_Init();
	te->workers = Listing_Init();
	
	pthread_mutex_init( &te->task_mutex, NULL );
	pthread_mutex_init( &te->worker_mutex, NULL );
	
	te->running = false;
}

void TaskEngine_Destroy( TaskEngine *te ) {
	if( te->running ) TaskEngine_Stop( te, true );
	
	pthread_mutex_destroy( &te->task_mutex );
	pthread_mutex_destroy( &te->worker_mutex );
	
	Listing_Free( &te->task_queue );
	Listing_Free( &te->workers );
}

/* Task Engine Query: Queued 
 *   Informs whether a task exists in the queue */
bool TaskEngine_Task_Queued( TaskEngine *te, Task *t ) {
	return Listing_IndexOf( te->task_queue, t ) == -1;
}

/* Task Queue Sorting (cb) */
bool TaskEngine_SortQueue_cb( void *a, void *b ) {
	if( ((Task *)b)->exec && !((Task *)a)->exec ) return true; /* < in the off-chance pre-executed tasks and duplicates end up in queue: handle bad tasks first */
	if( Task_isDependent( (Task *)b, (Task *)a ) && ! Task_isDependent((Task *)a, (Task *)b ) ) return true;
	if( ((Task *)b)->priority > ((Task *)a)->priority ) return true;
	if( !Task_isEnabled( (Task *)b ) && Task_isEnabled( (Task *)a ) ) return true;
	return false;
}

/* Task Management: duplicate tasks not permitted */
inline bool TaskEngine_Task_OK( void *te, void *t ) {
	return !((Task *)t)->exec && !TaskEngine_Task_Queued( (TaskEngine *)te, (Task *)t ) && Task_isEnabled( (Task *)t );
}

void TaskEngine_Task_Add( TaskEngine *te, Task *t ) {
	pthread_mutex_lock( &te->task_mutex );
	
	if( TaskEngine_Task_OK( te, t ) ) {
		Listing_PushBack( te->task_queue, t );
		Listing_Sort( te->task_queue, &TaskEngine_SortQueue_cb );
	}
	
	pthread_mutex_unlock( &te->task_mutex );
}

void TaskEngine_Task_Del( TaskEngine *te, Task *t ) {
	int index;
	pthread_mutex_lock( &te->task_mutex );
	
	do {
		index = Listing_IndexOf( te->task_queue, t );
		if( index != -1 ) Listing_Remove( te->task_queue, index );
	} while( index != -1 );
	
	pthread_mutex_unlock( &te->task_mutex );
}

void TaskEngine_Task_Add_Batch( TaskEngine *te, Listing tl ) {
	Listing import;
	pthread_mutex_lock( &te->task_mutex );
	
	import = Listing_Find( tl, &TaskEngine_Task_OK, te, get_nprocs() );
	Listing_Merge( te->task_queue, import );
	
	pthread_mutex_unlock( &te->task_mutex );
	Listing_Free( &import );
}

void TaskEngine_Task_Del_Batch( TaskEngine *te, Listing tl ) {
	pthread_mutex_lock( &te->task_mutex );
	
	for( int ix = 0; ix < Listing_Count( tl ); ix++ ) {
		int iy;
		do {
			iy = Listing_IndexOf( te->task_queue, Listing_At( tl, ix ) );
			if( iy != -1 ) Listing_Remove( te->task_queue, iy );
		} while( iy != -1 );
	}
	
	pthread_mutex_unlock( &te->task_mutex );
}

/* Task Engine Operation Mechanics */
bool TaskEngine_FindSelf_cb( void *na, void *pthr ) {
	return *((pthread_t *)pthr) == pthread_self();
}

void *TaskEngine_Worker( void *te ) {
	TaskEngine *engine = (TaskEngine *)te;
	/* Ensure Engine is Alive. */
	while( engine->running ) {
		/* SEGMENT:: Worker Management */
		pthread_mutex_lock( &engine->task_mutex );
		if( Listing_Count( engine->task_queue ) > 1) {
			/* Temporarily change objectives from controlling the task_queue to controlling the workers */
			if( pthread_mutex_trylock( &engine->worker_mutex ) != 0 ) {
				pthread_mutex_unlock( &engine->task_mutex );
				
				pthread_t *npthr;
				while( Listing_Count( engine->workers ) < engine->max_workers && Listing_Count( engine->workers ) < Listing_Count( engine->task_queue ) ) {
					npthr = malloc( sizeof( pthread_t ) );
					if( npthr == NULL ) break; /* memory allocation problem; *DO NOT PROCEDE CREATING WORKERS*: This is only for optimization and is not mission-critical */
					
					if( pthread_create( npthr, NULL, &TaskEngine_Worker, engine ) == 0 ) {		
						Listing_PushBack( engine->workers, npthr );
					} else {
						free( npthr );
						break;
					}
				}
					
				pthread_mutex_unlock( &engine->worker_mutex );
				pthread_mutex_lock( &engine->task_mutex );
			}
		} else {
			pthread_mutex_unlock( &engine->task_mutex );
			break;
		}
		
		/* SEGMENT:: Task Processing */
		/* Get Next Task */
		Task * myTask = NULL;
		do {
			myTask = Listing_AtFront( engine->task_queue );
			Listing_PopFront( engine->task_queue );
		} while( pthread_mutex_trylock( &myTask->mutex ) != 0 );
			
		/* Do Task */
		bool canExec = !myTask->exec && myTask->enable;
		for( int ix = 0; ix < Listing_Count( myTask->depend ) && canExec; ix++ ) {
			Task *tmp = Listing_At( myTask->depend, ix );
			if( pthread_mutex_trylock( &tmp->mutex ) == 0 ) {
				canExec = tmp->exec || (Task_isDependent( tmp, myTask ) && Listing_IndexOf( engine->task_queue, tmp ) != -1);
				pthread_mutex_unlock( &tmp->mutex );
			} else {
				/* Assume tmp->mutex is locked because task is running */
				canExec = false;
			}
		}
		pthread_mutex_unlock( &engine->task_mutex );
		
		if( canExec ) {
			myTask->cb( myTask->datapack );
			myTask->exec = true;
			pthread_mutex_unlock( &myTask->mutex );
			/* Task Cleanup Occurs Outside the Engine; our job is done */
		} else {
			/* Task Cannot Execute: Add dependencies to the queue, repost the task, and try again. */
			TaskEngine_Task_Add_Batch( engine, myTask->depend );
			TaskEngine_Task_Add( engine, myTask );
			pthread_mutex_unlock( &myTask->mutex );
		}
	}
	
	/* Cleanup: remove self from workers */
	pthread_mutex_lock( &engine->worker_mutex );

	Listing self = Listing_Find( engine->workers, &TaskEngine_FindSelf_cb, NULL, 1 );
	while( Listing_Count( self ) > 0 ) {
		int index;
		do {
			index = Listing_IndexOf( engine->workers, Listing_AtFront( self ) );
			if( index != -1 ) {
				free( Listing_At( engine->workers, index ) );
				Listing_Remove( engine->workers, index );
			}
		} while( index != -1 );
		Listing_PopFront( self );
	}
	Listing_Free( &self );
	if( Listing_Count( engine->workers ) == 0 ) engine->running = false; /* < If we are the last worker to die, the engine has stopped */
	pthread_mutex_unlock( &engine->worker_mutex );
	return NULL;
}

/* TaskEngine Start & Stop */
void TaskEngine_Start( TaskEngine *engine, int max_workers, bool wait) {
	engine->running = true;
	engine->max_workers = max_workers;
	
	/* Put First Thread on the Stack */
	pthread_mutex_lock( &engine->worker_mutex );
	pthread_t *first_thread;
	fmalloc( first_thread, sizeof( pthread_t ) );
	if( wait ) {
		*first_thread = pthread_self();
	} else {
		pthread_create( first_thread, NULL, TaskEngine_Worker, engine );
	}
	Listing_PushBack( engine->workers, first_thread );
	pthread_mutex_unlock( &engine->worker_mutex );
	
	
	if( wait ) {
		/* Utilize Current Thread as First Worker if we are waiting */
		TaskEngine_Worker( engine );
		
		/* If the worker on our thread exits before the engine stops, join the workers */
		while( Listing_Count( engine->workers ) != 0 ) {
			void *rval; /* < garbage; our workers don't return information */
			pthread_join( *((pthread_t *)Listing_AtFront( engine->workers )), &rval );
		}
	}
}

void TaskEngine_Stop( TaskEngine *engine, bool wait ) {
	engine->running = false;
	
	if( wait ) {
		/* To efficiently wait out the death of the workers, join them */
		while( Listing_Count( engine->workers ) != 0 ) {
			void *rval; /* < garbage; our workers don't return information */
			pthread_join( *((pthread_t *)Listing_AtFront( engine->workers )), &rval );
		}
	}
}

