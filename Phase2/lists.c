/*  lists.c handles linked list functions for phase1.c */

#include "lists.h"

/* -------------------------- Globals ------------------------------------- */

/*Mailbox Globals*/
extern mailbox MailboxTable[MAXMBOX];   //mailbox table
extern int totalMailbox;                //total mailboxes
extern unsigned int next_mid;           //next mailbox id [mbox_id]

/* -------------------------- Functions ----------------------------------- */

/* ------------------------------------------------------------------------
    Name -          InitializeList
    Purpose -       Initialize List pointed to by parameter
    Parameters -    Meant to be used as 'either or', use NULL for other
                    *pSlot: Pointer to Message Slot List
                    *pCurProc: Pointer to Process List
    Returns -       None, halt(1) on error
    Side Effects -  None
   ----------------------------------------------------------------------- */
void InitializeList(slotQueue *pSlot, procQueue *pProc)
{
    /*** Initialize Slot Queue ***/
    if(pProc == NULL)
    {
        /* Initialize the head, tail, and count slot queue */
        pSlot->pHeadSlot = NULL;
        pSlot->pTailSlot = NULL;
        pSlot->total = 0;
        return;
    }

    /*** Initialize Proc Queue ***/
    else if(pSlot == NULL)
    {
        /* Initialize the head, tail, and count proc queue */
        pProc->pHeadProc = NULL;
        pProc->pTailProc = NULL;
        pProc->total = 0;
    }

    /*** Error Check: Invalid Parameters ***/
    else
    {
        DebugConsole2("%s: Invalid Parameters Passed", __func__);
        halt(1);
    }

}/* InitializeList */


/* ------------------------------------------------------------------------
    Name -          ListIsFull
    Purpose -       Checks if Slot List OR Proc List is Full
    Parameters -    Meant to be used as (pSlot & pMbox) or (pCurProc)
                    *pSlot: Pointer to Message Slot List
                    *pMbox: Pointer to Mailbox
                    * * * OR * * *
                    *pCurProc: Pointer to Process List
    Returns -        0: not full
                     1: full
                    -1: list not found
    Side Effects -  none
   ----------------------------------------------------------------------- */
bool ListIsFull(const slotQueue *pSlot, const mailbox *pMbox, const procQueue *pProc)
{
    /*** Search if Slot Queue is Full ***/
    if(pProc == NULL)
    {
        return pSlot->total >= pMbox->maxSlots;
    }

    /*** Search if Proc Queue is Full ***/
    else if(pSlot == NULL)
    {
        return pProc->total >= MAXPROC;
    }

    /*** Error Check: Invalid Parameters ***/
    else
    {
        DebugConsole2("%s: Invalid Parameters Passed", __func__);
        halt(1);
    }

    return -1;

}/* ListIsFull */


/* ------------------------------------------------------------------------
    Name -          ListIsEmpty
    Purpose -       Checks if Slot List OR Proc List is Empty
    Parameters -    Meant to be used as 'either or', use NULL for other
                    *pSlot: Pointer to Message Slot List
                    *pCurProc: Pointer to Process List
    Returns -        0: not Empty
                     1: Empty
                    -1: list not found
    Side Effects -  none
   ----------------------------------------------------------------------- */
bool ListIsEmpty(const slotQueue *pSlot, const procQueue *pProc)
{
    /*** Search if Slot Queue is Empty ***/
    if(pProc == NULL)
    {
        return pSlot->pHeadSlot == NULL;    //checks if pHead is empty
    }

    /*** Search if Proc Queue is Empty ***/
    else if(pSlot == NULL)
    {
        return pProc->pHeadProc == NULL;    //checks if pHead is empty
    }

    /*** Error Check: Invalid Parameters ***/
    else
    {
        DebugConsole2("%s: Invalid Parameters Passed", __func__);
        halt(1);
    }

    return -1;

}/* ListIsEmpty */


