#include <stdbool.h>
#include "lists.h"
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
/* ------------------------------ Process List Functions ------------------------------ */

//usr_proc_struct ->p4proc_struct
//
/* ------------------------------------------------------------------------
    Name -          InitializeList
    Purpose -       Initialize List pointed to by parameter
    Parameters -    *pProc: Pointer to Process List
    Returns -       None
    Side Effects -  None
   ----------------------------------------------------------------------- */
void InitializeList(procQueue *pProc) {

    /*** Initialize Proc Queue ***/
    pProc->pHeadProc = NULL;    //Head of Queue
    pProc->pTailProc = NULL;    //Tail of Queue
    pProc->total = 0;           //Total Count

}/* InitializeList */


/* ------------------------------------------------------------------------
    Name -          ListIsFull
    Purpose -       Checks if process list is Full
    Parameters -    *pProc: Pointer to Process List
    Returns -        0: not full
                     1: full
    Side Effects -  none
   ----------------------------------------------------------------------- */
bool ListIsFull(const procQueue *pProc) {
    return pProc->total == MAXPROC;

}/* ListIsFull */


/* ------------------------------------------------------------------------
    Name -          ListIsEmpty
    Purpose -       Checks if process list is empty
    Parameters -    *pProc: Pointer to Process List
    Returns -        0: not Empty
                     1: Empty
    Side Effects -  none
   ----------------------------------------------------------------------- */
bool ListIsEmpty(const procQueue *pProc) {
    return pProc->pHeadProc == NULL;

}/* ListIsEmpty */


/* ------------------------------------------------------------------------
    Name -          AddProcessLL
    Purpose -       Add Process to Specified Process Queue
    Parameters -    pProc:      Pointer to Process
                    pq:         Pointer to Process Queue
    Returns -        0: Success
                    -1: Proc Queue is Full or System ProcTable is Full
    Side Effects -  None
   ----------------------------------------------------------------------- */
int AddProcessLL(proc_ptr pProc, procQueue * pq) {

    /*** Function Initialization ***/
    ProcList * pList;       //New Node
    ProcList * pSave;       //Save Pointer
    bool procAdded = false; //Process Added flag

    /*** Error Check: Verify Space Available ***/
    if (ListIsFull(pq)) {
        DebugConsole4("%s: Queue is full.\n", __func__);
        return -1;
    }

    /*** Prepare pList and pProc to add to Linked List ***/
    /* Allocate Space for New Node */
    pList = (ProcList *) malloc (sizeof (ProcList));

    /* Verify Allocation Success */
    if (pList == NULL) {
        DebugConsole4("%s: Failed to allocate memory for node in linked list. Halting...\n",
                      __func__);
        halt(1);    //memory allocation failed
    }

    /*** Add pList to ProcQueue and Update Links ***/
    /* Point pList to pProc */
    pList->pProc = pProc;

    /* Add New Node to List */
    pList->pNextProc = NULL;                //assign pNext to NULL

    if (ListIsEmpty(pq)) {                  //if list is empty...
        pq->pHeadProc = pList;              //add pNew to front of list
        pq->pTailProc = pList;              //reassign pTail to new node
    }
    else {                                  //if list not empty...

        /** Check if SleepingProcs List **/
        /* If SleepingProcs List - Order Matters */
        if(pq == &SleepingProcs) {           //todo: verify functionality

            /** Iterate Through List **/
            pSave = pq->pHeadProc;          //Set pSave to head of list
            while (pSave->pProc != NULL) {   //verify not end of list

                /** Compare Wake Times **/
                /* if pProc wakes later than pSave, iterate */
                if (pProc->wakeTime >= pSave->pProc->wakeTime) {
                    pSave = pSave->pNextProc;       //increment to next in list
                }
                    /* if pSave wakes later than pProc, insert pProc */
                else {
                    pList->pNextProc = pSave;
                    pList->pPrevProc = pSave->pPrevProc;
                    pSave->pPrevProc->pNextProc = pList;
                    pSave->pPrevProc = pList;

                    procAdded = true;   //update flag
                }
            }
        }

        if(procAdded == false){
            pq->pTailProc->pNextProc = pList;   //add pNew to end of list
            pList->pPrevProc = pq->pTailProc;   //assign pNew prev to current tail
            pq->pTailProc = pList;              //reassign pTail to new node
        }
    }


    /* Update Counter */
    pq->total++;            //update pq count

    /*** Function Termination ***/
    return !procAdded;      //returns 0 if Process Addition Successful

}/* AddProcessLL */


/* ------------------------------------------------------------------------
    Name -          RemoveProcessLL
    Purpose -       Removes Process from Specified Process Queue
    Parameters -    pid:    Process ID
                    pq:     Pointer to Process Queue
    Returns -       Pid of Removed Process, halt (1) on error
    Side Effects -  None
   ----------------------------------------------------------------------- */
