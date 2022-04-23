#ifndef PHASE_4_PHASE4_HELPER_H     //Header Guards
#define PHASE_4_PHASE4_HELPER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <usloss.h>
#include "process.h"
#include "driver.h"
#include "tables.h"
#include "lists.h"

/** ------------------------- Constants ----------------------------------- **/
#define DEBUG4      0

#define EMPTY       0
#define READY       1
#define SLEEPING    2
#define ZAPPED      3


/** ------------------------ External Prototypes ------------------------ **/
/* --------------------- External process.c Prototypes --------------------- */
/** General **/
/* Gets ProcessTable index from PID | Returns Corresponding index */
extern int GetIndex(int pid);

/** ProcTable **/
/* Add Process to SleepProcTable */
extern void AddToProcTable(int pid);
/* Remove Process from SleepProcTable | Returns 0 on Success */
extern int RemoveFromProcTable(int pid);
/* Creates a pointer to specified process | Pointer to Process */
extern proc_ptr CreatePointer(int pid);

/* --------------------- External lists.c Prototypes --------------------- */
/* Initializes Linked List */
extern void InitializeList(procQueue *pProc);
/* Check if Proc List or Sem list is Full | Returns True when Full */
extern bool ListIsFull(const procQueue *pProc);
/* Check if Proc List or Sem list is Empty | Returns True when Empty */
extern bool ListIsEmpty(const procQueue *pProc);
/* Add Process to Specified Process Queue | Returns 0 on Success */
extern int AddProcessLL(proc_ptr pProc, procQueue * pq);
/* Remove Process from Process Queue | Returns PID of Removed Process*/
extern int RemoveProcessLL(int pid, procQueue * pq);
/* Finds Process inside Specified Process Queue | Returns Pointer to Found Process*/
extern proc_ptr FindProcessLL(int pid, procQueue * pq);

///** Process Sleep Lists **/
///* Add Process to SleepProcTable | Returns 0 on Success */
//extern void AddToSleepProcTable(int pid, int mSec, int semSignal);
///* Remove Process from SleepProcTable | Returns 0 on Success */
//extern int RemoveFromSleepProcTable(int pid);


/** -------------------- END of External Prototypes -------------------- **/
#endif


