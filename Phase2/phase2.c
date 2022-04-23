/* ------------------------------------------------------------------------
    phase2.c
    Applied Technology
    College of Applied Science and Technology
    The University of Arizona
    CSCV 452

    Kiera Conway
    Katelyn Griffith

   ------------------------------------------------------------------------ */

#include <stdlib.h>
#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include "message.h"

/** ------------------------------ Prototypes ------------------------------ **/
int start1 (char *);
extern int start2 (char *);
int check_io();

/*** Helper Functions ***/
int HelperSend(int mbox_id, void *msg, int msg_size, int block_flag);
int HelperReceive(int mbox_id, void *msg, int max_msg_size, int block_flag);

/*** Functions Brought in From Phase 1 ***/
void enableInterrupts();
void disableInterrupts();
void check_kernel_mode(const char *functionName);
void DebugConsole2(char *format, ...);

/*** Handler Initializers ***/
void ClockIntHandler2(int dev, void *arg);
void DiskIntHandler(int dev, void *arg);
void TermIntHandler(int dev, void *arg);
void SyscallIntHandler(int dev, void *arg);

/** ------------------------------ Globals ------------------------------ **/

/*Flags*/
extern int debugFlag;                   //used for debugging
extern int releaseWasBlocked;           //used to check if releaser was blocked

/* Mailbox Globals */
extern mailbox MailboxTable[MAXMBOX];   //mailbox table
extern int totalMailbox;                //total mailboxes
extern unsigned int next_mid;           //next mailbox id [mbox_id]

/* Slot Globals */
extern slot_table SlotTable[MAXSLOTS];  //slot table
extern int totalActiveSlots;            //total active slot count
extern unsigned int next_sid;           //next slot id [slot_id]

/* Interrupt Handler Globals */
int io_mbox[7];                                 //array of interrupt handlers
void (*sys_vec[MAXSYSCALLS])(sysargs *args);    //system call handler

/** ----------------------------- Functions ----------------------------- **/

/* ------------------------------------------------------------------------
  Name -           start1
  Purpose -        Initializes mailboxes and interrupt vector,
                   Starts the phase2 test process.
  Parameters -     *arg:   default arg passed by fork1, not used here.
  Returns -        0:      indicates normal quit
  Side Effects -   lots since it initializes the phase2 data structures.
  ----------------------------------------------------------------------- */
int start1(char *arg)
{
    /*** Function Initialization ***/
    check_kernel_mode(__func__);    // Check for kernel mode
    disableInterrupts();        // Disable interrupts
    int kid_pid, status;        // Holds forked pid and status

    /*** Initialize Global Tables ***/
    memset(MailboxTable, 0, sizeof(MailboxTable));  //Mailbox Table
    memset(SlotTable, 0, sizeof(SlotTable));        //Slot Table

    /*** Initialize Individual Interrupts ***/
    int_vec[CLOCK_INT] = &clock_handler2;       // 0 - clock
    int_vec[DISK_INT] = &disk_handler;          // 2 - disk
    int_vec[TERM_INT] = &term_handler;          // 3 - terminal
    int_vec[SYSCALL_INT] = &syscall_handler;    // 5 - syscall

    /*** Initialize System Call Vector ***/
    for (int i = 0; i < MAXSYSCALLS; i++)
    {
        sys_vec[i] = nullsys;
    }

    /*** Allocate Mailboxes for Handlers ***/
    io_mbox[0] = MboxCreate(0, sizeof(int));        //Clock Handler
    io_mbox[1] = MboxCreate(0, sizeof(int));        //Disk Handlers
    io_mbox[2] = MboxCreate(0, sizeof(int));
    io_mbox[3] = MboxCreate(0, sizeof(int));        //Terminal Handlers
    io_mbox[4] = MboxCreate(0, sizeof(int));
    io_mbox[5] = MboxCreate(0, sizeof(int));
    io_mbox[6] = MboxCreate(0, sizeof(int));

    /*** Enable Interrupts ***/
    enableInterrupts();

    /*** Create Process for Start2 ***/
    kid_pid = fork1("start2", start2, NULL, 4 * USLOSS_MIN_STACK, 1);

    /*** Join until Start2 Quits ***/
    if ( join(&status) != kid_pid )
    {
        DebugConsole2("%s: join something other than start2's pid\n", __func__ );
        halt(1);
    }

    /*** Function Termination ***/
    return 0;

} /* start1 */


/* ------------------------------------------------------------------------
   Name -           MboxCreate
   Purpose -        Creates a mailbox
   Parameters -     slots:      maximum number of mailbox slots
                    slot_size:   max size of a msg sent to the mailbox
   Returns -        -1: no mailbox available, slot size incorrect
                  >= 0: Unique mailbox ID
   Side Effects -   initializes one element of the mail box array.
   ----------------------------------------------------------------------- */