/* ------------------------------------------------------------------------
    Name -          AddSlotLL
    Purpose -       Adds Slot with Message to mBox slotQueue
    Parameters -    slotIndex:  Slot Index in Mailbox
                    mbox:       Pointer to Mailbox
    Returns -        0: Success
                    -1: Slot Queue is Full
                    -2: System SlotTable is Full
    Side Effects -  None
   ----------------------------------------------------------------------- */
int AddSlotLL(slot_table * pSlot, mailbox * pMbox)
{
    /*** Function Initialization ***/
    SlotList * pList;     //new slot list

    /*** Error Check: Verify Slot Space Available***/
    /* check if queue is full */
    if(ListIsFull(&pMbox->slotQueue, pMbox, NULL))
    {
        DebugConsole2("%s: Slot Queue is full.\n", __func__);
        return -1;
    }

    /*** Prepare pList and pSLot to add to Linked List ***/
    /* allocate pList */
    pList = (SlotList *) malloc (sizeof (SlotList));

    /* verify allocation */
    if (pList == NULL)
    {
        DebugConsole2("%s: Failed to allocate memory for node in linked list. Halting...\n",
                      __func__ );
        halt(1);    //memory allocation failed
    }

    /* Point pList to pSlot */
    pList->slot = pSlot;
    pList->slot_id = pSlot->slot_id;

    /*** Add pList to Mailbox Queue and Update Links ***/
    pList->pNextSlot = NULL;                //Add pList to slotQueue

    if (ListIsEmpty(&pMbox->slotQueue, NULL))     //if index is empty...
    {
        pMbox->slotQueue.pHeadSlot = pList;                  //add to front of list
    }
    else
    {
        pMbox->slotQueue.pTailSlot->pNextSlot = pList;   //add to end of list'
        pList->pPrevSlot = pMbox->slotQueue.pTailSlot;   //assign pPrev to current Tail
    }

    pMbox->slotQueue.pTailSlot = pList;      //reassign pTail to new Tail

    /*** Update Counters***/
    pMbox->activeSlots++;        //mailbox active slots
    pMbox->slotQueue.total++;    //mailbox total

    /*** Function Termination ***/
    return 0;   //Slot Addition Successful

}/* AddSlotLL */


/* ------------------------------------------------------------------------
    Name -          RemoveSlotLL
    Purpose -       Remove Slot from Specific Mailbox
    Parameters -    slot_id:    Slot ID
                    pMbox:      Mailbox to Remove From
    Returns -       slot_id of removed slot
    Side Effects -  None
   ----------------------------------------------------------------------- */
