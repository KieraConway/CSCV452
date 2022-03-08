/* ------------------------------------------------------------------------
    phase2.c
    Applied Technology
    College of Applied Science and Technology
    The University of Arizona
    CSCV 452

    Kiera Conway
    Katelyn Griffith

    NOTES:
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

/* Helper Function for Send and Receive */
void HelperSend(int mbox_slot, void *msg, int msg_size, int block_flag);
void HelperReceive(int mbox_slot, void *msg, int msg_size, int block_flag);

/* Functions Brought in From Phase 1 */
static void enableInterrupts();
static void disableInterrupts();
static void check_kernel_mode(const char *functionName);
void DebugConsole2(char *format, ...);
void Time_Slice(void);

/* Handler Initializers */
void ClockIntHandler2(int dev, void *arg);
void DiskIntHandler(int dev, void *arg);
void TermIntHandler(int dev, void *arg);
void SyscallIntHandler(int dev, void *arg);

/*Mailbox Table*/
int GetNextMailID();
void InitializeMailbox(int mbox_slot, int mbox_id, int maxSlots, int slotSize);
int GetMboxIndex(int mbox_slot);

/* Linked List Functions */
void InitializeList(slotQueue *pSlot, procQueue *pProc);    //Initialize Linked List
int AddSlotLL(void *msg_ptr, slotQueue * pSlot);        //Add Slot to Specific Mailbox
void RemoveSlotLL(int slotId, slotQueue * pSlot);       //Remove Slot from Mailbox
bool SlotListIsFull(const slotQueue * pSlot);           //Check if Slot List is Full
bool SlotListIsEmpty(const slotQueue * pSlot);          //Check if Slot List is Empty


/* -------------------------- Globals ------------------------------------- */

int debugFlag2 = 0;                 //used for debugging
mailbox MailboxTable[MAXMBOX];      //mailbox table
int totalMailbox;                   //total mailboxes
int totalSlots = 0;                 //total slot count
int totalActiveSlots = 0;           //total active slot count
unsigned int next_mid = 0;          //next mailbox id [mbox_id]
int clock_mbox;                     //clock interrupt handler
int disk_mbox[2];                   //disk interrupt handler
int term_mbox[4];                   //terminal interrupt handler

/* -------------------------- Functions -----------------------------------

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

    DebugConsole2("start1(): at beginning\n");

    /* Check for kernel mode */
    check_kernel_mode("start1");

    /* Disable interrupts */
    disableInterrupts();

    /*Initialize the mailbox table*/
    memset(MailboxTable, 0, sizeof(MailboxTable)); // added in kg

    /*Initialize individual interrupts in the interrupt vector*/
    //values from usloss.h, handler.c
    int_vec[CLOCK_INT] = &clock_handler;      // 0 - clock
    int_vec[DISK_INT] = &disk_handler;        // 2 - disk
    int_vec[TERM_INT] = &term_handler;        // 3 - terminal
    int_vec[SYSCALL_INT] = &syscall_handler;  // 5 - syscall

    /*Initialize the system call vector*/