int RemoveProcessLL(int pid, procQueue * pq) {

    /*** Function Initialization ***/
    ProcList * pSave;       //Pointer for Linked List
    bool pidFlag = false;   //verify pid found in list
    bool remFlag = false;   //verify process removed successfully

    /*** Error Check: List is Empty ***/
    if (ListIsEmpty(pq)) {
        DebugConsole4("%s: Queue is empty.\n", __func__);
        halt(1);
    }

    /*** Find PID and Remove from List ***/
    pSave = pq->pHeadProc;              //Set pSave to head of list

    while (pSave != NULL)               //verify not end of list
    {
        /** PID Found **/
        if (pSave->pProc->pid == pid) {
            pidFlag = true;             //Trigger Flag - PID found

            /** If pid to be removed is at head **/
            if (pSave == pq->pHeadProc) {
                pq->pHeadProc = pSave->pNextProc;       //Set next pid as head
                pSave->pNextProc = NULL;                //Set old head next pointer to NULL

                /* if proc is NOT only node on list */
                if (pq->pHeadProc != NULL) {
                    pq->pHeadProc->pPrevProc = NULL;    //Set new head prev pointer to NULL
                }

                    /* if proc IS only node on list */
                else {
                    pSave->pPrevProc = NULL;            //Set old prev pointer to NULL
                    pq->pTailProc = NULL;               //Set new tail to NULL
                }

                remFlag = true;                         //Trigger Flag - Process Removed
                break;
            }

            /** If pid to be removed is at tail **/
            else if (pSave == pq->pTailProc) {
                pq->pTailProc = pSave->pPrevProc;       //Set prev pid as tail
                pSave->pPrevProc = NULL;                //Set old tail prev pointer to NULL
                pq->pTailProc->pNextProc = NULL;        //Set new tail next pointer to NULL
                remFlag = true;                         //Trigger Flag - Process Removed
                break;
            }

            /** If pid to be removed is in middle **/
            else {
                pSave->pPrevProc->pNextProc = pSave->pNextProc; //Set pSave prev next to pSave next
                pSave->pNextProc->pPrevProc = pSave->pPrevProc; //Set pSave next prev to pSave prev
                pSave->pNextProc = NULL;    //Set old pNext to NULL
                pSave->pPrevProc = NULL;    //Set old pPrev to NULL
                remFlag = true;             //Trigger Flag - Process Removed
                break;
            }
        }
        /** PID Not Found **/
        else {
            pSave = pSave->pNextProc;       //increment to next in list
        }
    }//end while loop

    /*** Decrement Counter ***/
    pq->total--;

    /*** Error Check: Pid Found and Removed ***/
    if (pidFlag && remFlag) {
        pSave = NULL;       //free() causing issues in USLOSS
    }
    else {
        if (pidFlag == false)
        {
            DebugConsole4("%s: Unable to locate pid [%d]. Halting...\n",
                          __func__, pid);
            halt(1);
        }
        else if (remFlag == false)
        {
            DebugConsole4("%s: Removal of pid [%d] unsuccessful. Halting...\n",
                          __func__, remFlag);
            halt(1);
        }
    }

    /*** Function Termination ***/
    return pid;     //process removal success

}/* RemoveProcessLL */

///* ------------------------------------------------------------------------
//    Name -          FindProcessLL
//    Purpose -       Finds Process inside Specified Process Queue
//    Parameters -    pid:    Process ID
//                    pq:     Pointer to Process Queue
//    Returns -       Pointer to found Process
//    Side Effects -  None
//   ----------------------------------------------------------------------- */
//proc_ptr FindProcessLL(int pid, procQueue * pq){
//    /*** Function Initialization ***/
//    ProcList * pSave;   //Pointer for Linked List
//    int pidFlag = 0;    //verify pid found in list
//
//    /*** Error Check: List is Empty ***/
//    if (ListIsEmpty(NULL, pq))
//    {
//        DebugConsole2("%s: Queue is empty.\n", __func__);
//        return NULL;
//    }
//
//    /*** Find PID ***/
//    pSave = pq->pHeadProc;          //Set pSave to head of list
//
//    while (pSave->pid != 0)         //verify not end of list
//    {
//        if (pSave->pid == pid)      //if found pid...
//        {
//            /*** Function Termination ***/
//            return pSave;           //process found
//
//        }
//        else                        //if pid not found
//        {
//            pSave = pSave->pNextProc;   //increment to next in list
//        }
//    }//end while
//
//    /*** Function Termination ***/
//    return NULL;     //process not found
//
//}/* FindProcessLL */

/* --------------------- END OF Linked List Functions --------------------- */

/** -------------------- END of Linked List Functions -------------------- **/