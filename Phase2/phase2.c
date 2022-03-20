/* ------------------------------------------------------------------------
    phase2.c
    Applied Technology
    College of Applied Science and Technology
    The University of Arizona
    CSCV 452

    Kiera Conway
    Katelyn Griffith

    NOTES:
        UPDATE clearMboxSlot NAME ***************
            clearMboxSlot - clears ALL slots
            RemoveSlotLL -  removes single Slot
                rename one or both to prevent confusion*******
        Going to need to work on phase2.c and handler.c
        term files work like message queues for phase 2
        need symbollic link for phase 2?
        need libmjdphase1 file for phase 2
        update makefile for phase1lib to mjdphase2

        check_kernel_mode
        enableInterrupts
        disableInterupts
   ------------------------------------------------------------------------ */
#include <stdlib.h>
#include <phase1.h>
#include <phase2.h>
#include <usloss.h>

#include "message.h"

/* ------------------------- Prototypes ----------------------------------- */
int start1 (char *);
extern int start2 (char *);
int check_io();

/*** Helper Functions ***/
int HelperSend(int mbox_id, void *msg, int msg_size, int block_flag);
int HelperReceive(int mbox_id, void *msg, int max_msg_size, int block_flag);
void HelperRelease(mailbox *pMbox);

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

/*** Global Table Functions ***/
/* Mailbox Table */
int GetNextMailID();
void InitializeMailbox(int newStatus,
                       int mbox_index,
                       int mbox_id,
                       int maxSlots,
                       int maxMsgSize);
int GetMboxIndex(int mbox_id);
bool MboxWasReleased(mailbox *pMbox);

/* Slot Table */
int GetNextSlotID();
void InitializeSlot(int newStatus,
                    int slot_index,
                    int slot_id,
                    void *msg_ptr,
                    int msgSize,
                    int mbox_id);
int GetSlotIndex(int slot_id);

/*** Linked List Functions ***/
/* General, Multi-use */
void InitializeList(slotQueue *pSlot,
                    procQueue *pProc);          //Initialize Linked List
bool ListIsFull(const slotQueue *pSlot,
                const mailbox *pMbox,
                const procQueue *pProc);        //Check if Slot List is Full
bool ListIsEmpty(const slotQueue *pSlot,
                 const procQueue *pProc);       //Check if Slot List is Empty

/* Slot Lists */
int AddSlotLL(slot_table * pSlot,
              mailbox * pMbox);                  //Add pSlot to Mailbox Queue
int RemoveSlotLL(slotQueue * pSlot,
                 mailbox * pMbox);               //Remove pSlot from Mailbox Queue

/* Process Lists */
void AddProcessLL(int pid, procQueue * pq);     //Add pid to Process Queue
int RemoveProcessLL(int pid, procQueue * pq);   //Remove pid from Process Queue

/* -------------------------- Globals ------------------------------------- */

/*Flags*/
int debugFlag2 = 0;                 //used for debugging
int releaseWasBlocked = 0;

/*Mailbox Globals*/
mailbox MailboxTable[MAXMBOX];      //mailbox table
int totalMailbox;                   //total mailboxes
unsigned int next_mid = 0;          //next mailbox id [mbox_id]

/*Slot Globals*/
slot_table SlotTable[MAXSLOTS];     //slot table
int totalActiveSlots = 0;           //total active slot count
unsigned int next_sid = 0;          //next slot id [slot_id]

/*Interrupt Handler Globals*/
int io_mbox[7];
void (*sys_vec[MAXSYSCALLS])(sysargs *args);
//int clock_mbox;                     //clock interrupt handler
//int disk_mbox[2];                   //disk interrupt handler
//int term_mbox[4];                   //terminal interrupt handler
//void (*sys_vec[])(sysargs *args);
/* -------------------------- Functions ----------------------------------- */

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
    int kid_pid, status;

    DebugConsole2("%s: at beginning\n", __func__);

    /* Check for kernel mode */
    check_kernel_mode("start1");

    /* Disable interrupts */
    disableInterrupts();

    /*Initialize the global Tables*/
    memset(MailboxTable, 0, sizeof(MailboxTable));  //Mailbox Table
    memset(SlotTable, 0, sizeof(SlotTable));        //Slot Table

    /*Initialize individual interrupts in the interrupt vector*/
    int_vec[CLOCK_INT] = &clock_handler2;       // 0 - clock
    int_vec[DISK_INT] = &disk_handler;          // 2 - disk
    int_vec[TERM_INT] = &term_handler;          // 3 - terminal
    int_vec[SYSCALL_INT] = &syscall_handler;    // 5 - syscall

    /*Initialize the system call vector*/
    for(int i = 0; i < MAXSYSCALLS; i++){
        sys_vec[i] = nullsys;
    }

    /*Allocate Mailboxes for Interrupt Handlers */
    io_mbox[0] = MboxCreate(0, sizeof(int));  //Clock
    io_mbox[1] = MboxCreate(0, sizeof(int));  //Disk
    io_mbox[2] = MboxCreate(0, sizeof(int));
    io_mbox[3] = MboxCreate(0, sizeof(int));  //Terminal
    io_mbox[4] = MboxCreate(0, sizeof(int));
    io_mbox[5] = MboxCreate(0, sizeof(int));
    io_mbox[6] = MboxCreate(0, sizeof(int));

    /* Enable interrupts */
    enableInterrupts();

    DebugConsole2("%s: fork'ing start2 process\n", __func__);

    /*Create Process for start2*/
    kid_pid = fork1("start2", start2, NULL, 4 * USLOSS_MIN_STACK, 1);

    /*Join until Start2 quits*/
    if ( join(&status) != kid_pid ) {
        DebugConsole2("%s: join something other than start2's pid\n", __func__);
        halt(1);
    }

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
    if((newMid = GetNextMailID()) == -1){           //get next mbox ID

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

    /*** Update Counters ***/
    //totalActiveSlots += slots;  //Increment totalActiveSlots
    totalMailbox += 1;          //Increment Mailbox Count

    /*** Function Termination ***/
    enableInterrupts();                         // Enable interrupts
    return MailboxTable[mboxIndex].mbox_id;     //Return Unique mailbox ID

} /* MboxCreate */