//    for(int i = 0; i < MAXSYSCALLS; i++){
//        sys_vec[i] = nullsys;
//    }

    /*allocate mailboxes for interrupt handlers */
    clock_mbox = MboxCreate(0, sizeof(int));    //Clock
    disk_mbox[0] = MboxCreate(0, sizeof(int));  //Disk
    disk_mbox[1] = MboxCreate(0, sizeof(int));
    term_mbox[0] = MboxCreate(0, sizeof(int));  //Terminal
    term_mbox[1] = MboxCreate(0, sizeof(int));
    term_mbox[2] = MboxCreate(0, sizeof(int));
    term_mbox[3] = MboxCreate(0, sizeof(int));

    /* Enable interrupts */
    enableInterrupts();

    DebugConsole2("start1(): fork'ing start2 process\n");

    /*Create Process for start2*/
    kid_pid = fork1("start2", start2, NULL, 4 * USLOSS_MIN_STACK, 1);

    /*Join until Start2 quits*/
    if ( join(&status) != kid_pid ) {
        console("start2(): join returned something other than start2's pid\n");
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

    int newMid;     //new mailbox id
    int mbox_slot;  //new mailbox index in Mailbox Table

    // Check for kernel mode
    check_kernel_mode(__func__);

    // Disable interrupts
    disableInterrupts();

    /*Error Check: No Mailboxes Available*/
    if(totalMailbox >= MAXMBOX){
        DebugConsole2("%s : No Mailboxes Available.\n", __func__);
        return -1;
    }

    /*Error Check: Incorrect Slot Size*/
    if (slot_size > MAXMESSAGE)
    {
        DebugConsole2("%s : The message size[%d] exceeds limit [%d].\n",
                      __func__, slots, MAXMESSAGE);
        return -1;
    }

    /*Error Check: Check for other invalid parameter values*/
    if (slots < 0 || slot_size < 0)
    {
        DebugConsole2("%s : An invalid value of slot size [%d] or message size [%d] was used.\n",
                      __func__, slots, slot_size);
        return -1;
    }

//    // Check if zero-slot mailbox
//    if(slots == 0){
//        /*TODO
//         * Zero-slot mailboxes are a special type of mailbox and are intended for
//         * synchronization between a sender and a receiver. These mailboxes have
//         * no slots and are used to pass messages directly between processes;
//         * they act much like a signal.
//         */
//    }

    /*Get next empty mailbox from mailboxTable*/
    if((newMid = GetNextMailID()) == -1){           //get next mbox ID
        //Error Check: Exceeding Max Processes
        DebugConsole2("Error: Attempting to exceed MAXMBOX limit [%d].\n", MAXMBOX);
        return -1;
    }
    mbox_slot = newMid % MAXMBOX;                   //assign mbox_slot

    /*Initialize Mailbox*/
    InitializeMailbox(mbox_slot, newMid, slots, slot_size);

    /*Increment totalSlots*/
    totalSlots += slots;    //note: totalSlots is different than activeSlots

    /*Increment Mailbox*/
    totalMailbox += 1;

    // Enable interrupts
    enableInterrupts();

    /*Return Unique mailbox ID*/
    return MailboxTable[mbox_slot].mbox_id;

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
    //////////////////////////
    //TODO
    //
    // NOTES: add to list function
    //messages are delivered in the order sent and are
    //////////////////////////
    int mbox_slot = GetNextMailID(mbox_id);

    // Check for kernel mode
    check_kernel_mode(__func__);

    /*** Error Check: illegal values given as arguments ***/
    //check mailbox id
    if(mbox_id < 0){
        DebugConsole2("%s : Illegal mailbox ID [%d].\n", __func__, mbox_id);
        return -1;
    }
    //check msg_ptr
    if(msg_ptr == NULL){
        DebugConsole2("%s : Invalid message pointer.\n", __func__);
        return -1;
    }
    //check msg_size
    if(msg_size > MAXMESSAGE || msg_size < 0){
        DebugConsole2("%s : Invalid message size [%d].\n", __func__, msg_size);
        return -1;
    }


    //Add Message and Slot into Mailbox
    if((AddSlotLL(msg_ptr, &MailboxTable[mbox_slot].slotQueue)) == -1){
        /*** Block the sending process if no slot available ***/
        int curPid = getpid();      //Get Current Running Process ID
        block_me(4);

        //add to waitlist

    }


    // will block when number of slots is full until one becomes available

    //use memcpy to copy binary message data

    /*** Error Check: Mailbox was Released ***/   //todo release vs empty ?

    if(MailboxTable[mbox_slot].status == MBOX_EMPTY){
        DebugConsole2("%s: mailbox was released while process was blocked.", __func__);
        return -3;
    }

    /*** Error Check: Process was Zapped ***/
    if(is_zapped()){
        DebugConsole2("%s: Process was zapped while it was blocked", __func__);
        return -3;
    }

    return 0;
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
    //NOTE:
    // messages are received in the order the receive function is called.

    /* TODO: remove upon completion
     * Return values:
     * -3:  process is zapped, or the mailbox was released while the process was
     * blocked on the mailbox.
     * -1:  illegal values given as arguments; or, message sent is too large
     * for receiver's buffer (no data copied in this case).
     * >= 0:  the size of the message received.
     * */

    //Get a msg from a slot of the indicated mailbox.
    //Block the receiving process if no msg available.


    // will block calling process until a message is Received

    //The receiver is responsible for providing storage to hold the message

    // Check for kernel mode
    check_kernel_mode(__func__);
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
    /* TODO: remove upon completion
     * Return values:
     * -3:  process was zap'd while releasing the mailbox.
     * -1:  the mailboxID is not a mailbox that is in use.
     * 0:   Success
     * */

    // Disable interrupts
    disableInterrupts();

    // Check for kernel mode
    check_kernel_mode(__func__);

    // if invalid ID
    if (mbox_id > MAXMBOX || mbox_id < 0)
    {
        DebugConsole2("The mailbox ID used was invalid.\n");
        return -1;
    }

    // if mailbox slot was not in use
    // return -1;

    // clear out mailbox slot

    // Enable interrupts
    enableInterrupts();

    //iszapped();

} /* MboxRelease */


