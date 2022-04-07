/*  lists.c handles linked list functions for phase3.c */

#include "lists.h"

/** ------------------------------------- Globals ------------------------------------- **/

/* Flags */
extern int debugFlag;

/* General Globals */
extern void DebugConsole3(char *format, ...);

/* Process Globals */
extern usr_proc_struct UsrProcTable[MAXPROC];  //Process Table
extern int totalProc;           //total Processes
extern unsigned int nextPID;    //next process id

/* Sem Globals */
extern semaphore_struct SemaphoreTable[MAXSEMS]; //Semaphore Table
extern int totalSem;            //total Semaphores
extern unsigned int nextSID;    //next semaphore id

/** ------------------------------------ Functions ------------------------------------ **/

/* --------------------------------- General Functions --------------------------------- */

/* ------------------------------------------------------------------------
    Name -          InitializeList
    Purpose -       Initialize List pointed to by parameter
    Parameters -    *pCurProc: Pointer to Process List
    Returns -       None
    Side Effects -  None
   ----------------------------------------------------------------------- */
void InitializeList(procQueue *pProc, semQueue *pSem) {
    /*** Initialize Proc Queue ***/
    if (pSem == NULL) {
        pProc->pHeadProc = NULL;    //Head of Queue
        pProc->pTailProc = NULL;    //Tail of Queue
        pProc->total = 0;           //Total Count
    }
    /*** Initialize Sem Queue ***/
    else if (pProc == NULL) {
        pSem->pHeadSem = NULL;      //Head of Queue
        pSem->pTailSem = NULL;      //Tail of Queue
        pSem->total = 0;            //Total Count
    }

}/* InitializeList */


/* ------------------------------------------------------------------------
    Name -          ListIsFull
    Purpose -       Checks if Slot List OR Proc List is Full
    Parameters -    Meant to be used as 'either or', use NULL for other
                    *pProc: Pointer to Process List
                    *pSem:  Pointer to Semaphore
    Returns -        0: not full
                     1: full
    Side Effects -  none
   ----------------------------------------------------------------------- */
bool ListIsFull(const procQueue *pProc, const semQueue *pSem) {
    if (pSem == NULL) {
        return pProc->total >= MAXPROC;
    }
    else {
        return pSem->total >= MAXSEMS;
    }

}/* ListIsFull */


/* ------------------------------------------------------------------------
    Name -          ListIsEmpty
    Purpose -       Checks if Slot List OR Proc List is Empty
    Parameters -    Meant to be used as 'either or', use NULL for other
                    *pProc: Pointer to Process List
                    *pSem:  Pointer to Semaphore
    Returns -        0: not Empty
                     1: Empty
    Side Effects -  none
   ----------------------------------------------------------------------- */
bool ListIsEmpty(const procQueue *pProc, const semQueue *pSem) {
    if (pSem == NULL) {
    return pProc->pHeadProc == NULL;
    }
    else {
        return pSem->pHeadSem == NULL;
    }

}/* ListIsEmpty */


/* ------------------------------- Process List Functions ------------------------------- */

/* ------------------------------------------------------------------------
    Name -          AddProcessLL
    Purpose -       Add Process to Specified Process Queue
    Parameters -    pProc:      Pointer to Process
                    pq:         Pointer to Process Queue
    Returns -        0: Success
                    -1: Proc Queue is Full
                    -2: System ProcTable is Full
    Side Effects -  None
   ----------------------------------------------------------------------- */