int MboxCreate(int slots, int slot_size)
{
    /*** Function Initialization ***/
    check_kernel_mode(__func__);    // Check for kernel mode
    disableInterrupts();    // Disable interrupts
    int newMid;             //new mailbox id
    int mboxIndex;          //new mailbox index in Mailbox Table

    /*** Error Check: Mailbox Availability ***/
    if(totalMailbox >= MAXMBOX){
        DebugConsole2("%s : No Mailboxes Available.\n", __func__);
        enableInterrupts();
        return -1;
    }

    /*** Error Check: Invalid Parameters ***/
    /* Verify Slot Size */
    if (slot_size < 0 || slot_size > MAXMESSAGE)
    {
        DebugConsole2("%s : The specified slot size [%d] is out of range [0 - %d].\n",
                      __func__, slots, MAXMESSAGE);
        enableInterrupts();
        return -1;
    }

    /* Verify Slot Amount */
    if (slots < 0 || slots > MAXSLOTS)
    {
        DebugConsole2("%s : The specified slot amount [%d] is out of range [0 - %d].\n",
                      __func__, slots, MAXSLOTS);
        enableInterrupts();
        return -1;
    }

    /*** Add New Mailbox to Mailbox Table ***/
    /* Get next empty mailbox from mailboxTable */
    if ((newMid = GetNextMailID()) == -1)        //get next mbox ID
    {
        /* Error Check: Exceeding Max Processes */
        DebugConsole2("%s : Attempting to exceed MAXMBOX limit [%d].\n",
                      __func__, slots, MAXMBOX);
        enableInterrupts();
        return -1;
    }

    /* Calculate Mailbox Index */
    mboxIndex = GetMboxIndex(newMid);

    /*Initialize Mailbox*/
    InitializeMailbox(MBOX_USED,
                      mboxIndex,
                      newMid,
                      slots,
                      slot_size);

    /*** Update Counter ***/
    totalMailbox += 1;                          //Increment Mailbox Count

    /*** Function Termination ***/
    enableInterrupts();                         // Enable interrupts
    return MailboxTable[mboxIndex].mbox_id;     //Return Unique mailbox ID

} /* MboxCreate */


/* ------------------------------------------------------------------------
   Name -           MboxSend
   Purpose -        Send a message to a mailbox
   Parameters -     mbox_id:    Unique mailbox ID
                    msg_ptr:    Pointer to the Message
                    msg_size:   Size of Message in Bytes
   Returns -        -3: process is zapped,
                        mailbox was released while the process
                        was blocked on the mailbox.
                    -1: Invalid Parameters Given
                     0: Message Sent Successfully
   Side Effects -   none
   ----------------------------------------------------------------------- */
int MboxSend(int mbox_id, void *msg_ptr, int msg_size)
{
    return HelperSend(mbox_id,msg_ptr, msg_size, 0);

} /* MboxSend */


/* ------------------------------------------------------------------------
   Name -           MboxReceive
   Purpose -        Receive a message from a mailbox
   Parameters -     mbox_id:        Unique Mailbox ID
                    msg_ptr:        Pointer to the Message
                    max_msg_size:   Max Storage Size for Message in Bytes
   Returns -        -3: process is zapped,
                        mailbox was released while the process
                        was blocked on the mailbox.
                    -1: Invalid Parameters Given,
                        Sent Message Size Exceeds Limit
                    >0: Size of Message Received
   Side Effects -   none.
   ----------------------------------------------------------------------- */
int MboxReceive(int mbox_id, void *msg_ptr, int max_msg_size)
{
    return HelperReceive(mbox_id, msg_ptr, max_msg_size, 0);

} /* MboxReceive */


/* ------------------------------------------------------------------------
   Name -           MboxRelease
   Purpose -        Releases a previously created mailbox
   Parameters -     mbox_id:  Unique Mailbox ID
   Returns -        -3:  Process was Zapped while Releasing the Mailbox
                    -1:  Invalid Mailbox ID
                     0:  Success
   Side Effects -
   ----------------------------------------------------------------------- */
int MboxRelease(int mbox_id)
{
    /*** Function Initialization ***/
    check_kernel_mode(__func__);    // Check for kernel mode
    disableInterrupts();                        // Disable interrupts
    int mboxIndex = GetMboxIndex(mbox_id);      // Get Mailbox index in MailboxTable
    mailbox *pMbox = &MailboxTable[mboxIndex];  // Create Mailbox pointer

    /*** Error Check: Invalid Parameters ***/
    /* Check mbox_id is Valid */
    if(mbox_id < 0 || pMbox->mbox_id != mbox_id)
    {
        DebugConsole2("%s : Invalid Mailbox ID [%d].\n",
                      __func__, mbox_id);
        enableInterrupts();
        return -1;
    }

    /*** Error Check: Unused Mailbox ***/
    if(pMbox->status == EMPTY || pMbox->status == MBOX_RELEASED)
    {
        DebugConsole2("%s: Attempting to release an already empty mailbox.",
                      __func__);
        enableInterrupts();
        return -1;
    }

    /*** Clear out Mailbox Slot ***/
    pMbox->status = MBOX_RELEASED;  //update released status
    pMbox->releaser = getpid();     //update releaser pid
    int mboxSlots = pMbox->activeSlots;
    HelperRelease(pMbox);

    /*** Update Global Counters ***/
    totalActiveSlots -= mboxSlots;  //decrement active
    totalMailbox--;                 //decrement mailbox

    /*** Error Check: Process was Zapped ***/
    if(is_zapped())
    {
        DebugConsole2("%s: Process was zapped while it was blocked",
                      __func__);
        enableInterrupts();
        return -3;
    }

    /*** Clear Mailbox Values ***/
    InitializeMailbox(EMPTY,
                      pMbox->mbox_index,
                      NULL,
                      NULL,
                      NULL);

    /*** Function Termination ***/
    enableInterrupts();     // Enable interrupts
    return 0;               // Successful Release

} /* MboxRelease */