/* ------------------------------------------------------------------------
   Name - MboxCondSend
   Purpose -    Send a message to a mailbox.
   Parameters -
   Returns -
            -3:  process is zap'd.
            -2:  mailbox full, message not sent;
                 or no mailbox slots available in the system.
            -1:  illegal values given as arguments.
             0:  message sent successfully.
   Side Effects -
   ----------------------------------------------------------------------- */
int MboxCondSend(int mailboxID, void *message, int message_size)
{
    /* TODO: remove upon completion
     * Return values:
     *   -3:  process is zapped
     *   -2:  mailbox full, message not sent; or no mailbox slots available in the system
     *   -1:  illegal values given as arguments.
     *    0:  message sent successfully.
    */

    // Check for kernel mode
    check_kernel_mode(__func__);

} /* MboxCondSend */

/* ------------------------------------------------------------------------
   Name - MboxCondReceive
   Purpose -
   Parameters -
   Returns -
   Side Effects -
   ----------------------------------------------------------------------- */
int MboxCondReceive(int mailboxID, void *message, int max_msg_size)
{
    /* Return values:
    -3:  process is zapped.
    -2:  mailbox empty, no message to receive.
    -1:  illegal values given as arguments; or, message sent is too large for
    receiver's buffer (no data copied in this case).
    >= 0:  the size of the message received.
    */

    // Check for kernel mode
    check_kernel_mode(__func__);

} /* MboxCondReceive */

/* ------------------------------------------------------------------------
   Name - check_io
   Purpose -
   Parameters -
   Returns - zero.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int check_io()
{
    return 0;
} /* check_io */

/* ------------------------------------------------------------------------
   Name - waitdevice
   Purpose -
   Parameters -
   Returns -
   Side Effects -
   ----------------------------------------------------------------------- */
int waitdevice(int type, int unit, int *status)
{
    /* Return values:
    -1:  the process was zapped while waiting
    0:  otherwise
    */

    // Check for kernel mode
    check_kernel_mode(__func__);

}  /* waitdevice */

/* ------------------------------------------------------------------------
   Name - HelperSend
   Purpose -
   Parameters -
   Returns -
   Side Effects -
   ----------------------------------------------------------------------- */
void HelperSend(int mbox_slot, void *msg, int msg_size, int block_flag)
{
    // fill me
    return;
}


/* ------------------------------------------------------------------------
   Name - HelperReceive
   Purpose -
   Parameters -
   Returns -
   Side Effects -
   ----------------------------------------------------------------------- */
void HelperReceive(int mbox_slot, void *msg, int msg_size, int block_flag)
{
    // fill me
    return;
}


