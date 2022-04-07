/* Header File for Phase 3 */

#ifndef PHASE_3_HELPER_H
#define PHASE_3_HELPER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sems.h"
#include "tables.h"
#include "lists.h"

/** ------------------------- Constants ----------------------------------- **/
#define DEBUG3 1

#define STATUS_EMPTY        0
#define STATUS_RUNNING      1
#define STATUS_READY        2
#define STATUS_QUIT         3
#define STATUS_ZAP_BLOCK    4
#define STATUS_JOIN_BLOCK   5
#define STATUS_ZAPPED       6
#define STATUS_LAST         7

#define ITEM_IN_USE         1

/** ------------------------ Typedefs and Structs ------------------------ **/
typedef struct usr_proc_struct usr_proc_struct;
typedef struct usr_proc_struct * usr_proc_ptr;
typedef struct procQueue procQueue;
typedef struct procList * proc_ptr;
typedef struct semQueue semQueue;

/* Structures for Processes */
typedef struct  procList{
    usr_proc_ptr    pProc;      //points to process
    struct procList *pNextProc; //points to next process
    struct procList *pPrevProc; //points to previous process
} ProcList;

typedef struct procQueue {
    ProcList *pHeadProc;            //points to msg list head
    ProcList *pTailProc;            //points to msg list tail
    int total;                      //counts total procs in queue
} ProcQueue;

struct usr_proc_struct {
    char                name[MAXNAME];        	    //Process Name
    int                 index;                      //Process Index: getpid()%MAXPROC
    char                startArg[MAXARG];    	    //Starting Argument(s) for startFunc
    int                 (* startFunc) (char *);     //Starting Function where Proc Launches
    short               pid;                  	    //Process ID
    unsigned int        stackSize;                  //Stack Size
    int                 priority;                   //Priority
    int                 mboxID;                    //Private MailboxID
    int                 status;                     //Status
    int                 parentPID;                  //Parent PID
    procQueue           children;                   //List of Children
};
//end of Process structures


struct psr_bits {
    unsigned int cur_mode:1;
    unsigned int cur_int_enable:1;
    unsigned int prev_mode:1;
    unsigned int prev_int_enable:1;
    unsigned int unused:28;
};

union psr_values {
    struct psr_bits bits;
    unsigned int integer_part;
};

/** ------------------------ External Prototypes ------------------------ **/
/* --------------------- External lists.c Prototypes --------------------- */
/** General, Multi-use **/
/* Initializes Linked List */
void InitializeList(procQueue *pProc, semQueue *pSem);
/* Check if Proc List or Sem list is Full | Returns True when Full */
bool ListIsFull(const procQueue *pProc, const semQueue *pSem);
/* Check if Proc List or Sem list is Empty | Returns True when Empty */
bool ListIsEmpty(const procQueue *pProc, const semQueue *pSem);

/** Process Lists **/
/* Add Process to Specified Process Queue | Returns 0 on Sucess */
extern int AddProcessLL(usr_proc_ptr pProc, procQueue * pq);
/* Remove Process from Process Queue | Returns PID of Removed Process*/
extern int RemoveProcessLL(int pid, procQueue * pq);
/* Copies a Process to another Queue | Returns Pointer to Copied Process */
//extern proc_ptr CopyProcessLL(int pid, procQueue * sourceQueue, procQueue * destQueue);
/* Finds Process in Process Queue | Returns Pointer to Found Process */
//extern proc_ptr FindProcessLL(int pid, procQueue * pq);

/* --------------------- External tables.c Prototypes --------------------- */
/** Process Table **/
/* Finds Next Available pid | Returns Next pid */
extern int GetNextPID();
/* Initializes Proc Table Entry */
extern void ProcessInit(int index, short pid);
/* Adds Process to Table */
extern void AddToProcTable(int newStatus, char name[], int pid, int (* startFunc) (char *), char startArg, int stackSize, int priority);
/* Gets Mailbox index from id | Returns Corresponding index */
extern int GetProcIndex(int pid);

//end external function prototypes
#endif