/* ------------------------------------------------------------------------
   Name -           MboxCondSend
   Purpose -        Send a message to a mailbox
   Parameters -     mailboxID:      Unique Mailbox ID
                    *message:       Pointer to the Message
                    message_size:   Size of Message in Bytes
   Returns -        -3: Process is Zapped,
                    -2: Mailbox is Full, Message not Sent;
                        No Mailbox Slots Available in System.
                    -1: Invalid Parameters Given,
                     0: Message Sent Successfully.
   Side Effects -   none
   ----------------------------------------------------------------------- */
int MboxCondSend(int mailboxID, void *message, int message_size)
{
    return HelperSend(mailboxID, message, message_size, 1);

} /* MboxCondSend */


/* ------------------------------------------------------------------------
   Name -           MboxCondReceive
   Purpose -
   Parameters -     mailboxID:      Unique Mailbox ID
                    *message:       Pointer to the Message
                    max_msg_size:   Max Storage Size for Message in Bytes
   Returns -        -3: Process is Zapped,
                    -2: Mailbox is Full, Message not Sent;
                        No Mailbox Slots Available in System.
                    -1: Invalid Parameters Given,
                     0: Message Sent Successfully.
   Side Effects -   none
   ----------------------------------------------------------------------- */
int MboxCondReceive(int mailboxID, void *message, int max_msg_size)
{
    return HelperReceive(mailboxID, message, max_msg_size, 1);

} /* MboxCondReceive */


/* ------------------------------------------------------------------------
   Name -           check_io
   Purpose -
   Parameters -     none
   Returns -        1:  At least one process is blocked
                    0:  No blocked processes
   Side Effects -   none
   ----------------------------------------------------------------------- */
int check_io()
{
    /*** Check for Blocked Processes ***/
    for(int i = 0; i < 7; i++)
    {
        if ((MailboxTable[i].waitingToSend.pHeadProc != NULL) ||
            (MailboxTable[i].waitingToReceive.pHeadProc != NULL))
        {
            return 1;
        }
    }
    return 0;

} /* check_io */


/* ------------------------------------------------------------------------
   Name -           waitdevice
   Purpose -
   Parameters -     type:
                    unit:
                    status:
   Returns -        -1: the process was zapped while waiting
                     0: otherwise
   Side Effects -   none
   ----------------------------------------------------------------------- */
int waitdevice(int type, int unit, int *status)
{
    /*** Function Initialization ***/
    check_kernel_mode(__func__);    // Check for kernel mode
    disableInterrupts();                        // Disable interrupts
    int i;
    int result = 0;

    /*** Identify Type ***/
    switch (type)
    {
        case CLOCK_DEV:
            i = 0;
            break;
        case DISK_DEV:
            i = 1;
            break;
        case TERM_DEV:
            i = 3;
            break;
        default:
            DebugConsole2("%s: Bad Type [%d]/ Halting...\n",
                          __func__ , type);
            halt(1);
    }

    result = MboxReceive(io_mbox[i + unit], status, sizeof(int));

    /** Return Error Check: Process was Zapped**/
    enableInterrupts();
    return result == -3 ? -1: 0;

}  /* waitdevice */

/* ------------------------------------------------------------------------
   Name -           HelperSend
   Purpose -
   Parameters -     mbox_id:    Unique mailbox ID
                    *msg:       Pointer to the Message
                    msg_size:   Size of Message in Bytes
                    block_flag:     (1) Blocked, (0) Unblocked
   Returns -        -3: process is zapped,
                        mailbox was released while the process
                        was blocked on the mailbox.
                    -2: Mailbox is Full, Message not Sent;
                        No Mailbox Slots Available in System.
                    -1: Invalid Parameters Given
                     0: Message Sent Successfully
   Side Effects -   none
   ----------------------------------------------------------------------- */
