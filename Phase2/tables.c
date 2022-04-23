/*  tables.c handles mailbox table and slot table */

#include "tables.h"

/* -------------------------------- Globals -------------------------------- */

/* Flags */
int debugFlag = 0;              //used for debugging
int releaseWasBlocked = 0;

/* Mailbox Globals */
mailbox MailboxTable[MAXMBOX];  //mailbox table
int totalMailbox;               //total mailboxes
unsigned int next_mid;          //next mailbox id [mbox_id]

/* Slot Globals */
slot_table SlotTable[MAXSLOTS]; //slot table
int totalActiveSlots = 0;       //total active slot count
unsigned int next_sid = 0;      //next slot id [slot_id]

/* -------------------------- Functions ----------------------------------- */

/* * * * * * * * * * * * Mailbox Table Functions * * * * * * * * * * * */

/* ------------------------------------------------------------------------
   Name -           GetNextMailID
   Purpose -        Finds next available mbox_id
   Parameters -     none
   Returns -        >0: next mbox_id
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetNextMailID()
{
    int newMid = -1;
    int mboxIndex = GetMboxIndex(next_mid);

    if (totalMailbox < MAXMBOX)
    {
        while ((totalMailbox < MAXMBOX) && (MailboxTable[mboxIndex].status != EMPTY))
        {
            next_mid++;
            mboxIndex = next_mid % MAXMBOX;
        }

        newMid = next_mid % MAXMBOX;    //assign newPid *then* increment next_pid
    }

    return newMid;

} /* GetNextMailID */


/* ------------------------------------------------------------------------
   Name -           InitializeMailbox
   Purpose -        Initializes the new mailbox
   Parameters -     newStatus:  New status being assigned
                    mbox_index: Location inside MailBoxTable
                    mbox_id:    Unique Mailbox ID
                    maxSlots:   Maximum number of slots
                    maxMsgSize:   Maximum size of slot/ message
   Returns -        none
   Side Effects -   Mailbox added to MailboxTable
   ----------------------------------------------------------------------- */
void InitializeMailbox(int newStatus, int mbox_index, int mbox_id, int maxSlots, int maxMsgSize) {

    //if removing a mailbox from MailboxTable
    if(newStatus == EMPTY)
    {
        mbox_id = -1;
        maxSlots = -1;
        maxMsgSize = -1;
    }

    mailbox * pMbox = &MailboxTable[mbox_index];

    pMbox->mbox_index = mbox_index;             //Mailbox Slot
    pMbox->mbox_id = mbox_id;                   //Mailbox ID
    pMbox->status = newStatus;                  //Mailbox Status
    InitializeList(NULL,
                   &pMbox->activeProcs);        //Waiting Process List
    InitializeList(NULL,
                   &pMbox->waitingToSend);      //Waiting to Send List
    InitializeList(NULL,
                   &pMbox->waitingToReceive);   //Waiting to Receive List
    pMbox->activeProcs.mbox_id = mbox_id;       //MailboxID inside Active Queue
    pMbox->waitingToSend.mbox_id = mbox_id;     //MailboxID inside Send Queue
    pMbox->waitingToReceive.mbox_id = mbox_id;  //MailboxID inside Receive Queue
    pMbox->maxSlots = maxSlots;                 //Max Slots
    pMbox->activeSlots = 0;                     //Active Slots
    pMbox->maxMsgSize = maxMsgSize;             //Slot Size
    InitializeList(&pMbox->slotQueue,
                   NULL);                 //Slot List
    pMbox->slotQueue.mbox_id = mbox_id;          //MailboxID inside Slot Queue

} /* InitializeMailbox */


/* ------------------------------------------------------------------------
   Name -           GetMboxIndex
   Purpose -        Gets Mailbox index from id
   Parameters -     mbox_id:    Unique Mailbox ID
   Returns -        Corresponding Index
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetMboxIndex(int mbox_id)
{
    return mbox_id % MAXMBOX;

} /* GetMboxIndex */

/* * * * * * * * * * * END OF Mailbox Table Functions * * * * * * * * * * */



/* * * * * * * * * * * Mailbox Table Release Functions * * * * * * * * * * */

/* ------------------------------------------------------------------------
   Name -           HelperRelease
   Purpose -        A Helper Function for mboxRelease
   Parameters -     pMbox:    Pointer to Mailbox
   Returns -        none
   Side Effects -   none
   ----------------------------------------------------------------------- */