int RemoveSlotLL(int slot_id, slotQueue * sq)
{
    /*** Function Initialization ***/
    SlotList * pSave;           //New node to save position
    int idFlag = 0;             //verify id found in list
    int remFlag = 0;            //verify slot removed successfully

    /*** Error Check: List is Empty ***/
    if (ListIsEmpty(sq, NULL))
    {
        DebugConsole2("%s: Queue is empty.\n", __func__);
        halt(1);
    }

    /*** Find PID and Remove from List ***/
    pSave = sq->pHeadSlot;          //Set pSave to head of list

    while (pSave != NULL)              //verify not end of list
    {
        if (pSave->slot_id == slot_id) //if found slot_id to remove...
        {
            idFlag = 1;                //Trigger Flag - PID found

            /** If pid to be removed is at head **/
            if (pSave == sq->pHeadSlot)
            {
                sq->pHeadSlot = pSave->pNextSlot;           //Set next pid as head
                pSave->pNextSlot = NULL;                    //Set old head next pointer to NULL

                /* if proc is NOT only node on list */
                if (sq->pHeadSlot != NULL)
                {
                    sq->pHeadSlot->pPrevSlot = NULL;        //Set new head prev pointer to NULL
                }

                    /* if proc IS only node on list */
                else
                {
                    pSave->pPrevSlot = NULL;    //Set old prev pointer to NULL
                    sq->pTailSlot = NULL;       //Set new tail to NULL

                }

                remFlag = 1;                    //Trigger Flag - Process Removed
                break;
            }

                /** If pid to be removed is at tail **/
            else if (pSave == sq->pTailSlot)
            {
                sq->pTailSlot = pSave->pPrevSlot;       //Set prev pid as tail
                pSave->pPrevSlot = NULL;                //Set old tail prev pointer to NULL
                sq->pTailSlot->pNextSlot = NULL;        //Set new tail next pointer to NULL
                remFlag = 1;                            //Trigger Flag - Process Removed
                break;
            }

                /** If pid to be removed is in middle **/
            else
            {
                pSave->pPrevSlot->pNextSlot = pSave->pNextSlot; //Set pSave prev next to pSave next
                pSave->pNextSlot->pPrevSlot = pSave->pPrevSlot; //Set pSave next prev to pSave prev
                pSave->pNextSlot = NULL;                        //Set old pNext to NULL
                pSave->pPrevSlot = NULL;                        //Set old pPrev to NULL
                remFlag = 1;                                    //Trigger Flag - Process Removed
                break;
            }
        }
        else                            //if pid not found
        {
            pSave = pSave->pNextSlot;   //increment to next in list
        }
    }//end while

    /*** Decrement Counter ***/
    sq->total--;

    /*** Error Check: Pid Found and Removed ***/
    if (idFlag && remFlag)
    {
        pSave = NULL;                  //free() causing issues in USLOSS
    }
    else
    {
        if (idFlag == 0)
        {
            DebugConsole2("%s: Unable to locate pid [%d]. Halting...\n",
                          __func__, slot_id);
            halt(1);
        }
        else if (remFlag == 0)
        {
            DebugConsole2("%s: Removal of pid [%d] unsuccessful. Halting...\n",
                          __func__, remFlag);
            halt(1);
        }
    }

    /*** Decrement Counters ***/
    int mboxIndex = GetMboxIndex(sq->mbox_id);  //get mailbox Index
    mailbox *pMbox = &MailboxTable[mboxIndex];  // Create Mailbox pointer
    pMbox->activeSlots--;
    pMbox->slotQueue.total--;

    /*** Function Termination ***/
    return slot_id;                             //process removal success

}/* RemoveSlotLL */


/* ------------------------------------------------------------------------
    Name -          FindSlotLL
    Purpose -       Finds Slot inside Specified Slot Queue
    Parameters -    slot_id:    Slot ID
                    sq:         Pointer to Slot Queue
    Returns -       Pointer to Found Slot
    Side Effects -  None
   ----------------------------------------------------------------------- */
slot_ptr FindSlotLL(int slot_id, slotQueue * sq)
{
    /*** Function Initialization ***/
    SlotList * pSave;                   //Pointer for Linked List
    int idFlag = 0;                     //verify id found in list

    /*** Error Check: List is Empty ***/
    if (ListIsEmpty(sq, NULL))
    {
        DebugConsole2("%s: Queue is empty.\n", __func__);
        return NULL;
    }

    /*** Find PID ***/
    pSave = sq->pHeadSlot;             //Set pSave to head of list

    while (pSave != NULL)              //verify not end of list
    {
        if (pSave->slot_id == slot_id) //if found id...
        {
            /*** Function Termination ***/
            return pSave;              //slot found

        }
        else                           //if pid not found
        {
            pSave = pSave->pNextSlot;  //increment to next in list
        }
    }//end while

    /*** Function Termination ***/
    return NULL;     //process not found

}/* FindSlotLL */


/* ------------------------------------------------------------------------
    Name -          AddProcessLL
    Purpose -       Add Process to Specified Process Queue
    Parameters -    pid:        Process ID
                    pq:         Pointer to Process Queue
                    delivered:  indicates if zero message delivered
                    slot_id:    Regular - holds slot queue id
                    zeroSlot:   Zero Slot - points to Slot Table
    Returns -       None, halt (1) on error
    Side Effects -  None
   ----------------------------------------------------------------------- */