int HelperSend(int mbox_id, void *msg, int msg_size, int block_flag)
{
    /*** Function Initialization ***/
    check_kernel_mode(__func__);    // Check for kernel mode
    disableInterrupts();                        // Disable interrupts
    int mboxIndex = GetMboxIndex(mbox_id);      // Get Mailbox index in MailboxTable
    mailbox *pMbox = &MailboxTable[mboxIndex];  // Create Mailbox pointer
    proc_ptr pCurProc;                          // Pointer to Current Process

    DebugConsole2("%s: Function successfully entered.\n",
                  __func__);

    /*** Error Check: Invalid Parameters ***/
    /* Check mbox_id is Valid */
    if(mbox_id < 0 || pMbox->mbox_id != mbox_id || pMbox->status == MBOX_RELEASED)
    {
        DebugConsole2("%s : Invalid Mailbox ID [%d].\n",
                      __func__, mbox_id);
        enableInterrupts();
        return -1;
    }

    /* Check msg is Valid */
    if (msg == NULL)
    {
        /* msg_size is not 0 */
        if (msg_size > 0)
        {
            DebugConsole2("%s : Invalid message pointer.\n",
                          __func__);
            enableInterrupts();
            return -1;
        }
    }

    /* Check msg_size */
    if(msg_size > pMbox->maxMsgSize || msg_size < 0)
    {
        DebugConsole2("%s : The specified message size [%d] is out of range [0 - %d].\n",
                      __func__, msg_size, pMbox->maxMsgSize);
        enableInterrupts();
        return -1;
    }

    /*** Add to Slot Table ***/
    /* determine pSlot location **/
    int slotID = GetNextSlotID();                   //find next available id
    int slotIndex = GetSlotIndex(slotID);    //find index
    slot_table * pSlot = &SlotTable[slotIndex];      //set pointer to slot

    /* Copy Information to SlotTable */
    InitializeSlot(SLOT_FULL,
                   slotIndex,
                   slotID,
                   msg,
                   msg_size,
                   pMbox->mbox_id);

    /* Increment Global Counter */
    totalActiveSlots++;

    /*** Update Process ***/
    /* Add Process to Active Procs **/
    AddProcessLL(getpid(),&pMbox->activeProcs);

    /* Create Process Pointer */
    pCurProc = FindProcessLL(getpid(), &pMbox->activeProcs);

    /* Assign Starting Values */
    pCurProc->delivered = false;    //msg not delivered yet
    pCurProc->slot_id = slotID;

    /*** Check if Zero-Slot Mailbox ***/
    if (pMbox->maxSlots == 0)
    {
        /** Update Zero Slot **/
        pCurProc->zeroSlot = pSlot;

        /** Check Waiting Process **/
        /* If No Processes Waiting */
        if (ListIsEmpty(NULL, &pMbox->waitingToReceive))
        {
            /** Block Current Process **/
            /* Copy Process to Wait List */
            CopyProcessLL(getpid(),
                          &pMbox->activeProcs,
                          &pMbox->waitingToSend);

            /* Block Sending Process */
            block_me(SEND_BLOCK);

            /** Error Check: Mailbox was Released **/
            if(MboxWasReleased(pMbox))
            {
                DebugConsole2("%s: Mailbox was released.",
                              __func__);
                enableInterrupts();
                return -3;
            }

            /** Error Check: Process was Zapped while Blocked **/
            if (is_zapped())
            {
                DebugConsole2("%s: Process was zapped while it was blocked.",
                              __func__);

                /* Remove Current Process from Active List */
                RemoveProcessLL(getpid(),
                                &pMbox->activeProcs);
                enableInterrupts();
                return -3;
            }

        }

        /** Check Message Delivery **/
        /* Find Process in Active Process List */
        pCurProc = FindProcessLL(getpid(), &pMbox->activeProcs);

        /* If Current Process Message Not Delivered */
        if(!pCurProc->delivered)
        {
            /* If Processes Waiting */
            if (!ListIsEmpty(NULL, &pMbox->waitingToReceive))
            {
                /** Unblock Waiting Process **/
                /* Copy to Active List */
                proc_ptr pWaitProc = CopyProcessLL(pMbox->waitingToReceive.pHeadProc->pid,
                                                   &pMbox->waitingToReceive,
                                                   &pMbox->activeProcs);

                /* Send Message and Update Flag */
                pWaitProc->zeroSlot = pSlot;    //slot table pointer
                pWaitProc->delivered = true;    //trigger delivered flag

                /* Unblock Process */
                unblock_proc(pWaitProc->pid);

            }
            /* If No Processes Waiting: Message not Delivered*/
            else {
                DebugConsole2("%s: No process waiting to receive.\n",
                              __func__);
                RemoveProcessLL(getpid(),
                                &pMbox->activeProcs);
                enableInterrupts();
                return -3;
            }
        }

        /** Error Check: Mailbox was Released **/
        if(MboxWasReleased(pMbox))
        {
            DebugConsole2("%s: Mailbox was released.",
                          __func__);
            enableInterrupts();
            return -3;
        }

        /** Error Check: Process was Zapped while Blocked **/
        if (is_zapped())
        {
            DebugConsole2("%s: Process was zapped while it was blocked.",
                          __func__);
            RemoveProcessLL(getpid(),
                            &pMbox->activeProcs);
            enableInterrupts();
            return -3;
        }

        /** Function Termination **/
        RemoveProcessLL(getpid(),
                        &pMbox->activeProcs);
        enableInterrupts();     // Enable interrupts
        return 0;               // Zero-Slot Message Sent Successfully

    }
    /* Not Zero Slot */
    else
    {
        pCurProc->zeroSlot = NULL;
    }

    /*** Verify Slot is Available ***/
    /* mboxSend: slot is full */
    if (block_flag == 0 && pMbox->activeSlots >= pMbox->maxSlots)
    {
        /** Block Current Process **/
        /* Copy Process to Wait List */
        CopyProcessLL(getpid(),
                      &pMbox->activeProcs,
                      &pMbox->waitingToSend);

        /* Block Sending Process */
        block_me(SEND_BLOCK);

        /** Error Check: Mailbox was Released **/
        if(MboxWasReleased(pMbox))
        {
            DebugConsole2("%s: Mailbox was released.",
                          __func__);
            enableInterrupts();
            return -3;
        }

        /** Error Check: Process was Zapped while Blocked **/
        if (is_zapped())
        {
            DebugConsole2("%s: Process was zapped while it was blocked.",
                          __func__);

            /* Remove Current Process from Active List */
            RemoveProcessLL(getpid(),
                            &pMbox->activeProcs);
            enableInterrupts();
            return -3;
        }
    }
    /* mboxCondSend: slot is full */
    else if (pMbox->activeSlots >= pMbox->maxSlots)
    {
        DebugConsole2("%s : Mailbox Slot Queue is full, unable to send message.\n",
                      __func__);

        /* Remove Current Process from Active List */
        RemoveProcessLL(getpid(),
                        &pMbox->activeProcs);
        enableInterrupts();
        return -2;
    }

    /*** Error Check: System Slot is Full ***/
    if (totalActiveSlots > MAXSLOTS)
    {
        DebugConsole2("%s: No Active Slots Available in System,"
                      " unable to send message.\n", __func__);

        /* Remove Current Process from Active List */
        RemoveProcessLL(getpid(),
                        &pMbox->activeProcs);
        enableInterrupts();
        return -2;
    }

    /*** Add Slot pointer in Mailbox Queue ***/
    /* Find Process in Active Process List */
    pCurProc = FindProcessLL(getpid(), &pMbox->activeProcs);

    /* Add to Process Queue if not Already Added */
    if(pCurProc->delivered == false)
    {
        if ((AddSlotLL(pSlot, pMbox)) == -1)
        {
            DebugConsole2("%s: Slot Queue is full.\n", __func__);

            /* Remove Current Process from Active List */
            RemoveProcessLL(getpid(),
                            &pMbox->activeProcs);
            return -3;
        }
    }
    /* Update Delivered Flag */
    pCurProc->delivered = true;

    /*** Check for Waiting Processes ***/
    if(!ListIsEmpty(NULL, &pMbox->waitingToReceive))
    {
        /** Send Message to Waiting Process **/
        /* Find Receiving Process in Waiting Process List */
        proc_ptr pRecvProc = pMbox->waitingToReceive.pHeadProc;

        /* Send Message and Update Flag */
        pRecvProc->slot_id = slotID;    //slot queue index
        pRecvProc->delivered = true;    //trigger delivered flag

        /** Unblock Waiting Process **/
        /* Copy to Active List */
        pRecvProc = CopyProcessLL(pRecvProc->pid,
                                  &pMbox->waitingToReceive,
                                  &pMbox->activeProcs);

        /* Unblock Process */
        unblock_proc(pRecvProc->pid);
    }

    /*** Function Termination ***/
    /* Remove Current Process from Active List */
    RemoveProcessLL(getpid(),
                    &pMbox->activeProcs);
    enableInterrupts();     // Enable interrupts
    return 0;               // Message Sent Successfully

} /* HelperSend */