void HelperRelease(mailbox *pMbox)
{
    /*** Function Initialization ***/
    pMbox->totalProcs = pMbox->waitingToSend.total + pMbox->waitingToReceive.total;

    /*** Unblock Receiving Processes ***/
    while (pMbox->waitingToReceive.total > 0)
    {
        /** Unblock Waiting Process **/
        /* Copy to Active List */
        proc_ptr pRecvProc = CopyProcessLL(pMbox->waitingToReceive.pHeadProc->pid,
                                           &pMbox->waitingToReceive,
                                           &pMbox->activeProcs);

        /* Unblock Process */
        unblock_proc(pRecvProc->pid);

        disableInterrupts();
    }

    /*** Unblock Sending Processes ***/
    while (pMbox->waitingToSend.total > 0)
    {
        /** Unblock Waiting Process **/
        /* Copy to Active List */
        proc_ptr pSendProc = CopyProcessLL(pMbox->waitingToSend.pHeadProc->pid,
                                           &pMbox->waitingToSend,
                                           &pMbox->activeProcs);

        /* Unblock Process */
        unblock_proc(pSendProc->pid);

        disableInterrupts();
    }

    /*** Verify Unblocked Processes Completed ***/
    if (pMbox->totalProcs > 0)
    {
        releaseWasBlocked = 1;
        block_me(RELEASE_BLOCK);
    }

    return;

} /* HelperRelease */


/* ------------------------------------------------------------------------
   Name -           MboxWasReleased
   Purpose -        Checks if mailbox was released
   Parameters -     pMbox:    Pointer to Mailbox
   Returns -        false:  Mailbox was not released
                    true:   Mailbox was released
   Side Effects -   none
   ----------------------------------------------------------------------- */
bool MboxWasReleased(mailbox *pMbox)
{
    /*** Check if Mailbox was Released ***/
    if(pMbox->status == MBOX_RELEASED)
    {
        /** Check Remaining Processes **/
        /* Current is Last Process */
        if (pMbox->totalProcs == 1)
        {
            /* Decrement totalProcs */
            pMbox->totalProcs--;

            /* Unblock Releaser */
            if (releaseWasBlocked == 1)
            {
                unblock_proc(pMbox->releaser);
                disableInterrupts();
            }
        }
            /* Multiple Processes Remaining */
        else
        {
            /* Remove Current Process from Active List */
            RemoveProcessLL(getpid(),
                            &pMbox->activeProcs);
            pMbox->totalProcs--;
        }

        /*** Function Termination ***/
        enableInterrupts();
        return true;    //Mailbox was released
    }

    return false;       //Mailbox was not released

} /* MboxWasReleased */

/* * * * * * * * * * END OF Mailbox Table Release Functions * * * * * * * * * */


/* * * * * * * * * * * * Slot Table Functions * * * * * * * * * * * */

/* ------------------------------------------------------------------------
   Name -           GetNextSlotID
   Purpose -        Finds next available slot_id
   Parameters -     none
   Returns -        >0: next mbox_id
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetNextSlotID()
{
    int newSid = -1;
    int slotIndex = GetSlotIndex(next_sid);

    if (totalActiveSlots < MAXSLOTS)
    {
        while ((totalActiveSlots < MAXSLOTS) && (SlotTable[slotIndex].status != EMPTY))
        {
            next_sid++;
            slotIndex = next_sid % MAXSLOTS;
        }

        newSid = next_sid;    //assign newPid *then* increment next_pid
    }

    return newSid;

} /* GetMextSlotID */


/* ------------------------------------------------------------------------
   Name -           InitializeSlot
   Purpose -        Initializes slot to match passed parameters.
   Parameters -     newStatus:  New status being assigned
                    slot_index: Location inside SlotTable
                    *msg_ptr
                    mbox_id:    Unique Mailbox ID
                    maxSlots:   Maximum number of slots
                    maxMsgSize:   Maximum size of slot/ message
   Returns -        none
   Side Effects -   Mailbox added to MailboxTable
   ----------------------------------------------------------------------- */
void InitializeSlot(int newStatus, int slot_index, int slot_id, void *msg_ptr, int msgSize, int mbox_id)
{
    //if removing slot from SlotTable
    //verify parameters are -1
    if(newStatus == EMPTY) {
        slot_id = -1;
        msgSize = -1;
        mbox_id = -1;
    }

    slot_table * pSlot = &SlotTable[slot_index];

    pSlot->slot_index = slot_index;     //Location in SlotTable
    pSlot->slot_id = slot_id;           //Unique Slot ID
    pSlot->status = newStatus;          //Assign New Status
    pSlot->mbox_id = mbox_id;           //Mailbox ID of Created Mailbox
    pSlot->messageSize = msgSize;       //Size of contained message


    /*Adding a Message*/
    if (newStatus != EMPTY)
    {
        if ((mbox_id >= 1) && (mbox_id <= 6))
        {
            memcpy(pSlot->message, &msg_ptr, msgSize);
        }
        else
        {
            memcpy(pSlot->message, msg_ptr, msgSize);
        }
    }
        /*Removing a Message*/
    else
    {
        memset(pSlot->message, '\0', sizeof(pSlot->message));
    }

} /* InitializeSlot */

/* ------------------------------------------------------------------------
   Name -           GetSlotIndex
   Purpose -        Gets Slot index from id
   Parameters -     slot_id:    Unique Slot ID
   Returns -        none
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetSlotIndex(int slot_id)
{
    return slot_id % MAXSLOTS;

} /* GetSlotIndex */

/* * * * * * * * * * * END OF Mailbox Table Functions * * * * * * * * * * */


/* ------------------------ END OF Table Functions ------------------------ */