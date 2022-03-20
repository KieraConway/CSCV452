#ifndef MESSAGE_H  //Header Guards
#define MESSAGE_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "handler.h"

/* ------------------------- Constants ----------------------------------- */
#define DEBUG2          1

/* Status Constants */
#define EMPTY           0   // Empty index in Mailbox Table/ Slot Table
#define MBOX_RELEASED   1   // Mailbox Table Index is Released
#define MBOX_USED       2   // Mailbox Table Index is Not Empty/ Used
#define SLOT_FULL       3   // Slot is full and holds a message currently
#define WAITING         4   // Process is waiting to send or receive a message
#define SEND_BLOCK      5   // Process is blocked in send
#define RECEIVE_BLOCK   6   // Process is blocked in receive
#define RELEASE_BLOCK   7   // Process is blocked in release
//#define SLOT_BLOCKED    8   // Process is blocked   //todo remove?


/* ------------------------ Typedefs and Structs ------------------------ */
typedef struct procQueue procQueue;
typedef struct slotQueue slotQueue;

typedef struct proc_table proc_table;
typedef struct slot_table slot_table;
typedef struct mailbox mailbox;

typedef struct proc_table * proc_ptr;   //todo: delete if unused
typedef struct slot_table * slot_ptr;


/* Structures for Mailbox Slot Lists */
typedef struct  slotList{
    slot_ptr slot;
    struct  slotList *pNextSlot;    //points to next process
    struct  slotList *pPrevSlot;    //points to previous process
} SlotList;

typedef struct slotQueue{
    SlotList *pHeadSlot;            //points to msg list head
    SlotList *pTailSlot;            //points to msg list tail
    int total;                      //counts total messages
    int mbox_id;                    //mailbox id (for index reference)
}SlotQueue;

struct slot_table {
    int             slot_index;             //index (slot_id % MAXSLOTS)
    int             slot_id;                //unique slot id
    char            message[MAXMESSAGE];    //contains message
    int             messageSize;            //size of message
    int             status;                 //message status
    bool            delivered;              //indicates if message delivered
    int             mbox_id;                //mailbox id for (for index reference)
};
//end of Slot structures

/* Structures for Processes */
typedef struct  procList{
    int             pid;            //process pid
    slot_ptr slot;                  //msg index in SlotTable
    SlotList *pSlot;                //pointer to slot
    struct procList *pNextProc;     //points to next process
    struct procList *pPrevProc;     //points to previous process
} ProcList;

typedef struct procQueue {
    ProcList *pHeadProc;            //points to msg list head
    ProcList *pTailProc;            //points to msg list tail
    int total;                      //counts total messages todo: remove?
    int mbox_id;                    //mailbox id (for index reference)
} ProcQueue;

//end of Process structures

struct mailbox {
    int         mbox_index;         // Location inside MailboxTable
    int         mbox_id;            // Unique Mailbox ID
    int         status;
    int         releaser;           // If mailbox is released, holds the releaser PID
    procQueue   waitingProcs;       // Linked list of waiting processes to be used to send and receive
    procQueue   waitingToSend;      // Linked list of processes waiting to send a message
    procQueue   waitingToReceive;   // Linked list of processes to receive a message
    int         totalProcs;         // Number of total processes
    int         maxSlots;           // Maximum number of slots for mailbox
    int         activeSlots;        // Number of active slots
    int         maxMsgSize;         // Maximum size of slot/ message
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