/* ------------------------------------------------------------------------
   Name -           HelperReceive
   Purpose -
   Parameters -     mbox_id:    Unique mailbox ID
                    *msg:       Pointer to the Message
                    max_msg_size:   Max Storage Size for Message in Bytes
                    block_flag:     (1) Blocked, (0) Unblocked
   Returns -        -3: process is zapped,
                        mailbox was released while the process
                        was blocked on the mailbox.
                    -1: Invalid Parameters Given,
                        Sent Message Size Exceeds Limit
                    >0: Size of Message Received
   Side Effects -   none
   ----------------------------------------------------------------------- */
int HelperReceive(int mbox_id, void *msg, int max_msg_size, int block_flag)
{
    /*** Function Initialization ***/
    check_kernel_mode(__func__);    // Check for kernel mode
    disableInterrupts();                        // Disable interrupts
    int sentMsgSize = 0;                        // Size of message sent
    int mboxIndex = GetMboxIndex(mbox_id);      //Get Mailbox index in MailboxTable
    mailbox *pMbox = &MailboxTable[mboxIndex];  // Create Mailbox pointer
    proc_ptr pCurProc;                          // Pointer to Current Process
    slot_ptr pSlot;                             // Pointer to Slot in Mailbox Queue

    /*** Error Check: Invalid Parameters ***/
    /* Check mbox_id is Valid */
    if(mbox_id < 0 || pMbox->mbox_id != mbox_id || pMbox->status == MBOX_RELEASED)
    {
        DebugConsole2("%s : Invalid Mailbox ID [%d].\n",
                      __func__, mbox_id);

        enableInterrupts();
        return -1;
    }

    /* Check max_msg_size */
    if(max_msg_size > MAXMESSAGE || max_msg_size < 0)
    {
        DebugConsole2("%s : Invalid maximum message size [%d].\n",
                      __func__, max_msg_size);

        enableInterrupts();
        return -1;
    }

    /*** Error Check: Mailbox was Released ***/
    if(pMbox->status == MBOX_RELEASED)
    {
        DebugConsole2("%s : Invalid Mailbox,.\n",
                      __func__, max_msg_size);
        enableInterrupts();
        return -1;
    }

    /*** Update Process ***/
    /* Add Process to Active Procs **/
    AddProcessLL(getpid(),&pMbox->activeProcs);

    /* Create Process Pointer */
    pCurProc = FindProcessLL(getpid(), &pMbox->activeProcs);

    /* Assign Starting Values */
    pCurProc->delivered = false;    //msg not delivered yet
    pCurProc->slot_id = -1;

    /*** Check if Zero-Slot Mailbox ***/
    if(pMbox->maxSlots == 0)
    {
        /** Check Waiting Process **/
        /* If No Processes Waiting */
        if(ListIsEmpty(NULL, &pMbox->waitingToSend))
        {
            /** Block Current Process **/
            /* Copy Process to Wait List */
            CopyProcessLL(getpid(),
                          &pMbox->activeProcs,
                          &pMbox->waitingToReceive);

            /* Block Receiving Process */
            block_me(RECEIVE_BLOCK);

            /** Error Check: Mailbox was Released **/
            if(MboxWasReleased(pMbox))
            {
                DebugConsole2("%s: Mailbox was released.",
                              __func__);
                enableInterrupts();
                return -3;
            }

            /** Error Check: Process was Zapped while Blocked **/
            if (is_zapped())
            {
                DebugConsole2("%s: Process was zapped while it was blocked.",
                              __func__);

                /* Remove Current Process from Active List */
                RemoveProcessLL(getpid(),
                                &pMbox->activeProcs);
                enableInterrupts();
                return -3;
            }
        }

        /** Check Message Delivery **/
        /* Find Process in Active Process List */
        pCurProc = FindProcessLL(getpid(), &pMbox->activeProcs);

        /* If Current Process Message Not Delivered */
        if(!pCurProc->delivered)
        {
            /* If Processes Waiting */
            if (!ListIsEmpty(NULL, &pMbox->waitingToSend))
            {
                /*** Error Check: Sent Message Size Exceeds Limit ***/
                sentMsgSize = pMbox->waitingToSend.pHeadProc->zeroSlot->messageSize;

                if (sentMsgSize > max_msg_size)
                {
                    DebugConsole2("%s : Sent message size [%d] exceeds maximum receive size [%d] .\n",
                                  __func__, sentMsgSize, max_msg_size);

                    /* Remove Current Process from Active List */
                    RemoveProcessLL(getpid(),
                                    &pMbox->activeProcs);
                    enableInterrupts();
                    return -1;
                }

                /*** Retrieve Message ***/
                /* copy message into buffer */
                memcpy(msg,
                       &pMbox->waitingToSend.pHeadProc->zeroSlot->message,
                       sentMsgSize);

                /*** Update Tables and Queues ***/
                /* save index of pHead for InitializeSlot() */
                int waitSlotIndex = pMbox->waitingToSend.pHeadProc->zeroSlot->slot_index;

                /* Remove Slot from SlotTable */
                InitializeSlot(EMPTY,
                               waitSlotIndex,
                               -1,
                               NULL,
                               -1,
                               -1);

                /* Decrement Global Counter */
                totalActiveSlots--;

                /** Unblock Waiting Process **/
                /* Copy to Active List */
                proc_ptr pWaitProc = CopyProcessLL(pMbox->waitingToSend.pHeadProc->pid,
                                                   &pMbox->waitingToSend,
                                                   &pMbox->activeProcs);

                /* Update Delivered Flag */
                pWaitProc->delivered = true;    //trigger delivered flag

                /* Unblock Process */
                unblock_proc(pWaitProc->pid);

            }
            else
            {
                DebugConsole2("%s: No process waiting to receive.",
                              __func__);

                /* Remove Current Process from Active List */
                RemoveProcessLL(getpid(),
                                &pMbox->activeProcs);
                enableInterrupts();
                return -3;
            }

            /** Error Check: Mailbox was Released **/
            if (MboxWasReleased(pMbox))
            {
                enableInterrupts();
                return -3;
            }

            /** Error Check: Process was Zapped while Blocked **/
            if (is_zapped())
            {
                DebugConsole2("%s: Process was zapped while it was blocked.",
                              __func__);

                /* Remove Current Process from Active List */
                RemoveProcessLL(getpid(),
                                &pMbox->activeProcs);
                enableInterrupts();
                return -3;
            }
        }
        /* If Current Process Message Delivered */
        else
        {
            /** Error Check: Sent Message Size Exceeds Limit **/
            /* Find Process in Active Process List */
            pCurProc = FindProcessLL(getpid(), &pMbox->activeProcs);
            sentMsgSize = pCurProc->zeroSlot->messageSize;

            if (sentMsgSize > max_msg_size)
            {
                DebugConsole2("%s : Sent message size [%d] exceeds maximum receive size [%d] .\n",
                              __func__, sentMsgSize, max_msg_size);

                /* Remove Current Process from Active List */
                RemoveProcessLL(getpid(),
                                &pMbox->activeProcs);
                enableInterrupts();
                return -1;
            }

            /** Copy Message into Buffer */
            pCurProc = FindProcessLL(getpid(), &pMbox->activeProcs);    //update currentProc
            memcpy(msg, &pCurProc->zeroSlot->message, sentMsgSize);

            /*** Update Tables and Queues ***/
            /* save index of pHead for InitializeSlot() */
            int waitSlotIndex = pCurProc->zeroSlot->slot_index;

            /* Remove Slot from SlotTable */
            InitializeSlot(EMPTY,
                           waitSlotIndex,
                           -1,
                           NULL,
                           -1,
                           -1);

            /* Decrement Global Counter */
            totalActiveSlots--;
        }

        /** Function Termination **/
        /* Remove Current Process from Active List */
        RemoveProcessLL(getpid(),
                        &pMbox->activeProcs);
        enableInterrupts();     // Enable interrupts
        return sentMsgSize;     // Zero-Slot Message Recvd Successfully
    }

    /* Update Current Pointer */
    pCurProc = FindProcessLL(getpid(), &pMbox->activeProcs);

    /*** Verify Message is Available ***/
    /* mboxReceive: no message available */
    if (block_flag == 0 && pMbox->activeSlots <= 0)
    {
        /** Block Current Process **/
        /* Copy Process to Wait List */
        CopyProcessLL(getpid(),
                      &pMbox->activeProcs,
                      &pMbox->waitingToReceive);

        /* Block Receiving Process */
        block_me(RECEIVE_BLOCK);

        /** Error Check: Mailbox was Released **/
        if(MboxWasReleased(pMbox))
        {
            DebugConsole2("%s: Mailbox was released.",
                          __func__);
            enableInterrupts();
            return -3;
        }

        /** Error Check: Process was Zapped while Blocked **/
        if (is_zapped())
        {
            DebugConsole2("%s: Process was zapped while it was blocked.",
                          __func__);

            /* Remove Current Process from Active List */
            RemoveProcessLL(getpid(),
                            &pMbox->activeProcs);
            enableInterrupts();
            return -3;
        }
    }
    /* mboxCondReceive no message available */
    else if(pMbox->activeSlots <= 0)
    {
            DebugConsole2("%s : No message available in mailbox.\n",
                          __func__);

            /* Remove Current Process from Active List */
            RemoveProcessLL(getpid(),
                            &pMbox->activeProcs);
            enableInterrupts();
            return -2;
    }

    /*** Update Current Proccess Pointer After Block ***/
    pCurProc = FindProcessLL(getpid(), &pMbox->activeProcs);

    /*** Check if Message already Received ***/
    /* If Message is Not Already Received */
    if(pCurProc->slot_id == -1)
    {
        /* Set pSLot to Head of Slot Queue */
        pSlot = pMbox->slotQueue.pHeadSlot;
    }
    /* If Message is Already Received */
    else
    {
        /* Set pSLot from Slot Index */
        pSlot = FindSlotLL(pCurProc->slot_id, &pMbox->slotQueue);
    }

    /*** Error Check: Sent Message Size Exceeds Limit ***/
    sentMsgSize = pSlot->slot->messageSize;

    if (sentMsgSize > max_msg_size)
    {
        DebugConsole2("%s : Sent message size [%d] exceeds maximum receive size [%d] .\n",
                      __func__, sentMsgSize, max_msg_size);

        /* Remove Current Process from Active List */
        RemoveProcessLL(getpid(),
                        &pMbox->activeProcs);
        enableInterrupts();
        return -1;
    }

    /*** Retrieve Message ***/
    /* copy message into buffer */
    memcpy(msg, &pSlot->slot->message, sentMsgSize);

    /*** Update Tables and Queues ***/
    /* save index of pHead for InitializeSlot() */
    int slotIndex = pSlot->slot->slot_index;

    /* Remove Slot from Queue */
    if((RemoveSlotLL(pSlot->slot_id, &pMbox->slotQueue)) == -1)
    {
        DebugConsole2("%s: Unable to receive message - mailbox empty. Halting...",
                      __func__);

        /* Remove Current Process from Active List */
        RemoveProcessLL(getpid(),
                        &pMbox->activeProcs);
        return -3;
    }

    /* Remove Slot from SlotTable */
    InitializeSlot(EMPTY,
                   slotIndex,
                   -1,
                   NULL,
                   -1,
                   -1);

    /* Decrement Global Counter */
    totalActiveSlots--;

    /*** Unblock Waiting Processes ***/
    if(!ListIsEmpty(NULL, &pMbox->waitingToSend))
    {
        /*** Add Sender Slot pointer to Mailbox Queue ***/
        int sendSlotIndex = GetSlotIndex(pMbox->waitingToSend.pHeadProc->slot_id);    //find index
        slot_table * pSendSlot = &SlotTable[sendSlotIndex];      //set pointer to slot

        if((AddSlotLL(pSendSlot, pMbox)) == -1)
        {
            DebugConsole2("%s: Slot Queue is full.\n", __func__);

            /* Remove Current Process from Active List */
            RemoveProcessLL(getpid(),
                            &pMbox->activeProcs);
            return -3;
        }

        /* Increment Global Counter */
        totalActiveSlots++;

        /** Unblock Waiting Process **/
        /* Copy to Active List */
        proc_ptr pWaitProc = CopyProcessLL(pMbox->waitingToSend.pHeadProc->pid,
                                           &pMbox->waitingToSend,
                                           &pMbox->activeProcs);

        /* Update Delivered Flag */
        pWaitProc->delivered = true;    //trigger delivered flag

        /* Unblock Process */
        unblock_proc(pWaitProc->pid);
    }

    /*** Function Termination ***/
    /* Remove Current Process from Active List */
    RemoveProcessLL(getpid(),
                    &pMbox->activeProcs);
    enableInterrupts();     // Enable interrupts
    return sentMsgSize;     //Return size of message received

} /* HelperReceive */