void AddProcessLL(int pid, procQueue * pq)
{
    /*** Function Initialization ***/
    ProcList * pNew;    //New Node

    /*** Error Check: List is Full ***/
    if (ListIsFull(NULL, NULL, pq))
    {
        DebugConsole2("%s: Queue is full.\n", __func__);
        halt(1);
    }

    /*** Allocate Space for New Node ***/
    pNew = (ProcList *) malloc (sizeof (ProcList));

    /* Verify Allocation Success */
    if (pNew == NULL)
    {
        DebugConsole2("%s: Failed to allocate memory for node in linked list. Halting...\n",
                      __func__);
        halt(1);    //memory allocation failed
    }

    /*** Update Node Values ***/
    /* Pid Value */
    pNew->pid = pid;

    /*** Add New Node to List ***/
    pNew->pNextProc = NULL;                     //assign pNext to NULL

    if (ListIsEmpty(NULL, pq))      //if list is empty...
    {
        pq->pHeadProc = pNew;                   //add pNew to front of list
    }
    else                                        //if list not empty...
    {
        pq->pTailProc->pNextProc = pNew;        //add pNew to end of list
        pNew->pPrevProc = pq->pTailProc;        //assign pNew prev to current tail
    }

    pq->pTailProc = pNew;                       //reassign pTail to new node
    pq->total++;                                //update pq count

    /*** Function Termination ***/
    return;     // List Addition Success

}/* AddProcessLL */


/* ------------------------------------------------------------------------
    Name -          RemoveProcessLL
    Purpose -       Removes Process from Specified Process Queue
    Parameters -    pid:    Process ID
                    pq:     Pointer to Process Queue
    Returns -       Pid of Removed Process, halt (1) on error
    Side Effects -  None
   ----------------------------------------------------------------------- */
int RemoveProcessLL(int pid, procQueue * pq)
{
    /*** Function Initialization ***/
    ProcList * pSave;   //Pointer for Linked List
    int pidFlag = 0;    //verify pid found in list
    int remFlag = 0;    //verify process removed successfully

    /*** Error Check: List is Empty ***/
    if (ListIsEmpty(NULL, pq))
    {
        DebugConsole2("%s: Queue is empty.\n", __func__);
        halt(1);
    }

    /*** Find PID and Remove from List ***/
    pSave = pq->pHeadProc;          //Set pSave to head of list

    while (pSave != NULL)           //verify not end of list
    {
        if (pSave->pid == pid)      //if found pid to remove...
        {
            pidFlag = 1;            //Trigger Flag - PID found

            /** If pid to be removed is at head **/
            if (pSave == pq->pHeadProc)
            {
                pq->pHeadProc = pSave->pNextProc;           //Set next pid as head
                pSave->pNextProc = NULL;                    //Set old head next pointer to NULL

                /* if proc is NOT only node on list */
                if (pq->pHeadProc != NULL)
                {
                    pq->pHeadProc->pPrevProc = NULL;        //Set new head prev pointer to NULL
                }

                    /* if proc IS only node on list */
                else
                {
                    pSave->pPrevProc = NULL;    //Set old prev pointer to NULL
                    pq->pTailProc = NULL;       //Set new tail to NULL
                }

                remFlag = 1;                    //Trigger Flag - Process Removed
                break;
            }

                /** If pid to be removed is at tail **/
            else if (pSave == pq->pTailProc)
            {
                pq->pTailProc = pSave->pPrevProc;       //Set prev pid as tail
                pSave->pPrevProc = NULL;                //Set old tail prev pointer to NULL
                pq->pTailProc->pNextProc = NULL;        //Set new tail next pointer to NULL
                remFlag = 1;                            //Trigger Flag - Process Removed
                break;
            }

                /** If pid to be removed is in middle **/
            else
            {
                pSave->pPrevProc->pNextProc = pSave->pNextProc; //Set pSave prev next to pSave next
                pSave->pNextProc->pPrevProc = pSave->pPrevProc; //Set pSave next prev to pSave prev
                pSave->pNextProc = NULL;    //Set old pNext to NULL
                pSave->pPrevProc = NULL;    //Set old pPrev to NULL
                remFlag = 1;                //Trigger Flag - Process Removed
                break;
            }
        }
        else                            //if pid not found
        {
            pSave = pSave->pNextProc;   //increment to next in list
        }
    }//end while loop

    /*** Decrement Counter ***/
    pq->total--;

    /*** Error Check: Pid Found and Removed ***/
    if (pidFlag && remFlag)
    {
        pSave = NULL;       //free() causing issues in USLOSS
    }
    else
    {
        if (pidFlag == 0)
        {
            DebugConsole2("%s: Unable to locate pid [%d]. Halting...\n",
                          __func__, pid);
            halt(1);
        }
        else if (remFlag == 0)
        {
            DebugConsole2("%s: Removal of pid [%d] unsuccessful. Halting...\n",
                          __func__, remFlag);
            halt(1);
        }
    }

    /*** Function Termination ***/
    return pid;     //process removal success

}/* RemoveProcessLL */


