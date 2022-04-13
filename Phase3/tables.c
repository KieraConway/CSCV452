/*  tables.c handles process table and sem table functions*/

#include "tables.h"
#include "phase3_helper.h"

/** ------------------------------------- Globals ------------------------------------- **/

extern int debugFlag;   //flag for debugging
extern int start2Pid;   //pid for start2
extern usr_proc_struct UsrProcTable[MAXPROC];       //Process Table
extern semaphore_struct SemaphoreTable[MAXSEMS];    //Semaphore Table

/** ------------------------------------ Functions ------------------------------------ **/
/* -------------------------------- External Prototypes -------------------------------- */
extern void DebugConsole3(char *format, ...);

/* ------------------------------ Process Table Functions ------------------------------ */

/* ------------------------------------------------------------------------
   Name -           ProcessInit
   Purpose -        Initializes Proc Table Entry
   Parameters -     index:  process location in ProcTable
                    pid:    unique process ID
   Returns -        none
   Side Effects -   Clears Process from Table
   ----------------------------------------------------------------------- */
void ProcessInit(int index, short pid) {

    /*** Function Initialization ***/
    usr_proc_struct * pProc = &UsrProcTable[index];

    /*** Clear out ProcTable Entry ***/
    memset(pProc->name,
           0,
           sizeof(pProc->name));        //Process Name

    memset(pProc->startArg,
           0,
           sizeof(pProc->startArg));    //Args for StartFunc

    pProc->index = -1;                      //Process Index
    pProc->startFunc = NULL;                //Starting Function
    pProc->pid = -1;                        //Process ID
    pProc->stackSize = -1;                  //Process Stack Size
    pProc->priority = -1;                   //Process Priority
    pProc->mboxID = -1;                     //Process Private Mailbox
    pProc->status = STATUS_EMPTY;           //Process Status
    pProc->parentPID = -1;                  //Process Parent
    InitializeList(&pProc->children); //Slot List

    return;

} /* ProcessInit */


/* ------------------------------------------------------------------------
   Name -           AddToProcTable
   Purpose -        Adds Process to ProcTable
   Parameters -     newStatus:  New status being assigned
                    name:       character string containing process’s name
                    pid:        unique process ID
                    startFunc:  address of the function to spawn
                    startArg:   parameter passed to spawned function
                    stackSize:  stack size (in bytes)
                    priority:   priority
   Returns -        none
   Side Effects -   Process added to ProcessTable
   ----------------------------------------------------------------------- */
void AddToProcTable(int newStatus, char name[], int pid, int (* startFunc) (char *),
                    char * startArg, int stackSize, int priority) {

    /*** Function Initialization ***/
    int parentPID = getpid();
    int index = GetProcIndex(pid);
    usr_proc_ptr pProc = &UsrProcTable[index];

    /*** Update ProcTable Entry ***/
    memcpy(pProc->name,
           name,
           sizeof(name));            //Process Name
    pProc->index = index;               //Process Index
    pProc->startFunc = startFunc;       //Starting Function
    pProc->pid = pid;                   //Process ID
    pProc->stackSize = stackSize;       //Process Stack Size
    pProc->priority = priority;         //Process Priority
    pProc->status = newStatus;          //Process Status
    InitializeList(&pProc->children);   //Children List

    if(parentPID > start2Pid){          //Prevent Start2 from being Parent
        pProc->parentPID = parentPID;   //Process Parent
    }

    if(startArg) {
        memcpy(pProc->startArg,
               startArg,
               sizeof(pProc->startArg)); //Args for StartFunc
    }

    return;

} /* AddToProcTable */


/* ------------------------------------------------------------------------
   Name -           GetProcIndex
   Purpose -        Gets Process index from pid
   Parameters -     pid:    Unique Process ID
   Returns -        Corresponding Index
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetProcIndex(int pid) {
    return pid % MAXPROC;

} /* GetProcIndex */

/* ----------------------------- Semaphore Table Functions ----------------------------- */

/* ------------------------------------------------------------------------
   Name -           SemaphoreInit
   Purpose -        Initializes Sem Table Entry
   Parameters -     index:  semaphore location in SemTable
                    sid:    unique semaphore ID
   Returns -        none
   Side Effects -   Clears Semaphore from Table
   ----------------------------------------------------------------------- */
void SemaphoreInit(int index, short sid) {

    /*** Function Initialization ***/
    semaphore_struct * pSem = &SemaphoreTable[index];

    /*** Clear out SemTable Entry ***/
    pSem->status = STATUS_EMPTY;
    pSem->sid = -1;
    pSem->value = -1;

    InitializeList(&pSem->blockedProcs); //Sem Process List

    return;

} /* SemaphoreInit */


/* ------------------------------------------------------------------------
   Name -           AddToSemTable
   Purpose -        Adds Semaphore to SemTable
   Parameters -     sid:        unique semaphore ID
                    newStatus:  new status being assigned
                    newValue:   new semaphore value
   Returns -        none
   Side Effects -   Semaphore added to SemaphoreTable
   ----------------------------------------------------------------------- */
void AddToSemTable(int sid, int newStatus, int newValue) {

    /*** Function Initialization ***/
    int index = GetSemIndex(sid);
    sem_struct_ptr pSem = &SemaphoreTable[index];

    /*** Update SemTable Entry ***/
    pSem->sid = sid;
    pSem->status = newStatus;
    pSem->value = newValue;
    InitializeList(&pSem->blockedProcs);   //Waiting List

    return;

} /* AddToSemTable */


/* ------------------------------------------------------------------------
   Name -           GetSemIndex
   Purpose -        Gets Semaphore index from sid
   Parameters -     sid:    unique semaphore ID
   Returns -        Corresponding Index
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetSemIndex(int sid) {
    return sid % MAXSEMS;

} /* GetSemIndex */

/** -------------------- END of Table Functions -------------------- **/