/* ------------------------------------------------------------------------
   Name -           MboxSend
   Purpose -        Send a message to a mailbox
   Parameters -     mbox_id:    Unique mailbox ID
                    msg_ptr:    pointer to the message
                    msg_size:   size of message in bytes
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
    int mboxIndex = GetMboxIndex(mbox_id);      //Get Mailbox index in MailboxTable
    mailbox *pMbox = &MailboxTable[mboxIndex];  // Create Mailbox pointer

    DebugConsole2("%s: Function successfully entered.\n",
                  __func__);

    /*** Error Check: Invalid Parameters ***/
    /* Check mbox_id is Valid */
    if(mbox_id < 0 || pMbox->mbox_id != mbox_id){
        DebugConsole2("%s : Invalid Mailbox ID [%d].\n",
                      __func__, mbox_id);
        enableInterrupts();
        return -1;
    }

    /*** Error Check: Unused Mailbox ***/
    if(pMbox->status == EMPTY || pMbox->status == MBOX_RELEASED){
        DebugConsole2("%s: Attempting to release an already empty mailbox.",
                      __func__);
        enableInterrupts();
        return -1;
    }

    /*** Clear out Mailbox Slot ***/
    pMbox->status = MBOX_RELEASED;  //update released status
    pMbox->releaser = getpid();     //update releaser pid
    int mboxSlots = pMbox->activeSlots;
    //CheckBlockLists(pMbox); todo: remove
    HelperRelease(pMbox);

    /*** Update Global Counters ***/
    totalActiveSlots -= mboxSlots;  //decrement active
    totalMailbox--;                 //decrement mailbox

    /*** Error Check: Process was Zapped ***/
    if(is_zapped()){
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
 * TODO
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
    for(int i = 0; i < 7; i++){
        if (MailboxTable[i].waitingToSend.pHeadProc != NULL || MailboxTable[i].waitingToReceive.pHeadProc != NULL){
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
int waitdevice(int type, int unit, int *status){

    /*** Function Initialization ***/
    check_kernel_mode(__func__);    // Check for kernel mode
    disableInterrupts();                        // Disable interrupts
    int i;

    /*** Identify Type ***/
    switch (type) {
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

    MboxReceive(io_mbox[i + unit], status, sizeof(int));

    /** Return Error Check: Process was Zapped**/
    enableInterrupts();
    return is_zapped() ? -1: 0;

//    /*** Function Initialization ***/
//    int result = 0;
//
//    /*** Identify Type ***/
//    switch(type){
//        case CLOCK_DEV:
//            MboxReceive(0, status, sizeof(int));
//            device_input(type,unit, status);
//            break;
//        case DISK_DEV:
//            result = MboxReceive(disk_mbox[unit], status, sizeof(int));
//            //DeviceInput(type,unit, status); todo ?
//            break;
//        case TERM_DEV:
//            result = MboxReceive(term_mbox[unit], status, sizeof(int));
//            //DeviceInput(type,unit, status); todo ?
//        default:
//            DebugConsole2("%s : Bad Type (%d). Halting...\n",
//                          __func__, type);
//            halt(1);
//    }
//    /** Error Check: Process was Zapped**/
//    if(result == -3){
//        DebugConsole2("%s: Process was zapped while it was blocked.",
//                      __func__);
//        return -1;
//    }
//    else{
//        /*** Function Termination ***/
//        return 0;               // Successful
//    }

}  /* waitdevice */

/* ------------------------------------------------------------------------
 * TODO fix return in title
   Name -           HelperSend
   Purpose -
   Parameters -     mbox_index:      MailboxTable Index
                    *msg:           Pointer to the Message
                    msg_size:       Size of Message in Bytes
                    block_flag:     (1) Blocked, (0) Unblocked
   Returns -        none
   Side Effects -   none
   ----------------------------------------------------------------------- */
int HelperSend(int mbox_id, void *msg, int msg_size, int block_flag)
{
    // fill me

    //if message already there, there are no differences between functions
    //block_flag - should proc wait or ?
    //if no block, return immediately?
    /*** Function Initialization ***/
    check_kernel_mode(__func__);    // Check for kernel mode
    disableInterrupts();                        // Disable interrupts
    int mboxIndex = GetMboxIndex(mbox_id);      //Get Mailbox index in MailboxTable
    mailbox *pMbox = &MailboxTable[mboxIndex];  // Create Mailbox pointer

    DebugConsole2("%s: Function successfully entered.\n",
                  __func__);

    /*** Error Check: Invalid Parameters ***/
    /* Check mbox_id is Valid */
    if(mbox_id < 0 || pMbox->mbox_id != mbox_id){
        DebugConsole2("%s : Invalid Mailbox ID [%d].\n",
                      __func__, mbox_id);
        enableInterrupts();
        return -1;
    }

    /* Check msg is Valid */
    if (msg == NULL) {
        if (msg_size > 0) {

            //Note:Null is only invalid for non-zero slot mailboxes

            DebugConsole2("%s : Invalid message pointer.\n",
                          __func__);
            enableInterrupts();
            return -1;
        }
    }

    /* Check msg_size */
    if(msg_size > pMbox->maxMsgSize || msg_size < 0){
        DebugConsole2("%s : The specified message size [%d] is out of range [0 - %d].\n",
                      __func__, msg_size, pMbox->maxMsgSize);
        enableInterrupts();
        return -1;
    }

    /*** Check if Zero-Slot Mailbox ***/
    if (pMbox->maxSlots == 0) {

        /** Check Waiting Process **/
        /* If No Processes Waiting */
        if (ListIsEmpty(NULL, &pMbox->waitingToReceive)) {
            AddProcessLL(getpid(),
                         &pMbox->waitingToSend);    //add proc to waiting list
            block_me(SEND_BLOCK);                       //block sending process

            /** Error Check: Mailbox was Released **/
            if(MboxWasReleased(pMbox)){
                DebugConsole2("%s: Mailbox was released.",
                              __func__);
                enableInterrupts();
                return -3;
            }

            /** Error Check: Process was Zapped while Blocked **/
            if (is_zapped()) {
                DebugConsole2("%s: Process was zapped while it was blocked.",
                              __func__);
                enableInterrupts();
                return -3;
            }

        }

        /* If Processes Waiting */
        if (!ListIsEmpty(NULL, &pMbox->waitingToReceive)) {
            int pid = RemoveProcessLL(pMbox->waitingToReceive.pHeadProc->pid,
                                      &pMbox->waitingToReceive);    //remove from list
            unblock_proc(pid);                                          //unlock process
        }
        else {
            DebugConsole2("%s: No process waiting to receive.\n",
                          __func__);
            enableInterrupts();
            return -3;
        }

        /** Error Check: Mailbox was Released **/
        if(MboxWasReleased(pMbox)){
            DebugConsole2("%s: Mailbox was released.",
                          __func__);
            enableInterrupts();
            return -3;
        }

        /** Error Check: Process was Zapped while Blocked **/
        if (is_zapped()) {
            DebugConsole2("%s: Process was zapped while it was blocked.",
                          __func__);
            enableInterrupts();
            return -3;
        }

        /** Function Termination **/
        enableInterrupts();     // Enable interrupts
        return 0;               // Zero-Slot Message Sent Successfully

        //todo:
        //if message already there, there are no differences between functions
        //block_flag - should proc wait or ?
        //if no block, return immediately?
        //return;
    }

    /*** Verify Slot is Available ***/
    /* mboxSend: slot is full */
    if (block_flag == 0 && pMbox->activeSlots >= pMbox->maxSlots) {  //if exceeding max slots
        AddProcessLL(getpid(), &pMbox->waitingToSend);      //add proc to waiting list
        DebugConsole2("%s: Entering a SEND_BLOCK.",
                      __func__);
        block_me(SEND_BLOCK);

        /** Error Check: Mailbox was Released **/
        if(MboxWasReleased(pMbox)){
            DebugConsole2("%s: Mailbox was released.",
                          __func__);
            enableInterrupts();
            return -3;
        }

        /** Error Check: Process was Zapped while Blocked **/
        if (is_zapped()) {
            DebugConsole2("%s: Process was zapped while it was blocked.",
                          __func__);
            enableInterrupts();
            return -3;
        }
    }
    /* mboxCondSend: slot is full */
    else if (pMbox->activeSlots >= pMbox->maxSlots) {
        DebugConsole2("%s : Mailbox Slot Queue is full, unable to send message.\n",
                      __func__);
        enableInterrupts();
        return -2;
    }

    /*** Error Check: System Slot is Full ***/
    if (totalActiveSlots >= MAXSLOTS) {
        DebugConsole2("%s: No Active Slots Available in System,"
                      " unable to send message.\n", __func__);
        enableInterrupts();
        return -2;
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

    /*** Add Slot pointer in Mailbox Queue ***/
    if((AddSlotLL(pSlot, pMbox)) == -1){
        DebugConsole2("%s: Slot Queue is full.\n", __func__);
        return -3;
    }

    /*** Unblock Waiting Processes ***/
    if(!ListIsEmpty(NULL, &pMbox->waitingToReceive)){
        int pid = RemoveProcessLL(pMbox->waitingToReceive.pHeadProc->pid,
                                  &pMbox->waitingToReceive);    //remove from list
        unblock_proc(pid);                                          //unlock process
    }

    /*** Function Termination ***/
    enableInterrupts();     // Enable interrupts
    return 0;               // Message Sent Successfully

} /* HelperSend */

/* ------------------------------------------------------------------------
   Name -           HelperReceive
   Purpose -
   Parameters -     mbox_index:      MailboxTable Index
                    *msg:           Pointer to the Message
                    msg_size:       Size of Message in Bytes
                    block_flag:     (1) Blocked, (0) Unblocked
   Returns -        none
   Side Effects -   none
   ----------------------------------------------------------------------- */
int HelperReceive(int mbox_id, void *msg, int max_msg_size, int block_flag)
{
    /*** Function Initialization ***/
    check_kernel_mode(__func__);    // Check for kernel mode
    disableInterrupts();                        // Disable interrupts
    int mboxIndex = GetMboxIndex(mbox_id);      //Get Mailbox index in MailboxTable
    mailbox *pMbox = &MailboxTable[mboxIndex];  // Create Mailbox pointer

    DebugConsole2("%s: Function successfully entered.\n",
                  __func__);

    /*** Error Check: Invalid Parameters ***/
    /* Check mbox_id is Valid */
    if(mbox_id < 0 || pMbox->mbox_id != mbox_id){
        DebugConsole2("%s : Invalid Mailbox ID [%d].\n",
                      __func__, mbox_id);
        enableInterrupts();
        return -1;
    }

    /* Check max_msg_size */
    if(max_msg_size > MAXMESSAGE || max_msg_size < 0){
        DebugConsole2("%s : Invalid maximum message size [%d].\n",
                      __func__, max_msg_size);
        enableInterrupts();
        return -1;
    }

    /*** Error Check: Mailbox was Released ***/
    if (MboxWasReleased(pMbox)) {
        enableInterrupts();
        return -3;
    }

    /*** Check if Zero-Slot Mailbox ***/
    if(pMbox->maxSlots == 0){

        /** Check Waiting Process **/
        /* If No Processes Waiting */
        if(ListIsEmpty(NULL, &pMbox->waitingToSend)){
            AddProcessLL(getpid(),
                         &pMbox->waitingToReceive);   //add proc to waiting list
            block_me(RECEIVE_BLOCK);                      //block sending process

            /** Error Check: Mailbox was Released **/
            if(MboxWasReleased(pMbox)){
                DebugConsole2("%s: Mailbox was released.",
                              __func__);
                enableInterrupts();
                return -3;
            }

            /** Error Check: Process was Zapped while Blocked **/
            if (is_zapped()) {
                DebugConsole2("%s: Process was zapped while it was blocked.",
                              __func__);
                enableInterrupts();
                return -3;
            }
        }

        /* If Processes Waiting */
        if(!ListIsEmpty(NULL, &pMbox->waitingToSend)){
            int pid = RemoveProcessLL(pMbox->waitingToSend.pHeadProc->pid,
                                      &pMbox->waitingToSend);    //remove from list
            unblock_proc(pid);                                          //unlock process
        }
        else{
            DebugConsole2("%s: No process waiting to receive.",
                          __func__);
            enableInterrupts();
            return -3;
        }

        /** Error Check: Mailbox was Released **/
        if(MboxWasReleased(pMbox)){
            enableInterrupts();
            return -3;
        }

        /** Error Check: Process was Zapped while Blocked **/
        if(is_zapped()){
            DebugConsole2("%s: Process was zapped while it was blocked.",
                          __func__);
            enableInterrupts();
            return -3;
        }

        /** Function Termination **/
        enableInterrupts();     // Enable interrupts
        return pMbox->slotQueue.pHeadSlot->slot->messageSize; // Zero-Slot Message Recvd Successfully
    }

    /*** Verify Message is Available ***/
    /* mboxReceive: no message available */
    if (block_flag == 0 && pMbox->activeSlots <= 0) {                    //if no message available
        AddProcessLL(getpid(), &pMbox->waitingToReceive);   //add proc to waiting list
        DebugConsole2("%s: Entering a RECEIVE_BLOCK.",
                      __func__);
        block_me(RECEIVE_BLOCK);                                    //block receiving process

        /** Error Check: Mailbox was Released **/
        if(MboxWasReleased(pMbox)){
            DebugConsole2("%s: Mailbox was released.",
                          __func__);
            enableInterrupts();
            return -3;
        }

        /** Error Check: Process was Zapped while Blocked **/
        if (is_zapped()) {
            DebugConsole2("%s: Process was zapped while it was blocked.",
                          __func__);
            enableInterrupts();
            return -3;
        }
    }
    /* mboxCondReceive no message available */
    else if(pMbox->activeSlots <= 0){
            DebugConsole2("%s : No message available in mailbox.\n",
                          __func__);
            enableInterrupts();
            return -2;
    }


    /*** Error Check: Sent Message Size Exceeds Limit ***/
    int sentMsgSize = pMbox->slotQueue.pHeadSlot->slot->messageSize;
    if (sentMsgSize > max_msg_size) {
        DebugConsole2("%s : Sent message size [%d] exceeds maximum receive size [%d] .\n",
                      __func__, sentMsgSize, max_msg_size);
        enableInterrupts();
        return -1;
    }

    /*** Retrieve Message ***/
    /* copy message into buffer */
    memcpy(msg, &pMbox->slotQueue.pHeadSlot->slot->message, sentMsgSize);

    /*** Update Tables and Queues ***/
    /* save index of pHead for InitializeSlot() */
    int slotIndex = pMbox->slotQueue.pHeadSlot->slot->slot_index;

    /* Remove Slot from Queue */
    if((RemoveSlotLL(&pMbox->slotQueue, pMbox)) == -1){
        DebugConsole2("%s: Unable to receive message - mailbox empty. Halting...",
                      __func__);
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
    if(!ListIsEmpty(NULL, &pMbox->waitingToSend)){  //if proc waiting to send
        int pid = RemoveProcessLL(pMbox->waitingToSend.pHeadProc->pid,
                                  &pMbox->waitingToSend);   //remove proc from block list
        unblock_proc(pid);                                      //unlock proc
    }

    /*** Function Termination ***/
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
    check_kernel_mode(__func__);
    /*Confirmed Kernel Mode*/

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
    if((PSR_CURRENT_MODE & psr_get()) == 0) {
        //not in kernel mode
        console("Kernel Error: Not in kernel mode, may not disable interrupts. Halting...\n");
        halt(1);
    } else
        /* We ARE in kernel mode */
        psr_set( psr_get() & ~PSR_CURRENT_INT );
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

    //DebugConsole2("check_kernel_node(): verifying kernel mode for %s\n", functionName);

    /* test if in kernel mode; halt if in user mode */
    psrValue.integer_part = psr_get();  //returns int value of psr

    if (psrValue.bits.cur_mode == 0)
    {
        if (curPid > 0 ){ //if currently running function
            int cur_proc_slot = curPid % MAXMBOX;
            DebugConsole2("%s(): called while in user mode, by process %d. Halting...\n", functionName, cur_proc_slot);
        }
        else {  //if no currently running function, startup is running
            DebugConsole2("%s(): called while in user mode, by startup(). Halting...\n", functionName);
        }
        halt(1);
    }

    //DebugConsole2("Function is in Kernel mode (:\n");
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
    if (DEBUG2 && debugFlag2)
    {
        va_list argptr;
        va_start(argptr, format);
        console(format, argptr);
        va_end(argptr);
    }
}

/* * * * * * * * * * * * Mailbox Table Functions * * * * * * * * * * * */

/* ------------------------------------------------------------------------
   Name -           GetNextMailID
   Purpose -        Finds next available mbox_id
   Parameters -     none
   Returns -        >0: next mbox_id
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetNextMailID() {
    int newMid = -1;
    int mboxIndex = GetMboxIndex(next_mid);

    if (totalMailbox < MAXMBOX) {
        while ((totalMailbox < MAXMBOX) && (MailboxTable[mboxIndex].status != EMPTY)) {
            next_mid++;
            mboxIndex = next_mid % MAXMBOX;
        }

        newMid = next_mid % MAXMBOX;    //assign newPid *then* increment next_pid
    }

    return newMid;
}

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
    if(newStatus == EMPTY) {
        mbox_id = -1;
        maxSlots = -1;
        maxMsgSize = -1;
    }

    mailbox * pMbox = &MailboxTable[mbox_index];

    pMbox->mbox_index = mbox_index;               //Mailbox Slot
    pMbox->mbox_id = mbox_id;                     //Mailbox ID
    pMbox->status = newStatus;                    //Mailbox Status
    InitializeList(NULL, &pMbox->waitingProcs);   //Waiting Process List //todo delete?
    InitializeList(NULL, &pMbox->waitingToSend);  //Waiting to Send List
    InitializeList(NULL, &pMbox->waitingToReceive);//Waiting to Receive List
    pMbox->maxSlots = maxSlots;                   //Max Slots
    pMbox->activeSlots = 0;                       //Active Slots
    pMbox->maxMsgSize = maxMsgSize;               //Slot Size
    InitializeList(&pMbox->slotQueue, NULL);      //Slot List
}
/* ------------------------------------------------------------------------
   Name -           GetMboxIndex
   Purpose -        Gets Mailbox index from id
   Parameters -     mbox_id:    Unique Mailbox ID
   Returns -        none
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetMboxIndex(int mbox_id){
    return mbox_id % MAXMBOX;
}

/* * * * * * * * * * * END OF Mailbox Table Functions * * * * * * * * * * */


/* * * * * * * * * * * * Slot Table Functions * * * * * * * * * * * */

/* ------------------------------------------------------------------------
   Name -           GetNextSlotID
   Purpose -        Finds next available slot_id
   Parameters -     none
   Returns -        >0: next mbox_id
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetNextSlotID() {
    int newSid = -1;
    int slotIndex = GetSlotIndex(next_sid);

    if (totalActiveSlots < MAXSLOTS) {
        while ((totalActiveSlots < MAXSLOTS) && (SlotTable[slotIndex].status != EMPTY)) {
            next_sid++;
            slotIndex = next_sid % MAXSLOTS;
        }

        newSid = next_sid;    //assign newPid *then* increment next_pid
    }

    return newSid;
}


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
void InitializeSlot(int newStatus, int slot_index, int slot_id, void *msg_ptr, int msgSize, int mbox_id){

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
    pSlot->mbox_id = mbox_id;           //Mailbox ID of Created Mailbox     //todo delete?
    pSlot->messageSize = msgSize;       //Size of contained message

    /*Adding a Message*/
    if(newStatus != EMPTY){
        memcpy(pSlot->message, msg_ptr, msgSize);
    }
    /*Removing a Message*/
    else{
        memset(pSlot->message, '\0', sizeof(pSlot->message));
    }
}


/* ------------------------------------------------------------------------
   Name -           GetSlotIndex
   Purpose -        Gets Slot index from id
   Parameters -     slot_id:    Unique Slot ID
   Returns -        none
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetSlotIndex(int slot_id){
    return slot_id % MAXSLOTS;
}

/* * * * * * * * * * * END OF Mailbox Table Functions * * * * * * * * * * */



/* * * * * * * * * * * * * Linked List Functions * * * * * * * * * * * * */

/* ------------------------------------------------------------------------
    Name -          InitializeList
    Purpose -       Initialize List pointed to by parameter
    Parameters -    Meant to be used as 'either or', use NULL for other
                    *pSlot: Pointer to Message Slot List
                    *pProc: Pointer to Process List
    Returns -       None, halt(1) on error
    Side Effects -  None
   ----------------------------------------------------------------------- */
void InitializeList(slotQueue *pSlot, procQueue *pProc)
{
    /*** Initialize Slot Queue ***/
    if(pProc == NULL) {
        /* Initialize the head, tail, and count slot queue */
        pSlot->pHeadSlot = NULL;
        pSlot->pTailSlot = NULL;
        pSlot->total = 0;
        return;
    }

    /*** Initialize Proc Queue ***/
    else if(pSlot == NULL){
        /* Initialize the head, tail, and count proc queue */
        pProc->pHeadProc = NULL;
        pProc->pTailProc = NULL;
        pProc->total = 0;
    }

    /*** Error Check: Invalid Parameters ***/
    else{
        DebugConsole2("%s: Invalid Parameters Passed", __func__);
        halt(1);
    }
}/* InitializeList */

/* ------------------------------------------------------------------------
    Name -          ListIsFull
    Purpose -       Checks if Slot List OR Proc List is Full
    Parameters -    Meant to be used as (pSlot & pMbox) or (pProc)
                    *pSlot: Pointer to Message Slot List
                    *pMbox: Pointer to Mailbox
                    * * * OR * * *
                    *pProc: Pointer to Process List
    Returns -        0: not full
                     1: full
                    -1: list not found
    Side Effects -  none
   ----------------------------------------------------------------------- */
bool ListIsFull(const slotQueue *pSlot, const mailbox *pMbox, const procQueue *pProc){

    /*** Search if Slot Queue is Full ***/
    if(pProc == NULL) {
        return pSlot->total >= pMbox->maxSlots;
    }

    /*** Search if Proc Queue is Full ***/
    else if(pSlot == NULL){
        return pProc->total >= MAXPROC;
    }

    /*** Error Check: Invalid Parameters ***/
    else{
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
                    *pProc: Pointer to Process List
    Returns -        0: not Empty
                     1: Empty
                    -1: list not found
    Side Effects -  none
   ----------------------------------------------------------------------- */
bool ListIsEmpty(const slotQueue *pSlot, const procQueue *pProc){

    /*** Search if Slot Queue is Empty ***/
    if(pProc == NULL) {
        return pSlot->pHeadSlot == NULL;    //checks if pHead is empty
    }

    /*** Search if Proc Queue is Empty ***/
    else if(pSlot == NULL){
        return pProc->pHeadProc == NULL;    //checks if pHead is empty
    }

    /*** Error Check: Invalid Parameters ***/
    else{
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
int AddSlotLL(slot_table * pSlot, mailbox * pMbox){

    /*** Function Initialization ***/
    SlotList * pList;     //new slot list


    /*** Error Check: Verify Slot Space Available***/
    /* check if queue is full */
    if(ListIsFull(&pMbox->slotQueue, pMbox, NULL)){
        DebugConsole2("%s: Slot Queue is full.\n", __func__);
        return -1;
    }


    /*** Prepare pList and pSLot to add to Linked List ***/
    /* allocate pList */
    pList = (SlotList *) malloc (sizeof (SlotList));

    /* verify allocation */
    if (pList == NULL){
        DebugConsole2("%s: Failed to allocate memory for node in linked list. Halting...\n",
                      __func__ );
        halt(1);    //memory allocation failed
    }

    /* Point pList to pSlot */
    pList->slot = pSlot;


    /*** Add pList to Mailbox Queue and Update Links ***/
    pList->pNextSlot = NULL;                //Add pList to slotQueue

    if(ListIsEmpty(&pMbox->slotQueue, NULL)){    //if index is empty...
        pMbox->slotQueue.pHeadSlot = pList;                  //add to front of list
    }
    else{
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
    Parameters -    pSlotQ:  Pointer to Queue to remove Slot from
    Returns -       None
    Side Effects -  None
   ----------------------------------------------------------------------- */
int RemoveSlotLL(slotQueue * pSlot, mailbox * pMbox){

    /*** Function Initialization ***/
    SlotList * pSave;           //New node to save position
    pSave = pSlot->pHeadSlot;   //Save slot pointer to list head


    /*** Error Check: Verify Messages in List ***/
    if(pSave == NULL){  //Check that pSave returned process
        DebugConsole2("%s: Attempting to Remove Slot from Empty List\n", __func__);
        return -1;
    }


    /*** Remove Slot and Update Linkers ***/
    pSlot->pHeadSlot = pSave->pNextSlot;    //increment head to next slot
    pSave->pNextSlot = NULL;                //set old head->next to NULL

    if(pSlot->pHeadSlot != NULL){           //if NOT only node in Queue
        pSlot->pHeadSlot->pPrevSlot = NULL; //set new head->prev to NULL
    }
    else{                                   //if only node in Queue
        pSave->pPrevSlot = NULL;            //clear linkers
        pSlot->pTailSlot = NULL;
    }


    /*** Decrement Counters ***/
    pMbox->activeSlots--;
    pMbox->slotQueue.total--;

    return 0;
}/* RemoveSlotLL */

/* ------------------------------------------------------------------------
    Name -          AddProcessLL
    Purpose -       Add Process to Specified Process Queue
    Parameters -    pid:    Process ID
                    pq:     Pointer to Process Queue
    Returns -       None, halt (1) on error
    Side Effects -  None
   ----------------------------------------------------------------------- */
void AddProcessLL(int pid, procQueue * pq){

    /*** Function Initialization ***/
    mailbox * pMbox = GetMboxIndex(pq->mbox_id);
    ProcList * pNew;    //New Node


    /*** Error Check: List is Full ***/
    if(ListIsFull(NULL, NULL, pq)){
        DebugConsole2("%s: Queue is full.\n", __func__);
        halt(1);
    }


    /*** Allocate Space for New Node ***/
    pNew = (ProcList *) malloc (sizeof (ProcList));

    /* Verify Allocation Success */
    if(pNew == NULL){
        DebugConsole2("%s: Failed to allocate memory for node in linked list. Halting...\n",
                      __func__);
        halt(1);    //memory allocation failed
    }


    /*** Add New Node to List ***/
    pNew->pid = pid;                            //update pid value
    pNew->pNextProc = NULL;                     //assign pNext to NULL

    if(ListIsEmpty(NULL, pq)){      //if list is empty...
        pq->pHeadProc = pNew;                   //add pNew to front of list
    }
    else{                                       //if list not empty...
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
    Returns -       Pid of removed Process, halt (1) on error
    Side Effects -  None
   ----------------------------------------------------------------------- */
int RemoveProcessLL(int pid, procQueue * pq){

    /*** Function Initialization ***/
    ProcList * pSave;   //Pointer for Linked List
    int pidFlag = 0;    //verify pid found in list
    int remFlag = 0;    //verify process removed successfully


    /*** Error Check: List is Empty ***/
    if(ListIsEmpty(NULL, pq)){
        DebugConsole2("%s: Queue is empty.\n", __func__);
        halt(1);
    }


    /*** Find PID and Remove from List ***/
    pSave = pq->pHeadProc;          //Set pSave to head of list

    while(pSave->pid != 0) {        //verify not end of list    //todo: does 0 work? x>0?
        if(pSave->pid == pid){      //if found pid to remove...
            pidFlag = 1;            //Trigger Flag - PID found

            /** If pid to be removed is at head **/
            if(pSave == pq->pHeadProc){
                pq->pHeadProc = pSave->pNextProc;           //Set next pid as head
                pSave->pNextProc = NULL;                    //Set old head next pointer to NULL

                /* if proc is NOT only node on list */
                if(pq->pHeadProc != NULL) {
                    pq->pHeadProc->pPrevProc = NULL;        //Set new head prev pointer to NULL
                }

                    /* if proc IS only node on list */
                else{
                    pSave->pPrevProc = NULL;    //Set old prev pointer to NULL
                    pq->pTailProc = NULL;       //Set new tail to NULL

                }

                remFlag = 1;                    //Trigger Flag - Process Removed
                break;
            }

            /** If pid to be removed is at tail **/
            else if(pSave == pq->pTailProc){
                pq->pTailProc = pSave->pPrevProc;       //Set prev pid as tail
                pSave->pPrevProc = NULL;                //Set old tail prev pointer to NULL
                pq->pTailProc->pNextProc = NULL;        //Set new tail next pointer to NULL
                remFlag = 1;                            //Trigger Flag - Process Removed
                break;
            }

            /** If pid to be removed is in middle **/
            else{
                pSave->pPrevProc->pNextProc = pSave->pNextProc; //Set pSave prev next to pSave next
                pSave->pNextProc->pPrevProc = pSave->pPrevProc; //Set pSave next prev to pSave prev
                pSave->pNextProc = NULL;    //Set old pNext to NULL
                pSave->pPrevProc = NULL;    //Set old pPrev to NULL
                remFlag = 1;                //Trigger Flag - Process Removed
                break;
            }
        }
        else{                           //if pid not found
            pSave = pSave->pNextProc;   //increment to next in list
        }
    }//end while

    /*** Decrement Counter ***/
    pq->total--;

    /*** Error Check: Pid Found and Removed ***/
    if(pidFlag && remFlag){
        pSave = NULL;       //free() causing issues in USLOSS

    }
    else {
        if (pidFlag == 0){
            DebugConsole2("%s: Unable to locate pid [%d]. Halting...\n",
                          __func__, pid);
            halt(1);
        }
        else if (remFlag == 0) {
            DebugConsole2("%s: Removal of pid [%d] unsuccessful. Halting...\n",
                          __func__, remFlag);
            halt(1);
        }
    }

    /*** Function Termination ***/
    return pid;     //process removal success

}/* RemoveProcessLL */

/* * * * * * * * * * * * END OF Linked List Functions * * * * * * * * * * * */



/* * * * * * * * * * * Mailbox Slots Release Functions * * * * * * * * * * */
/* ------------------------------------------------------------------------
   Name -           HelperRelease
   Purpose -        A helper function for mboxRelease
   Parameters -     pMbox:    Pointer to Mailbox
   Returns -        none
   Side Effects -   none
   ----------------------------------------------------------------------- */
void HelperRelease(mailbox *pMbox) {

    DebugConsole2("%s: Function successfully entered.\n",
                  __func__);

    /*** Function Initialization ***/
    pMbox->totalProcs = pMbox->waitingToSend.total + pMbox->waitingToReceive.total;


    /*** Unblock Receiving Processes ***/
    while (pMbox->waitingToReceive.total > 0) {
        DebugConsole2("%s: Unblocking RECEIVE_BLOCK",
                      __func__);

        /* Remove from List */
        int pid = RemoveProcessLL(pMbox->waitingToReceive.pHeadProc->pid,
                                  &pMbox->waitingToReceive);

        /* Unblock Process */
        unblock_proc(pid);
        disableInterrupts();
    }


    /*** Unblock Sending Processes ***/
    while (pMbox->waitingToSend.total > 0) {
        DebugConsole2("%s: Unblocking SEND_BLOCK",
                      __func__);
        /* Remove from List */
        int pid = RemoveProcessLL(pMbox->waitingToSend.pHeadProc->pid,
                                  &pMbox->waitingToSend);
        /* Unblock Process */
        unblock_proc(pid);
        disableInterrupts();;
    }


    /*** Verify Unblocked Processes Completed ***/
    if (pMbox->totalProcs > 0) {
        releaseWasBlocked = 1;
        block_me(RELEASE_BLOCK);
    }

    return;
}

/* ------------------------------------------------------------------------
   Name -           MboxWasReleased
   Purpose -        Checks if mailbox was released
   Parameters -     pMbox:    Pointer to Mailbox
   Returns -        0:  Mailbox was not released
                    1:  Mailbox was released
   Side Effects -   none
   ----------------------------------------------------------------------- */
bool MboxWasReleased(mailbox *pMbox){
    DebugConsole2("%s: Function successfully entered.\n",
                  __func__);

    /*** Check if Mailbox was Released ***/
    if(pMbox->status == MBOX_RELEASED){
        DebugConsole2("%s: Mailbox was released. "
                      "%d Process(es) need to be released\n",
                      __func__, pMbox->totalProcs);

        /** Check Remaining Processes **/
        /* Current is Last Process */
        if (pMbox->totalProcs == 1) {
            DebugConsole2("%s: Unblocking Releaser\n",
                          __func__, pMbox->totalProcs);

            /* Decrement totalProcs */
            pMbox->totalProcs--;

            /* Unblock Releaser */
            if (releaseWasBlocked == 1) {

                unblock_proc(pMbox->releaser);
                disableInterrupts();
            }
        }
        /* Multiple Processes Remaining */
        else{
            pMbox->totalProcs--;
        }

        /*** Function Termination ***/
        DebugConsole2("%s: mailbox was released while process was blocked.\n "
                      "%d Process(es) need to be released\n",
                      __func__, pMbox->totalProcs);
        enableInterrupts();
        return true;    //Mailbox was released
    }
    return false;       //Mailbox was not released
}

/* * * * * * * * * * END OF Mailbox Slots Release Functions * * * * * * * * * */