/* Header File for Phase 4 */

#ifndef PROCESS_H    //Header Guards
#define PROCESS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <usloss.h>
#include "driver.h"
#include "tables.h"
#include "lists.h"

/** ------------------------- Constants ----------------------------------- **/


/** ------------------------ Typedefs and Structs ------------------------ **/
typedef struct p4proc_struct p4proc_struct;
typedef struct p4proc_struct proc_struct;
typedef struct p4proc_struct * proc_ptr;
typedef struct procQueue procQueue;
typedef struct p4procList * proc_list_ptr;

typedef struct sleep_struct sleep_struct;
typedef struct sleep_struct * sleep_proc_ptr;
typedef struct sleepQueue sleepQueue;
typedef struct sleepList * sleep_proc_list_ptr;

/* Structures for Processes */
typedef struct procList{
    proc_ptr    pProc;          //points to process
    struct procList *pNextProc; //points to next process
    struct procList *pPrevProc; //points to previous process
} ProcList;

typedef struct procQueue {
    ProcList *pHeadProc;            //points to proc list head
    ProcList *pTailProc;            //points to proc list tail
    int total;                      //counts total procs in queue
} ProcQueue;

struct p4proc_struct {
    int pid;
    int mboxId;
    int blockSem;
    int wakeTime;
    int diskFirstTrack;
    int diskFirstSec;
    int diskSecs;
    char *diskBuffer;
    // lists?
    device_request diskRequest; // found within usloss.h
};
//end of Process structures

/* Structures for Sleeping Processes */
typedef struct sleepList {
    sleep_proc_ptr pProc;
    struct sleepList *pNextProc;
    struct sleepList *pPrevProc;
} SleepList;

typedef struct sleepQueue {
    SleepList *pHeadProc;
    SleepList *pTailProc;
    int total;
} SleepQueue;

struct sleep_struct {
    int pid;
    int msecSleep;      //msec: milliseconds
    int signalingSem;
};
//end of Sleeping Process structures


#endif //PROCESS_H