/* ------------------------------------------------------------------------
   Name -           enableInterrupts
   Purpose -        Enable System Interrupts
   Parameters -     none
   Returns -        none
   Side Effects -   Modifies PSR Values
   ----------------------------------------------------------------------- */
void enableInterrupts()
{
    /*** Function Initialization ***/
    check_kernel_mode(__func__);    // Check for kernel mode

    int curPsr = psr_get();

    curPsr = curPsr | PSR_CURRENT_INT;

    psr_set(curPsr);

} /* enableInterrupts */


/* ------------------------------------------------------------------------
   Name -           disableInterrupts
   Purpose -        Disable System Interrupts
   Parameters -     none
   Returns -        none
   Side Effects -   Modifies PSR Values
   ----------------------------------------------------------------------- */
void disableInterrupts()
{
    /* turn the interrupts OFF if we are in kernel mode */
    if((PSR_CURRENT_MODE & psr_get()) == 0)
    {
        //not in kernel mode
        console("Kernel Error: Not in kernel mode, may not disable interrupts. Halting...\n");
        halt(1);
    }
    else
    {
        /* We ARE in kernel mode */
        psr_set(psr_get() & ~PSR_CURRENT_INT);
    }
} /* disableInterrupts */


/* ------------------------------------------------------------------------
   Name -           check_kernel_mode
   Purpose -        Checks the PSR for kernel mode and halts if in user mode
   Parameters -     functionName:   Function to Verify
   Returns -        none
   Side Effects -   Halts if not kernel mode
   ----------------------------------------------------------------------- */
