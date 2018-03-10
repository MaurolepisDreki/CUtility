/* Common Task Engine */

#include <pthread.h>
#include "listing.h"
#include <stdbool.h>
#include "task.h"

#ifndef INCLUDED_TASKENGINE_H
#define INCLUDED_TASKENGINE_H

typedef struct taskengine_t {
    Listing task_queue;
    pthread_mutex_t task_mutex;
    Listing workers;
    pthread_mutex_t worker_mutex;
    int max_workers;
    bool running;
} TaskEngine;

/* Constructor & Destructor */
void TaskEngine_Init( TaskEngine * );
void TaskEngine_Destroy( TaskEngine * );

/* Task Engine Query */
bool TaskEngine_Task_Queued( TaskEngine *, Task * );

/* Task Management: duplicate tasks not permitted */
void TaskEngine_Task_Add( TaskEngine *, Task * );
void TaskEngine_Task_Add_Batch( TaskEngine *, Listing );
void TaskEngine_Task_Del( TaskEngine *, Task * );
void TaskEngine_Task_Del_Batch( TaskEngine *, Listing );

/* TaskEngine Start & Stop */
void TaskEngine_Start(TaskEngine *, int max_workers, bool wait);
void TaskEngine_Stop(TaskEngine *, bool wait);

#endif /* INCLUDED_TASKENGINE_H */
