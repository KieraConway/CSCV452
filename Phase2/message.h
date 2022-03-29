#ifndef MESSAGE_H  //Header Guards
#define MESSAGE_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "handler.h"
#include "tables.h"
#include "lists.h"

/** ------------------------- Constants ----------------------------------- **/
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


/** ------------------------ Typedefs and Structs ------------------------ **/
typedef struct procQueue procQueue;
typedef struct procList * proc_ptr;

typedef struct slot_table * table_ptr;
typedef struct slot_table slot_table;
typedef struct slotQueue slotQueue;
typedef struct slotList * slot_ptr;

typedef struct mailbox mailbox;

/* Structures for Mailbox Slot Lists */
typedef struct  slotList{
    table_ptr       slot;
    int             slot_id;       //unique slot id
    struct slotList *pNextSlot;    //points to next process
    struct slotList *pPrevSlot;    //points to previous process
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
    int             mbox_id;                //mailbox id for (for index reference)
};
//end of Slot structures

/* Structures for Processes */
typedef struct  procList{
    int         pid;                //process pid
    table_ptr   zeroSlot;           //points to SlotTable for zero slots
    int         slot_id;            //unique slot queue id that holds msg
    bool        delivered;          //indicates if zero-slot message delivered
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
    int         status;             // Contains the Mailbox Status [0-2]
    int         releaser;           // If mailbox is released, holds the releaser PID
    procQueue   activeProcs;        // Linked list of active processes
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

/** ------------------------ External Prototypes ------------------------ **/
/* --------------------- External lists.c Prototypes --------------------- */
/** General, Multi-use **/
/* Initializes Linked List */
extern void InitializeList(slotQueue *pSlot, procQueue *pProc);
/* Check if Slot List is Full | Returns True when Full */
extern bool ListIsFull(const slotQueue *pSlot, const mailbox *pMbox, const procQueue *pProc);
/* Check if Slot List is Empty | Returns True when Empty */
extern bool ListIsEmpty(const slotQueue *pSlot, const procQueue *pProc);

/** Slot Lists **/
/* Add pSlot to Mailbox Queue | Returns 0 on Success */
extern int AddSlotLL(slot_table * pSlot, mailbox * pMbox);
/* Remove pSlot from Mailbox Queue | Returns slot_id of Removed Slot */
extern int RemoveSlotLL(int slot_id, slotQueue * sq);
/* Finds Slot in Slot Queue | Returns Pointer to Found Slot */
extern slot_ptr FindSlotLL(int slot_id, slotQueue * pq);

/** Process Lists **/
/* Add pid to Process Queue */
extern void AddProcessLL(int pid, procQueue * pq);
/* Remove pid from Process Queue | Returns Pid of Removed Process*/
extern int RemoveProcessLL(int pid, procQueue * pq);
/* Copies a Process to another Queue | Returns Pointer to Copied Process */
extern proc_ptr CopyProcessLL(int pid, procQueue * sourceQueue, procQueue * destQueue);
/* Finds Process in Process Queue | Returns Pointer to Found Process */
extern proc_ptr FindProcessLL(int pid, procQueue * pq);


/* --------------------- External tables.c Prototypes --------------------- */
/** Mailbox Table **/
/* Finds Next Available mbox_id | Returns Next mbox_id */
extern int GetNextMailID();
/* Initializes the new mailbox */
extern void InitializeMailbox(int newStatus, int mbox_index, int mbox_id, int maxSlots, int maxMsgSize);
/* Gets Mailbox index from id | Returns Corresponding index */
extern int GetMboxIndex(int mbox_id);

/** Mailbox Table Release **/
/* A Helper Function for mboxRelease */
extern void HelperRelease(mailbox *pMbox);
/* Checks if mailbox was released | Returns True if Released */
extern bool MboxWasReleased(mailbox *pMbox);


/** Slot Table **/
/* Finds Next Available slot_id | Returns Next slot_id */
extern int GetNextSlotID();
/* Initializes Slot to Match Passed Parameters */
extern void InitializeSlot(int newStatus, int slot_index, int slot_id, void *msg_ptr, int msgSize, int mbox_id);
/* Gets Slot index from id | Returns Corresponding index */
extern int GetSlotIndex(int slot_id);

//end external function prototypes
#endif