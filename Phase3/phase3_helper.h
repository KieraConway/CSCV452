/* Header File for Phase 3 */

#ifndef PHASE_3_HELPER_H
#define PHASE_3_HELPER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tables.h"

/** ------------------------- Constants ----------------------------------- **/
#define DEBUG3 1

#define STATUS_EMPTY        0
#define STATUS_RUNNING      1
#define STATUS_READY        2
#define STATUS_QUIT         3
#define STATUS_ZAP_BLOCK    4
#define STATUS_JOIN_BLOCK   5
#define STATUS_ZAPPED       6
#define STATUS_LAST         7

#define LOWEST_PRIORITY     5
#define HIGHEST_PRIORITY    1
#define ITEM_IN_USE         1

/** ------------------------ Typedefs and Structs ------------------------ **/
typedef struct usr_proc_struct usr_proc_struct;
typedef struct usr_proc_struct * usr_proc_ptr;

struct usr_proc_struct {
    char                name[MAXNAME];        	    // process's name
    int                 index;                      // getpid()%MAXPROC
    char                start_arg[MAXARG];    	    // args passed to process
    int                 (* start_func) (char *);    // function where process begins -- launch
    short               pid;                  	    // process id
    unsigned int        stacksize;
    int                 priority;
    int                 pMboxID;                    // private mailboxID
    int                 status;
    // prob need more variables here for lists
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
/* --------------------- External tables.c Prototypes --------------------- */
/** Process Table **/
/* Finds Next Available pid | Returns Next p_id */
extern int GetNextPID();
/* Initializes the new mailbox */
extern void InitializeProcTable();
/* Gets Mailbox index from id | Returns Corresponding index */
extern int GetProcIndex(int pid);

//end external function prototypes
#endif