void check_kernel_mode(const char *functionName)
{
    union psr_values psrValue; /* holds caller's psr values */
    int curPid = getpid();

    /* test if in kernel mode; halt if in user mode */
    psrValue.integer_part = psr_get();  //returns int value of psr

    if (psrValue.bits.cur_mode == 0)
    {
        /* if currently running function */
        if (curPid > 0 )
        {
            int cur_proc_slot = curPid % MAXMBOX;
            DebugConsole2("%s(): called while in user mode, by process %d. Halting...\n",
                          functionName, cur_proc_slot);
        }
        /* if no currently running function, startup is running */
        else
        {
            DebugConsole2("%s(): called while in user mode, by startup(). Halting...\n",
                          functionName);
        }

        halt(1);
    }

} /* check_kernel_mode */


/* ------------------------------------------------------------------------
   Name -           DebugConsole2
   Purpose -        Prints the message to the console if in debug mode
   Parameters -     *format:    Format String to Display
                    ...:        Optional Corresponding Arguments
   Returns -        none
   Side Effects -   none
   ----------------------------------------------------------------------- */
void DebugConsole2(char *format, ...)
{
    if (DEBUG2 && debugFlag)
    {
        va_list argptr;
        va_start(argptr, format);
        vfprintf(stdout, format, argptr);
        fflush(stdout);
        va_end(argptr);
    }

} /* DebugConsole2 */