/* ------------------------------------------------------------------------
Name -          enableInterrupts
Purpose -
Parameters -    None
Returns -
Side Effects -
----------------------------------------------------------------------- */
static void enableInterrupts()
{
    check_kernel_mode(__func__);
    /*Confirmed Kernel Mode*/

    int curPsr = psr_get();

    curPsr = curPsr | PSR_CURRENT_INT;

    psr_set(curPsr);
} /* enableInterrupts */


/* ------------------------------------------------------------------------
Name -          disableInterrupts
Purpose -
Parameters -    None
Returns -
Side Effects -
----------------------------------------------------------------------- */
static void disableInterrupts()
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
Name -          check_kernel_mode
Purpose -       Checks the PSR for kernel mode and halts if in user mode
Parameters -    functionName
Returns -       nothing
Side Effects -  Will halt if not kernel mode
----------------------------------------------------------------------- */
static void check_kernel_mode(const char *functionName)
{
//    union psr_values psrValue; /* holds caller's psr values */
//
//    DebugConsole2("check_kernel_node(): verifying kernel mode for %s\n", functionName);
//
//    /* test if in kernel mode; halt if in user mode */
//    psrValue.integer_part = psr_get();  //returns int value of psr
//
//    if (psrValue.bits.cur_mode == 0)
//    {
//        if (Current) {  //if currently running function
//            int cur_proc_slot = Current->pid % MAXMBOX;
//            console("%s(): called while in user mode, by process %d. Halting...\n", functionName, cur_proc_slot);
//        }
//        else {  //if no currently running function, startup is running
//            console("%s(): called while in user mode, by startup(). Halting...\n", functionName);
//        }
//        halt(1);
//    }

    DebugConsole2("Function is in Kernel mode (:\n");
} /* check_kernel_mode */


/* ------------------------------------------------------------------------
Name -          DebugConsole2
Purpose -       Prints the message to the console if in debug mode
Parameters -    format string and va args
Returns -       nothing
Side Effects -
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

/* ------------------------------------------------------------------------
    Name -          CopyToList
    Purpose -       Copies process at MailBoxTable[mbox_slot] to pStat
    Parameters -    mbox_slot: location of mailbox in MailBoxTable
                    pSlot: Address of mailbox slot
    Returns -       none
    Side Effects -  none
   ----------------------------------------------------------------------- */
static void CopyToList(int mbox_slot, SlotList * pSlot)
{
    //pStat->process = &ProcTable[proc_slot];
    return;
}

/* ------------------------------------------------------------------------
 * TODO
 *
    Name -          time_slice
    Purpose -       calls the dispatcher if the currently executing process
                        has exceeded its time slice
    Parameters -    none
    Returns -       none
    Side Effects -  none
   ----------------------------------------------------------------------- */
void Time_Slice(void){
//    int proc_slot;
//    int curPid;
//    int currentTime = sys_clock();
//
//    Current->totalCpuTime += ReadTime(); //calculate total cpu time
//    curPid = getpid();                    //set current pid
//    proc_slot = (curPid) % MAXMBOX;     //find current proc_slot
//
//
//    if (Current->status == STATUS_RUNNING) {  //if current status is running (not blocked)
//        if ((currentTime - ((Current->switchTime) / 1000)) > TIME_SLICE) {  //if exceeded
//
//            AddProcessLL(proc_slot, &ReadyList);
//            Current->status = STATUS_READY; //switch Current status to ready
//            dispatcher();
//        }
//    }
//
//    return;
}


/* ------------------------------------------------------------------------
 * TODO
 *
    Name -          InList
    Purpose -       iterates through list pStat to find process using pid
    Parameters -    id:     Unique Parameter ID [mbox_id, slot_id, pid]
                    list:   List to Search Through [slot, mbox, proc]
    Returns -       0:  not found
                    1:  found
    Side Effects -  none
   ----------------------------------------------------------------------- */