/* ------------------------------------------------------------------------
    Name -          CopyProcessLL
    Purpose -       Copies process from sourceQueue to destQueue
    Parameters -    pid:            Process ID
                    sourceQueue:    Pointer to Source Process Queue
                    destQueue:      Pointer to Destination Process Queue
    Returns -       pointer to copied process in destination
    Side Effects -  None
   ----------------------------------------------------------------------- */
proc_ptr CopyProcessLL(int pid, procQueue * sourceQueue, procQueue * destQueue)
{
    /*** Function Initialization ***/
    proc_ptr pSrcProc;      // Pointer to Proc in Source Queue
    proc_ptr pDstProc;      // Pointer to Proc in Dest Queue

    /*** Add Process to Destination ***/
    AddProcessLL(pid, destQueue);

    /*** Update Values ***/
    /* Find Process in Source Queue */
    pSrcProc = FindProcessLL(pid, sourceQueue);

    /* Find Process in Destination Queue */
    pDstProc = FindProcessLL(pid, destQueue);

    /* Compare Values and Update on Mismatch */
    if (pDstProc->zeroSlot != pSrcProc->zeroSlot)
    {
        pDstProc->zeroSlot = pSrcProc->zeroSlot;
    }
    if (pDstProc->delivered != pSrcProc->delivered)
    {
        pDstProc->delivered = pSrcProc->delivered;
    }
    if (pDstProc->slot_id != pSrcProc->slot_id)
    {
        pDstProc->slot_id = pSrcProc->slot_id;
    }

    /*** Remove Process from Destination ***/
    RemoveProcessLL(pSrcProc->pid, sourceQueue);

    /*** Function Termination ***/
    return pDstProc;     //process copy success

} /* CopyProcessLL */


/* ------------------------------------------------------------------------
    Name -          FindProcessLL
    Purpose -       Finds Process inside Specified Process Queue
    Parameters -    pid:    Process ID
                    pq:     Pointer to Process Queue
    Returns -       Pointer to found Process
    Side Effects -  None
   ----------------------------------------------------------------------- */
proc_ptr FindProcessLL(int pid, procQueue * pq)
{
    /*** Function Initialization ***/
    ProcList * pSave;   //Pointer for Linked List
    int pidFlag = 0;    //verify pid found in list

    /*** Error Check: List is Empty ***/
    if (ListIsEmpty(NULL, pq))
    {
        DebugConsole2("%s: Queue is empty.\n", __func__);
        return NULL;
    }

    /*** Find PID ***/
    pSave = pq->pHeadProc;          //Set pSave to head of list

    while (pSave->pid != 0)         //verify not end of list
    {
        if (pSave->pid == pid)      //if found pid...
        {
            /*** Function Termination ***/
            return pSave;           //process found

        }
        else                        //if pid not found
        {
            pSave = pSave->pNextProc;   //increment to next in list
        }
    }//end while

    /*** Function Termination ***/
    return NULL;     //process not found

}/* FindProcessLL */

/* --------------------- END OF Linked List Functions --------------------- */