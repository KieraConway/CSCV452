#ifndef MESSAGE_H  //Header Guards
#define MESSAGE_H

#include <stdbool.h>
#include <string.h>
#include "handler.h"

/* ------------------------- Constants ----------------------------------- */
#define DEBUG2 1
// Added in for slot/process usage status
#define MBOX_EMPTY      0   //Mailbox Table Index is Empty
#define MBOX_USED       1   //Mailbox Table Index is Not Empty/ Used
#define SLOT_EMPTY      2   // Slot is empty and ready to hold a message
#define SLOT_FULL       3   // Slot is full and holds a message currently
#define WAITING         4   // Process is waiting to send or receive a message
#define SENDING         5   // Process is sending a message
#define RECEIVING       6   // Process is receiving a message
#define SLOT_BLOCKED    7   // Process is blocked


/* ------------------------ Typedefs and Structs ------------------------ */
typedef struct procQueue procQueue;
typedef struct slotQueue slotQueue;
typedef struct slot_Table * slot_ptr;
typedef struct msg_table * msg_ptr;
typedef struct mailbox mailbox;
typedef struct mbox_proc *mbox_proc_ptr;

/* Linked List for PID */
typedef struct  procList{
    int pid;                        //process PID
    struct ProcList *pNextProc;     //points to next process
    struct ProcList *pPrevProc;     //points to previous process
} ProcList;

struct procQueue {
    ProcList *pHeadProc;            //points to msg list head
    ProcList *pTailProc;            //points to msg list tail
    int total;                      //counts total messages
};

/* Structures for Mailbox Slot Lists */
typedef struct  slotList{
    slot_ptr slot;
    struct  SlotList *pNextSlot;    //points to next process
    struct  SlotList *pPrevSlot;    //points to previous process
} SlotList;

struct slotQueue {
    SlotList *pHeadSlot;            //points to msg list head
    SlotList *pTailSlot;            //points to msg list tail
    int total;                      //counts total messages
    int mbox_id;                    //mailbox id (for index reference)
};

typedef struct slot_Table {
    //short           slot_id;              //unique slot id //TODO: REMOVE
    //short           slot_index;           //index (slot_id % MAXSLOTS) TODO: REMOVE
    unsigned int    message;                // Binary representation of Message
    int             status;                 //message status
    int             mbox_id;                //mailbox id (for index reference)
} Slot_Table;
//end of Slot structures

struct mailbox {
    int         mbox_slot;          // Location inside MailboxTable
    int         mbox_id;            // Unique Mailbox ID
    int         status;
    procQueue   waitingProcs;       // Linked list of waiting processes to be used to send and receive
    procQueue   waitingToSend;      // Linked list of processes waiting to send a message
    procQueue   waitingToReceive;   // Linked list of processes to receive a message
    int         maxSlots;           // Maximum number of slots for mailbox
    int         activeSlots;        // Number of active slots
    int         slotSize;           // Maximum size of slot/ message
    slotQueue   slotQueue;          // Corresponding slot within the mailbox
};

struct psr_bits {
    unsigned int cur_mode:1;
    unsigned int cur_int_enable:1;
    unsigned int prev_mode:1;
    unsigned int prev_int_enable:1;
    unsigned int unused:28;
};

union psr_values {
    struct psr_bits bits;
    unsigned int integer_part;
};


#endif