bool InList(int id, char list) {
//    int proc_slot = pid % MAXMBOX;                  //find zapped_proc_slot
//    proc_ptr Process = &ProcTable[proc_slot];       //pointer to zapped process
//    StatusStruct * pSave;                           //used to iterate through ReadyList
//
//    int index = (Process->priority)-1;   //index in list
//    pSave = pStat[index].pHeadProc;     //Find correct index, Start at head
//
//    if(pSave == NULL){  //if pSave is NULL, process not found
//        return 0;
//    }
//
//    //iterate through list
//    while(pSave->process != NULL){          //verify not end of list
//
//        if(pSave->process->pid == pid){     //if PIDs match,
//            return 1;                       //process found
//        }
//        else{
//            pSave = pSave->pNextProc;       //iterate to next on list
//        }
//    }
//    return 0;   //not found
}
/* ------------------------------------------------------------------------
   Name -           GetNextMailID
   Purpose -        Finds next mbox_id
   Parameters -     none
   Returns -        >0: next mbox_id
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetNextMailID() {
    int newMid = -1;
    int mboxSlot = next_mid % MAXMBOX;

    if (totalMailbox < MAXMBOX) {
        while ((totalMailbox < MAXMBOX) && (MailboxTable[mboxSlot].status != MBOX_EMPTY)) {
            next_mid++;
            mboxSlot = next_mid % MAXMBOX;
        }

        //TODO make sure it doesn't have to be next_mid++
        newMid = next_mid;    //assign newPid *then* increment next_pid
    }

    return newMid;
}

/* ------------------------------------------------------------------------
   Name -           InitializeMailbox
   Purpose -        Initializes the new mailbox
   Parameters -     mbox_slot:  Location inside MailBoxTable
                    mbox_id:    Unique Mailbox ID
                    maxSlots:   Maximum number of slots
                    slotSize:   Maximum size of slot/ message
   Returns -        none
   Side Effects -   Mailbox added to MailboxTable
   ----------------------------------------------------------------------- */
