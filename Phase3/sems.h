/* Header File for Semaphores */

#ifndef PHASE3_SEMS_H
#define PHASE3_SEMS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tables.h"
#include "lists.h"
#include "phase3_helper.h"

#define SEM_READY 0
#define SEM_USED  1
#define LOCKED    1
#define UNLOCKED  0
#define FREEING   2

/** ------------------------ Typedefs and Structs ------------------------ **/
typedef struct semaphore_struct semaphore_struct;
typedef struct semaphore_struct * sem_struct_ptr;
typedef struct semQueue semQueue;
typedef struct semList * sem_ptr;

/* Structures for Semaphores */
typedef struct semList {
    sem_struct_ptr  pSem;        //points to semaphore
    struct semList *pNextSem;    //points to next semaphore
    struct semList *pPrevSem;    //points to previous semaphore
} SemList;

typedef struct semQueue {
    SemList *pHeadSem;           //points to semaphore list head
    SemList *pTailSem;           //points to semaphore list tail
    int total;                   //counts total semaphores in queue
} SemQueue;

//struct semaphore_struct {
//    int sid;                     // ID of semaphore
//    int mutex;                   // semaphore mutex
//    int status;                  // semaphore status
//    semQueue WaitingSems;        // list of waiting semaphores
//    int mboxID;                        // private mbox ID
//    int value;                         // value of semaphore
//    struct procQueue blockedProcs;      // list of blocked processes
//};

/** ------------------------ External Prototypes ------------------------ **/
/* --------------------- External lists.c Prototypes --------------------- */
/** Semaphore Lists **/
/* Add Semaphore to Specified Sem Queue | Returns 0 on Success */
extern int AddSemaphoreLL(sem_struct_ptr pSem, semQueue * sq);
/* Remove Semaphore from Sem Queue | Returns SID of Removed Semaphore*/
extern int RemoveSemaphoreLL(int sid, semQueue * sq);
/* Copies a Semaphore to another Queue | Returns Pointer to Copied Semaphore */
//extern sem_ptr CopySemaphoreLL(int sid, semQueue * sourceQueue, semQueue * destQueue) ;
/* Finds Semaphore in Sem Queue | Returns Pointer to Found Semaphore */
//extern sem_ptr FindSemaphoreLL(int sid, semQueue * sq);

/* --------------------- External tables.c Prototypes --------------------- */
/** Semaphore Table **/
/* Finds Next Available sid | Returns Next sid */
//int GetNextSID();
/* Initializes Sem Table Entry */
void SemaphoreInit(int index, short sid);
/* Adds Semaphore to Table */
void AddToSemTable(int sid, int newMutex, int newStatus, int newMboxID, int newValue);
/* Gets Semaphore index from sid | Returns Corresponding index */
int GetSemIndex(int sid);

#endif