int AddProcessLL(usr_proc_ptr pProc, procQueue * pq) {

    /*** Function Initialization ***/
    ProcList * pList;    //New Node

    /*** Error Check: Verify Space Available ***/
    if (ListIsFull(pq, NULL)) {
        DebugConsole3("%s: Queue is full.\n", __func__);
        return -1;
    }

    /*** Prepare pList and pProc to add to Linked List ***/
    /* Allocate Space for New Node */
    pList = (ProcList *) malloc (sizeof (ProcList));

    /* Verify Allocation Success */
    if (pList == NULL) {
        DebugConsole3("%s: Failed to allocate memory for node in linked list. Halting...\n",
                      __func__);
        halt(1);    //memory allocation failed
    }

    /*** Add pList to ProcQueue and Update Links ***/
    /* Point pList to pProc */
    pList->pProc = pProc;

    /* Add New Node to List */
    pList->pNextProc = NULL;                //assign pNext to NULL

    if (ListIsEmpty(pq, NULL)) {            //if list is empty...
        pq->pHeadProc = pList;              //add pNew to front of list
    }
    else {                                  //if list not empty...
        pq->pTailProc->pNextProc = pList;   //add pNew to end of list
        pList->pPrevProc = pq->pTailProc;   //assign pNew prev to current tail
    }

    pq->pTailProc = pList;                  //reassign pTail to new node

    /* Update Counter */
    pq->total++;                            //update pq count

    /*** Function Termination ***/
    return 0;                               //Proc List Addition Successful

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
    ProcList * pSave;   //Pointer for Linked List
    int pidFlag = 0;    //verify pid found in list
    int remFlag = 0;    //verify process removed successfully

    /*** Error Check: List is Empty ***/
    if (ListIsEmpty(pq, NULL)) {
        DebugConsole3("%s: Queue is empty.\n", __func__);
        halt(1);
    }

    /*** Find PID and Remove from List ***/
    pSave = pq->pHeadProc;              //Set pSave to head of list

    while (pSave != NULL)               //verify not end of list
    {
        /* PID Found */
        if (pSave->pProc->pid == pid) {
            pidFlag = 1;                //Trigger Flag - PID found

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

                remFlag = 1;                            //Trigger Flag - Process Removed
                break;
            }

            /** If pid to be removed is at tail **/
            else if (pSave == pq->pTailProc) {
                pq->pTailProc = pSave->pPrevProc;       //Set prev pid as tail
                pSave->pPrevProc = NULL;                //Set old tail prev pointer to NULL
                pq->pTailProc->pNextProc = NULL;        //Set new tail next pointer to NULL
                remFlag = 1;                            //Trigger Flag - Process Removed
                break;
            }

            /** If pid to be removed is in middle **/
            else {
                pSave->pPrevProc->pNextProc = pSave->pNextProc; //Set pSave prev next to pSave next
                pSave->pNextProc->pPrevProc = pSave->pPrevProc; //Set pSave next prev to pSave prev
                pSave->pNextProc = NULL;    //Set old pNext to NULL
                pSave->pPrevProc = NULL;    //Set old pPrev to NULL
                remFlag = 1;                //Trigger Flag - Process Removed
                break;
            }
        }
        /* PID Not Found */
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
        if (pidFlag == 0)
        {
            DebugConsole3("%s: Unable to locate pid [%d]. Halting...\n",
                          __func__, pid);
            halt(1);
        }
        else if (remFlag == 0)
        {
            DebugConsole3("%s: Removal of pid [%d] unsuccessful. Halting...\n",
                          __func__, remFlag);
            halt(1);
        }
    }

    /*** Function Termination ***/
    return pid;     //process removal success

}/* RemoveProcessLL */


/* ------------------------------------------------------------------------
 * TODO: Need?
 *
    Name -          CopyProcessLL
    Purpose -       Copies process from sourceQueue to destQueue
    Parameters -    pid:            Process ID
                    sourceQueue:    Pointer to Source Process Queue
                    destQueue:      Pointer to Destination Process Queue
    Returns -       pointer to copied process in destination
    Side Effects -  None
   ----------------------------------------------------------------------- */
//proc_ptr CopyProcessLL(int pid, procQueue * sourceQueue, procQueue * destQueue) {
//
//    /*** Function Initialization ***/
//    proc_ptr pSrcProc;      // Pointer to Proc in Source Queue
//    proc_ptr pDstProc;      // Pointer to Proc in Dest Queue
//
//    /*** Add Process to Destination ***/
//    AddProcessLL(pid, destQueue);
//
//    /*** Update Values ***/
//    /* Find Process in Source Queue */
//    pSrcProc = FindProcessLL(pid, sourceQueue);
//
//    /* Find Process in Destination Queue */
//    pDstProc = FindProcessLL(pid, destQueue);
//
//    /* Compare Values and Update on Mismatch */
//    if (pDstProc->zeroSlot != pSrcProc->zeroSlot)
//    {
//        pDstProc->zeroSlot = pSrcProc->zeroSlot;
//    }
//    if (pDstProc->delivered != pSrcProc->delivered)
//    {
//        pDstProc->delivered = pSrcProc->delivered;
//    }
//    if (pDstProc->slot_id != pSrcProc->slot_id)
//    {
//        pDstProc->slot_id = pSrcProc->slot_id;
//    }
//
//    /*** Remove Process from Destination ***/
//    RemoveProcessLL(pSrcProc->pid, sourceQueue);
//
//    /*** Function Termination ***/
//    return pDstProc;     //process copy success
//} /* CopyProcessLL */


/* ------------------------------------------------------------------------
    Name -          FindProcessLL
    Purpose -       Finds Process inside Specified Process Queue
    Parameters -    pid:    Process ID
                    pq:     Pointer to Process Queue
    Returns -       Pointer to found Process
    Side Effects -  None
   ----------------------------------------------------------------------- */
