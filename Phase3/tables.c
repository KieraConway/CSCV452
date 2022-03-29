/*  tables.c handles process table and sem table functions*/

#include "tables.h"

/* -------------------------------- Globals -------------------------------- */

/* Flags */
int debugFlag = 0;              //used for debugging


/* Process Globals */
usr_proc_struct UsrProcTable[MAXPROC];  //Process Table
int totalProc;           //total Processes
extern unsigned int nextPID;    //next process id

/* Sem Globals */
//todo

/* -------------------------- Functions ----------------------------------- */

/* * * * * * * * * * * * Process Table Functions * * * * * * * * * * * */

/* ------------------------------------------------------------------------
   Name -           GetNextPID
   Purpose -        Finds next available pid
   Parameters -     none
   Returns -        >0: next pid
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetNextPID()
{
//    int newMid = -1;
//    int mboxIndex = GetMboxIndex(next_mid);
//
//    if (totalMailbox < MAXMBOX)
//    {
//        while ((totalMailbox < MAXMBOX) && (MailboxTable[mboxIndex].status != EMPTY))
//        {
//            next_mid++;
//            mboxIndex = next_mid % MAXMBOX;
//        }
//
//        newMid = next_mid % MAXMBOX;    //assign newPid *then* increment next_pid
//    }
//
//    return newMid;

} /* GetNextPID */


/* ------------------------------------------------------------------------
   Name -           InitializeProcTable
   Purpose -        Initializes the new process
   Parameters -
   Returns -        none
   Side Effects -   Process added to ProcessTable
   ----------------------------------------------------------------------- */
void InitializeProcTable() {

    //todo: update in phase3_helper.h
//    //if removing a mailbox from MailboxTable
//    if(newStatus == EMPTY)
//    {
//        mbox_id = -1;
//        maxSlots = -1;
//        maxMsgSize = -1;
//    }
//
//    mailbox * pMbox = &MailboxTable[mbox_index];
//
//    pMbox->mbox_index = mbox_index;             //Mailbox Slot
//    pMbox->mbox_id = mbox_id;                   //Mailbox ID
//    pMbox->status = newStatus;                  //Mailbox Status
//    InitializeList(NULL,
//                   &pMbox->activeProcs);        //Waiting Process List
//    InitializeList(NULL,
//                   &pMbox->waitingToSend);      //Waiting to Send List
//    InitializeList(NULL,
//                   &pMbox->waitingToReceive);   //Waiting to Receive List
//    pMbox->activeProcs.mbox_id = mbox_id;       //MailboxID inside Active Queue
//    pMbox->waitingToSend.mbox_id = mbox_id;     //MailboxID inside Send Queue
//    pMbox->waitingToReceive.mbox_id = mbox_id;  //MailboxID inside Receive Queue
//    pMbox->maxSlots = maxSlots;                 //Max Slots
//    pMbox->activeSlots = 0;                     //Active Slots
//    pMbox->maxMsgSize = maxMsgSize;             //Slot Size
//    InitializeList(&pMbox->slotQueue,
//                   NULL);                 //Slot List
//    pMbox->slotQueue.mbox_id = mbox_id;          //MailboxID inside Slot Queue

} /* InitializeProcTable */


/* ------------------------------------------------------------------------
   Name -           GetProcIndex
   Purpose -        Gets Process index from pid
   Parameters -     pid:    Unique Process ID
   Returns -        Corresponding Index
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetProcIndex(int pid)
{
    return pid % MAXPROC;

} /* GetProcIndex */

/* * * * * * * * * * * END OF Process Table Functions * * * * * * * * * * */



/* * * * * * * * * * * * Sem Table Functions * * * * * * * * * * * */

/* * * * * * * * * * * END OF Sem Table Functions * * * * * * * * * * */


/* ------------------------ END OF Table Functions ------------------------ */