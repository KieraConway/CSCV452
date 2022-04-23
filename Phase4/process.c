#include "process.h"
#include "usloss/include/phase1.h"  //Added for Clion Functionality

/** ------------------------------------- Globals ------------------------------------- **/
/* Flags */
extern int debugFlag;   //flag for debugging

/* Process Globals  */
extern p4proc_struct ProcTable[MAXPROC];    //Process Table
extern procQueue SleepingProcs;             //Sleeping Processes List

/** ------------------------------------ Functions ------------------------------------ **/
/* -------------------------------- External Prototypes -------------------------------- */
extern void DebugConsole4(char *format, ...);

/* ------------------------------ Process Table Functions ------------------------------ */

/* ------------------------------------------------------------------------
   Name -           GetIndex
   Purpose -        Gets Process index from id
   Parameters -     pid:    Unique Process ID
   Returns -        Process Index
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetIndex(int pid){
    return pid % MAXPROC;

} /* GetSlotIndex */


/* ------------------------------------------------------------------------
  Name -            AddToProcTable
  Purpose -         Adds current process to ProcTable()
  Parameters -      pid:    Unique Process ID
  Returns -         none
  Side Effects -    Process added to SleepProcTable
  ----------------------------------------------------------------------- */
void AddToProcTable(int pid){
    /*** Function Initialization ***/
    int index = GetIndex(pid);
    proc_ptr pProc = &ProcTable[index];

    /*** Update ProcTable Entry ***/
    pProc->index = index;       //Process Index
    pProc->pid = pid;           //Process ID
    pProc->status = READY;      //Process Status

}

/* ------------------------------------------------------------------------
 * TODO: function and header
  Name -            AddToSleepProcTable
  Purpose -         Adds current process to SleepProcTable()
  Parameters -      seconds:
                    semSignal:
  Returns -         0:  Success
  Side Effects -    Process added to SleepProcTable
  ----------------------------------------------------------------------- */
RemoveFromProcTable(int pid){

}

/* ------------------------------------------------------------------------
    Name -          CreatePointer
    Purpose -       Creates a pointer to specified process
    Parameters -    pid:    Process ID
    Returns -       Pointer to found Process
    Side Effects -  None
   ----------------------------------------------------------------------- */
proc_ptr CreatePointer(int pid){
    /*** Create Current Pointer ***/
    int index = getpid() % MAXPROC;
    return &ProcTable[index];

}

///* ------------------------------------------------------------------------
// * todo: finish header
//  Name -            AddToProcTable
//  Purpose -         Adds current process to ProcTable()
//  Parameters -      pid:        Unique Process ID
//                    mSec:       milliseconds
//                    semSignal:
//  Returns -         none
//  Side Effects -    Process added to SleepProcTable
//  ----------------------------------------------------------------------- */
//void AddToSleepProcTable(int pid, int mSec, int semSignal){
//    /*** Function Initialization ***/
//    /* Create Process Pointer */
//    int index = GetIndex(pid);
//    sleep_proc_ptr pProc = &SleepProcTable[index];
//
//    /* Calculate Wake Time */
//    int wakeTime = sys_clock() + mSec;  //todo: need to test functionality
//
//    /*** Update ProcTable Entry ***/
//    pProc->index = index;       //Process Index
//    pProc->pid = pid;           //Process ID
//    pProc->status = SLEEPING;   //Process Status
//    pProc->mSecSleep = mSec;    //mSeconds to sleep
//    pProc->wakeTime = wakeTime; //Time to Wake Process
//
//}
//