//proc_ptr FindProcessLL(int pid, procQueue * pq) {
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

/* ------------------------------ Semaphore List Functions ------------------------------ */

/* ------------------------------------------------------------------------
    Name -          AddSemaphoreLL
    Purpose -       Add Semaphore to Specified Sem Queue
    Parameters -    pSem:   Pointer to Semaphore
                    sq:     Pointer to Semaphore Queue
    Returns -        0: Success
                    -1: Sem Queue is Full
                    -2: System SemTable is Full
    Side Effects -  None
   ----------------------------------------------------------------------- */
int AddSemaphoreLL(sem_struct_ptr pSem, semQueue * sq) {

    /*** Function Initialization ***/
    SemList * pList;    //New Node

    /*** Error Check: Verify Space Available ***/
    if (ListIsFull(NULL, sq)) {
        DebugConsole3("%s: Queue is full.\n", __func__);
        return -1;
    }

    /*** Prepare pList and pProc to add to Linked List ***/
    /* Allocate Space for New Node */
    pList = (SemList *) malloc (sizeof (SemList));

    /* Verify Allocation Success */
    if (pList == NULL) {
        DebugConsole3("%s: Failed to allocate memory for node in linked list. Halting...\n",
                      __func__);
        halt(1);    //memory allocation failed
    }

    /*** Add pList to ProcQueue and Update Links ***/
    /* Point pList to pSem */
    pList->pSem = pSem;

    /* Add New Node to List */
    pList->pNextSem = NULL;                //assign pNext to NULL

    if (ListIsEmpty(NULL, sq)) {            //if list is empty...
        sq->pHeadSem = pList;              //add pNew to front of list
    }
    else {                                  //if list not empty...
        sq->pTailSem->pNextSem = pList;   //add pNew to end of list
        pList->pPrevSem = sq->pTailSem;   //assign pNew prev to current tail
    }

    sq->pTailSem = pList;                  //reassign pTail to new node

    /* Update Counter */
    sq->total++;                            //update sq count

    /*** Function Termination ***/
    return 0;                               //Sem List Addition Successful

}/* AddSemaphoreLL */


/* ------------------------------------------------------------------------
    Name -          RemoveSemaphoreLL
    Purpose -       Removes Semaphore from Specified Sem Queue
    Parameters -    sid:    Semaphore ID
                    sq:     Pointer to Semaphore Queu
    Returns -       SID of Removed Semaphore, halt (1) on error
    Side Effects -  None
   ----------------------------------------------------------------------- */
int RemoveSemaphoreLL(int sid, semQueue * sq) {

    /*** Function Initialization ***/
    SemList * pSave;   //Pointer for Linked List
    int sidFlag = 0;    //verify pid found in list
    int remFlag = 0;    //verify process removed successfully

    /*** Error Check: List is Empty ***/
    if (ListIsEmpty(NULL, sq)) {
        DebugConsole3("%s: Queue is empty.\n", __func__);
        halt(1);
    }

    /*** Find SID and Remove from List ***/
    pSave = sq->pHeadSem;              //Set pSave to head of list

    while (pSave != NULL)               //verify not end of list
    {
        /* PID Found */
        if (pSave->pSem->sid == sid) {
            sidFlag = 1;                //Trigger Flag - SID found

            /** If sid to be removed is at head **/
            if (pSave == sq->pHeadSem) {
                sq->pHeadSem = pSave->pNextSem;       //Set next pid as head
                pSave->pNextSem = NULL;                //Set old head next pointer to NULL

                /* if sem is NOT only node on list */
                if (sq->pHeadSem != NULL) {
                    sq->pHeadSem->pPrevSem = NULL;    //Set new head prev pointer to NULL
                }

                    /* if proc IS only node on list */
                else {
                    pSave->pPrevSem = NULL;            //Set old prev pointer to NULL
                    sq->pTailSem = NULL;               //Set new tail to NULL
                }

                remFlag = 1;                            //Trigger Flag - Semaphore Removed
                break;
            }

                /** If sid to be removed is at tail **/
            else if (pSave == sq->pTailSem) {
                sq->pTailSem = pSave->pPrevSem;       //Set prev sid as tail
                pSave->pPrevSem = NULL;                //Set old tail prev pointer to NULL
                sq->pTailSem->pNextSem = NULL;        //Set new tail next pointer to NULL
                remFlag = 1;                            //Trigger Flag - Semaphore Removed
                break;
            }

                /** If sid to be removed is in middle **/
            else {
                pSave->pPrevSem->pNextSem = pSave->pNextSem; //Set pSave prev next to pSave next
                pSave->pNextSem->pPrevSem = pSave->pPrevSem; //Set pSave next prev to pSave prev
                pSave->pNextSem = NULL;    //Set old pNext to NULL
                pSave->pPrevSem = NULL;    //Set old pPrev to NULL
                remFlag = 1;                //Trigger Flag - Process Removed
                break;
            }
        }
            /* SID Not Found */
        else {
            pSave = pSave->pNextSem;       //increment to next in list
        }
    }//end while loop

    /*** Decrement Counter ***/
    sq->total--;

    /*** Error Check: Pid Found and Removed ***/
    if (sidFlag && remFlag) {
        pSave = NULL;       //free() causing issues in USLOSS
    }
    else {
        if (sidFlag == 0)
        {
            DebugConsole3("%s: Unable to locate pid [%d]. Halting...\n",
                          __func__, sid);
            halt(1);
        }
        else if (remFlag == 0)
        {
            DebugConsole3("%s: Removal of pid [%d] unsuccessful. Halting...\n",
                          __func__, remFlag);
            halt(1);
        }
    }

    /*** Function Termination ***/
    return sid;     //semaphore removal success

}/* RemoveSemaphoreLL */