void InitializeMailbox(int mbox_slot, int mbox_id, int maxSlots, int slotSize) {

    MailboxTable[mbox_slot].mbox_slot = mbox_slot;                 //Mailbox Slot
    MailboxTable[mbox_slot].mbox_id = mbox_id;                     //Mailbox ID
    MailboxTable[mbox_slot].status = MBOX_USED;                    //Mailbox Status
    InitializeList(NULL, &MailboxTable[mbox_slot].waitingProcs);    //Waiting Process List
    InitializeList(NULL, &MailboxTable[mbox_slot].waitingToSend);   //Waiting to Send List
    InitializeList(NULL, &MailboxTable[mbox_slot].waitingToReceive); //Waiting to Receive List
    MailboxTable[mbox_slot].maxSlots = maxSlots;                   //Max Slots
    MailboxTable[mbox_slot].activeSlots = 0;                       //Active Slots
    MailboxTable[mbox_slot].slotSize = slotSize;                   //Slot Size
    InitializeList(&MailboxTable[mbox_slot].slotQueue, NULL);       //Slot List

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
/* * * * * SLOTS Linked List Functions * * * * */
/* */
/* */

/* ------------------------------------------------------------------------
    Name -          InitializeList
    Purpose -       Initialize List pointed to by parameter
    Parameters -    Meant to be used as either or, use NULL for other
                    *pSlot: Pointer to Message Slot List
                    *pProc: Pointer to Process List
    Returns -       None, halt(1) on error
    Side Effects -  None
   ----------------------------------------------------------------------- */
void InitializeList(slotQueue *pSlot, procQueue *pProc)
{
    if(pProc == NULL) {
        // Initialize the head, tail, and count of the passed in list
        pSlot->pHeadSlot = NULL;
        pSlot->pTailSlot = NULL;
        pSlot->total = 0;
        return;
    }
    else if(pSlot == NULL){
        pProc->pHeadProc = NULL;
        pProc->pTailProc = NULL;
        pProc->total = 0;
    }
    else{
        DebugConsole2("%s: Invalid Parameters Passed", __func__);
        halt(1);
    }
}


/* ------------------------------------------------------------------------
    Name -          AddSlotLL
    Purpose -       Adds Slot with Message to slotQueue
    Parameters -    slotIndex:  Slot Index in Mailbox
                    pSlot:      Pointer to MailboxList to add Slot to
    Returns -        0: Success
                    -1: Slot Queue is Full
    Side Effects -  None
   ----------------------------------------------------------------------- */
int AddSlotLL(void *msg_ptr, slotQueue * pSlot){

    SlotList * pNew;   //new slot node
    int index = GetMboxIndex(pSlot->mbox_id);   //get mailbox index

    /*** Error Check: Verify Slot Space Available***/
    // check if slot queue is full
   if(SlotListIsFull(pSlot)){
      DebugConsole2("%s: Slot Queue is full.\n", __func__);
      return -1;
   }
   // check if exceeding active slots
   if (totalActiveSlots >= MAXSLOTS){
       DebugConsole2("%s: No Active Slots Available. Halting...\n", __func__);
       halt(1);
   }

    /*** Prepare pNew to add to Linked List ***/
    //allocate pNew
    pNew = (SlotList *) malloc (sizeof (SlotList));

   //verify allocation
    if (pNew == NULL){
        DebugConsole2("%s: Failed to allocate memory for node in linked list. Halting...\n",
                      __func__ );
        halt(1);    //memory allocation failed
    }

    //copy message into pNew
    memcpy(pNew->slot->message, msg_ptr, sizeof(msg_ptr+1));


    /*** Add pNew with Message to slotQueue ***/
    pNew->pNextSlot = NULL;

    if(SlotListIsEmpty(pSlot)){                 //if index is empty...
        pSlot->pHeadSlot = pNew;                //add to front of list
    }
    else{
        pSlot->pTailSlot->pNextSlot = pNew;     //add to end of list
        pNew->pPrevSlot = pSlot->pTailSlot;     //assign pPrev to current Tail
    }

    pSlot->pTailSlot = pNew;        //reassign pTail to new Tail


    /*** Update Global Counters and Status ***/
    totalSlots++;
    totalActiveSlots++;


    return 0;
}

/* ------------------------------------------------------------------------
    Name -          RemoveSlotLL
    Purpose -       Removes Slot from Mailbox
    Parameters -    slotId: Slot ID
                    pSlot:  Pointer to List to remove Slot from
    Returns -       None
    Side Effects -  None
   ----------------------------------------------------------------------- */
void RemoveSlotLL(int slotId, slotQueue * pSlot){
//    int index = priority - 1;
//    StatusStruct * pSave;   //pointer for LL
//    int pidFlag = 0;        //verify pid found in list
//    int remFlag = 0;        //verify process removed successfully
//
//    pSave = pSlot[index].pHeadProc;    //Find correct index, Start at head
//
//    if(pSave == NULL){  //Check that pSave returned process
//       console("Error: Process %d is not on list. Halting...\n", pid );
//       halt(1);
//    }
//
//    //iterate through list
//    while(pSave->process != NULL){    //verify not end of list
//
//       if(pSave->process->pid == pid){ //if found process to remove
//          pidFlag = 1;                   //Trigger Flag - PID found
//
//          /* If process to be removed is at head */
//          if(pSave == pStat[index].pHeadProc){
//                pStat[index].pHeadProc = pSave->pNextProc; //Set next process as head
//                pSave->pNextProc = NULL;                   //Set old head next pointer to NULL
//                if(pStat[index].pHeadProc != NULL) {           //if NOT only one node on list
//                   pStat[index].pHeadProc->pPrevProc = NULL;  //Set new head prev pointer to NULL
//                }
//                else{           //if only node on list
//                   pSave->pPrevProc = NULL;
//                   pStat[index].pTailProc = NULL;
//
//                }
//                remFlag = 1;                               //Trigger Flag - Process Removed
//                break;
//          }
//
//                /* If process to be removed is at tail */
//          else if(pSave == pStat[index].pTailProc){
//                pStat[index].pTailProc = pSave->pPrevProc;  //Set prev process as tail
//                pSave->pPrevProc = NULL;                   //Set old tail prev pointer to NULL
//                pStat[index].pTailProc->pNextProc = NULL; //Set new tail next pointer to NULL
//                remFlag = 1;
//                break;
//          }
//
//                /* If process to be removed is in middle */
//          else{
//                pSave->pPrevProc->pNextProc = pSave->pNextProc; //Set pSave prev next to pSave next
//                pSave->pNextProc->pPrevProc = pSave->pPrevProc; //Set pSave next prev to pSave prev
//                pSave->pNextProc = NULL;    //Set old pNext to NULL
//                pSave->pPrevProc = NULL;    //Set old pPrev to NULL
//                remFlag = 1;
//                break;
//          }
//
//       }
//       else{
//          pSave = pSave->pNextProc;
//       }
//
//    }
//
//    if(pidFlag && remFlag){
//       pSave = NULL;
//
//       /*** Decrement Counter ***/
//       if (pStat == ReadyList){    //compare memory locations to verify list type
//          totalReadyProc--;       //decrement total ready processes
//       }
//       else if(pStat == BlockedList){ //compare memory locations to verify list type
//          totalBlockedProc--;        //decrement total blocked processes
//       }
//       else if(pStat == Current->activeChildren.procList){ //compare memory locations to verify list type
//          Current->activeChildren.total--;                //decrement total activeChildren processes
//       }
//       else if(pStat == Current->quitChildren.procList){   //compare memory locations to verify list type
//          Current->quitChildren.total--;                  //decrement total quitChildren processes
//       }
//       else if(pStat == Current->zappers.procList){        //compare memory locations to verify list type
//          Current->zappers.total--;                       //decrement total zappers processes
//       }
//       else if(pStat == Current->parent_proc_ptr->quitChildren.procList){
//          Current->parent_proc_ptr->quitChildren.total--;
//       }
//       else if(pStat == Current->parent_proc_ptr->activeChildren.procList){
//          Current->parent_proc_ptr->activeChildren.total--;
//       }
//       else if(pStat == Current->parent_proc_ptr->zappers.procList){
//          Current->parent_proc_ptr->zappers.total--;
//       }
//
//       return 0;
//    }
//    else {
//       if (pidFlag == 0){
//          console("Error: Unable to locate process pid [%d]. Halting...\n", pid);
//          halt(1);
//       }
//       else if (remFlag == 0) {
//          console("Error: Removal of process [%d] unsuccessful. Halting...\n", remFlag);
//          halt(1);
//       }
//    }
//
    return;
}


/* ------------------------------------------------------------------------
    Name -          SlotListIsFull
    Purpose -       Checks if Slot List is Full
    Parameters -    pSlot: Mailbox Slot List Structure
    Returns -        0: not full
                     1: full
                    -1: list not found
    Side Effects -  none
   ----------------------------------------------------------------------- */
bool SlotListIsFull(const slotQueue * pSlot){

    /*** Find Mailbox Index ***/
    int index = GetMboxIndex(pSlot->mbox_id);

    /*** Check if Mailbox is Full ***/
    if (MailboxTable[index].maxSlots >= MAXSLOTS)
        return true;    //is full
    else
        return false;   //is not full
}


/* ------------------------------------------------------------------------
    Name -          SlotListIsEmpty
    Purpose -       Checks if Slot List is Empty
    Parameters -    pSlot: Mailbox Slot List Structure
    Returns -        0: not Empty
                     1: Empty
                    -1: list not found
    Side Effects -  none
   ----------------------------------------------------------------------- */
bool SlotListIsEmpty(const slotQueue * pSlot){
    return pSlot->pHeadSlot == NULL;    //checks if pHead is empty
}


/* */
/* */
/* * * * * END of Slots Linked List Functions * * * * */