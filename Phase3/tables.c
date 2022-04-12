/*  tables.c handles process table and sem table functions*/

#include "tables.h"
#include "phase3_helper.h"

/** ------------------------------------- Globals ------------------------------------- **/
/* Flags */
extern int debugFlag;

/* General Globals */
extern int start2Pid;

/* Semaphore Globals */
extern usr_proc_struct UsrProcTable[MAXPROC];  //Process Table
extern int totalProc;           //total Processes
extern unsigned int nextPID;    //next process id

/* Sem Globals */
extern semaphore_struct SemaphoreTable[MAXSEMS]; //Semaphore Table
extern int totalSem;            //total Semaphores
extern unsigned int nextSID;    //next semaphore id

/** ------------------------------------ Functions ------------------------------------ **/
/* -------------------------------- External Prototypes -------------------------------- */
extern void DebugConsole3(char *format, ...);

/* ------------------------------ Process Table Functions ------------------------------ */

/* ------------------------------------------------------------------------
 * //todo will we need? because fork1 returns PID
   Name -           GetNextPID
   Purpose -        Finds next available pid
   Parameters -     none
   Returns -        >0: next pid
   Side Effects -   none
   ----------------------------------------------------------------------- */
//int GetNextPID()
//{
//    int newPID = -1;
//    int procIndex = GetProcIndex(nextPID);
//
//    if (totalProc < MAXPROC) {
//        while ((totalProc < MAXPROC) && (UsrProcTable[procIndex].status != STATUS_EMPTY)) {
//            nextPID++;
//            procIndex = nextPID % MAXPROC;
//        }
//
//        newPID = nextPID % MAXPROC;
//    }
//
//    return newPID;
//
//} /* GetNextPID */


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
    InitializeList(&pProc->children, NULL); //Slot List

} /* ProcessInit */


/* ------------------------------------------------------------------------
   Name -           AddToProcTable
   Purpose -        Adds Process to ProcTable
   Parameters -     newStatus:  New status being assigned
                    name:       character string containing process’s name
                    pid:        unique process ID
                    func:       address of the function to spawn
                    arg:        parameter passed to spawned function
                    stack_size: stack size (in bytes)
                    priority:   priority
   Returns -        none
   Side Effects -   Process added to ProcessTable
   ----------------------------------------------------------------------- */
void AddToProcTable(int newStatus, char name[], int pid, int (* startFunc) (char *),
                    char startArg, int stackSize, int priority) {

    /*** Function Initialization ***/
    int parentPID = getpid();   //todo: what if adding self to proc table?
    int index = GetProcIndex(pid);
    usr_proc_ptr pProc = &UsrProcTable[index];

    /*** Update ProcTable Entry ***/
    memcpy(pProc->name,
           name,
           sizeof(name));               //Process Name
    pProc->index = index;               //Process Index
    pProc->startFunc = startFunc;       //Starting Function
    pProc->pid = pid;                   //Process ID
    pProc->stackSize = stackSize;       //Process Stack Size
    pProc->priority = priority;         //Process Priority
    pProc->status = newStatus;          //Process Status
    InitializeList(&pProc->children,
                   NULL);         //Children List

    if(parentPID > start2Pid){          //Prevent Start2 from being Parent
        pProc->parentPID = parentPID;   //Process Parent
    }

    if(startArg) {
        memcpy(pProc->startArg,
               &startArg,
               sizeof(pProc->startArg)); //Args for StartFunc
    }

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
   Side Effects -   Clears Process from Table
   ----------------------------------------------------------------------- */
void SemaphoreInit(int index, short sid) {

    /*** Function Initialization ***/
    semaphore_struct * pSem = &SemaphoreTable[index];

    /*** Clear out SemTable Entry ***/
    pSem->status = STATUS_EMPTY;
    pSem->mutex = -1;
    pSem->sid = -1;
    pSem->mboxID = -1;

    InitializeList(&pSem->blockedProcs, NULL); //Slot List

} /* SemaphoreInit */


/* ------------------------------------------------------------------------
 * TODO: UPDATE function header info
 *
   Name -           AddToSemTable
   Purpose -        Adds Semaphore to SemTable
   Parameters -     newStatus:  New status being assigned
                    sid:        unique semaphore ID
                    newMutex:
                    newMboxID:
                    newValue:
   Returns -        none
   Side Effects -   Process added to ProcessTable
   ----------------------------------------------------------------------- */
void AddToSemTable(int sid, int newMutex, int newStatus, int newMboxID, int newValue) {

    /*** Function Initialization ***/
    int index = GetProcIndex(sid);
    sem_struct_ptr pSem = &SemaphoreTable[index];

    /*** Update SemTable Entry ***/
    pSem->sid = sid;
    pSem->mutex = newMutex;
    pSem->status = newStatus;
    pSem->mboxID = newMboxID;
    pSem->value = newValue;
    InitializeList(&pSem->blockedProcs, NULL);   //Waiting List

} /* AddToSemTable */


/* ------------------------------------------------------------------------
   Name -           GetProcIndex
   Purpose -        Gets Process index from pid
   Parameters -     pid:    Unique Process ID
   Returns -        Corresponding Index
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetSemIndex(int sid) {
    return sid % MAXSEMS;

} /* GetSemIndex */

/* ------------------------ END OF Table Functions ------------------------ */