/* ------------------------------------------------------------------------
 * TODO: Need? update header info
 *
    Name -          CopySemaphoreLL
    Purpose -       Copies Semaphore from sourceQueue to destQueue
    Parameters -    sid:            Semaphore ID
                    sourceQueue:    Pointer to Source Process Queue
                    destQueue:      Pointer to Destination Process Queue
    Returns -       pointer to copied process in destination
    Side Effects -  None
   ----------------------------------------------------------------------- */
//sem_ptr CopySemaphoreLL(int sid, semQueue * sourceQueue, semQueue * destQueue) {
//    /*** Function Initialization ***/
//    sem_ptr pSrcSem;      // Pointer to Sem in Source Queue
//    sem_ptr pDstSem;      // Pointer to Sem in Dest Queue
//
//    /*** Add Process to Destination ***/
//    //AddProcessLL(sid, destQueue);
//
//    /*** Update Values ***/
//    /* Find Process in Source Queue */
//    pSrcSem = FindProcessLL(sid, sourceQueue);
//
//    /* Find Process in Destination Queue */
//    pDstSem = FindProcessLL(sid, destQueue);
//
//    /* Compare Values and Update on Mismatch */
//    if (pDstProc->zeroSlot != pSrcProc->zeroSlot)
//    {
//        pDstProc->zeroSlot = pSrcProc->zeroSlot;
//    }
//    if (pDstProc->delivered != pSrcProc->delivered)
//    {
//        pDstProc->delivered = pSrcProc->delivered;
//    }
//    if (pDstProc->slot_id != pSrcProc->slot_id)
//    {
//        pDstProc->slot_id = pSrcProc->slot_id;
//    }
//
//    /*** Remove Process from Destination ***/
//    //RemoveProcessLL(pSrcProc->pid, sourceQueue);
//
//    /*** Function Termination ***/
//    return pDstSem;     //process copy success
//
//} /* CopySemaphoreLL */


/* ------------------------------------------------------------------------
 * TODO: Need? update header info
 *
    Name -          FindSemaphoreLL
    Purpose -       Finds Semaphore inside Specified Sem Queue
    Parameters -    sid:    Semaphore ID
                    sq:     Pointer to Sem Queue
    Returns -       Pointer to Found Semaphore
    Side Effects -  None
   ----------------------------------------------------------------------- */
//sem_ptr FindSemaphoreLL(int sid, semQueue * sq) {
//    /*** Function Initialization ***/
//    SemList * pSave;   //Pointer for Linked List
//    int sidFlag = 0;    //verify pid found in list
//
//    /*** Error Check: List is Empty ***/
//    if (ListIsEmpty(NULL, sq))
//    {
//        DebugConsole3("%s: Queue is empty.\n", __func__);
//        return NULL;
//    }
//
//    /*** Find PID ***/
//    pSave = sq->pHeadSem;          //Set pSave to head of list
//
//    while (pSave->pNextSem != NULL)         //verify not end of list
//    {
//        if (pSave->pSem->sid == sid)      //if found sid...
//        {
//            /*** Function Termination ***/
//            return pSave;           //semaphore found
//
//        }
//        else                        //if sid not found
//        {
//            pSave = pSave->pNextSem;   //increment to next in list
//        }
//    }//end while
//
//    /*** Function Termination ***/
//    return NULL;     //semaphore not found
//
//}/* FindSemaphoreLL */
/* --------------------- END OF Linked List Functions --------------------- */