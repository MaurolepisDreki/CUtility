/* Task Configuration & Management */

#include <pthread.h>
#include <stdbool.h>
#include "listing.h"

#ifndef INCLUDED_TASK_H
#define INCLUDED_TASK_H

typedef void (*task_func_t)(void *);

typedef struct task_t{
    task_func_t cb;
    void *datapack;
    Listing depend;
	Listing blocks;
	bool enable;
	bool exec;
	short priority;
	pthread_mutex_t mutex;
} Task;

/* Task Init & Free */
Task * Task_Init( task_func_t, void *, short );
void Task_Free( Task * );
void Task_FreeTree( Task * ); /* < Recursively call Task_Free foreach dependency of Task */

/* Task Dependencies */
void Task_Depend_Add( Task *target, Task *depend );
void Task_Depend_Del( Task *target, Task *depend );

/* Task Queries */
bool Task_isEnabled( Task * ); /* < A Task is enabled if and only if all of it's dependencies are enabled. */
bool Task_hasCircDeps( Task * );  /* < A Task contains Circular Dependencies if by following dependencies as a stack a task shows up more than once */
bool Task_isDependent( Task *, Task * ); /* < Detects dependencies recursively */
bool Task_Ready( Task * ); /* < A Task is ready if it is enabled, has not been executed, and has no dependences that have not been executed */

/* Set the enable flag of a single Task */
void Task_setEnabled( Task *, bool );
#define Task_Enable( task ) Task_setEnabled( task, true )
#define Task_Disable( task ) Task_setEnabled( task, false )

/* Reset tasks for execution */
void Task_Reset( Task * ); /* Reset Single Task */
void Task_ResetTree( Task * ); /* Reset Task and all it's dependencies */

/* Perform the selected task */
bool Task_Do( Task * ); /* < Returns true if task was executed, false if task not ready */

#endif /* INCLUDED_TASK_H */
