/* Header File for Phase 4 */

#ifndef PROCESS_H    //Header Guards
#define PROCESS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <usloss.h>
#include "phase4_helper.h"
#include "driver.h"
#include "tables.h"
#include "lists.h"
#include "usloss/include/usloss.h"  //Added for Clion Functionality

/** ------------------------ Typedefs and Structs ------------------------ **/
typedef struct p4proc_struct p4proc_struct;
typedef struct p4proc_struct proc_struct;   //todo: need?
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
    int     pid;            //process ID
    int     index;          //index Location
    int     status;         //status
    int     wakeTime;       //time to wake
    int     mboxID;         //mailbox ID
    int     blockSem;
    int     signalingSem;
    int     diskFirstTrack;
    int     diskFirstSec;
    int     diskSecs;
    char    *diskBuffer;
    // todo: lists?
    device_request diskRequest;     //found within usloss.h
};
//end of Process structures

/* Structures for Sleeping Processes */
typedef struct sleepList {
    sleep_proc_ptr      pProc;
    struct sleepList    *pNextProc;
    struct sleepList    *pPrevProc;
} SleepList;

typedef struct sleepQueue {
    SleepList   *pHeadProc;
    SleepList   *pTailProc;
    int         total;
} SleepQueue;

//struct sleep_struct {
//    int     pid;            //process ID
//    int     index;          //index Location
//    int     status;         //status
//    int     mSecSleep;      //mSec: milliseconds    //todo: need?
//    int     wakeTime;       //time to wake
//    int     signalingSem;
//};
//end of Sleeping Process structures


#endif //PROCESS_H