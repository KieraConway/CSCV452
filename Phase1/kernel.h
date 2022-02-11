#include "stdbool.h"

#define DEBUG 1

typedef struct proc_struct proc_struct;

typedef struct proc_struct * proc_ptr;

//typedef struct status_struct * pStatusNode;

// ADDED FROM WEEK 3 VIDEO
#define STATUS_EMPTY        0
#define STATUS_RUNNING      1
#define STATUS_READY        2
#define STATUS_QUIT         3
#define STATUS_ZAP_BLOCK    4
#define STATUS_JOIN_BLOCK   5
#define STATUS_ZAPPED       6
#define STATUS_ZAPPER       7
#define STATUS_LAST         8
#define TIME_SLICE          80000   //80ms
#define SIBLING_LIST_OFFSET (sizeof(proc_ptr))
#define ZAPPERS_LIST_OFFSET (sizeof(proc_ptr))

/* Structures for Ready/Block Status Lists */
typedef struct statusStruct {
    proc_ptr process;
    struct statusStruct *pNextProc;
    struct statusStruct *pPrevProc;
} StatusStruct;

typedef struct statusQueue {
    StatusStruct *pHeadProc;
    StatusStruct *pTailProc;
} StatusQueue;
/* end of status structure lists */

/* Structure for Process Child Lists */
typedef struct procList{
    struct statusQueue childList[6];    //contains linked list of children
    int total;
}procList;

/* Primary Process Structure */
struct proc_struct {
    proc_ptr        parent_proc_ptr;		//Pointer to Parent
    procList        activeChildren;         //list of active children
    procList        quitChildren;           //list of quit children
    procList        zappers;                //list of all zap
    char            name[MAXNAME];        /* process's name */
    char            start_arg[MAXARG];    /* args passed to process */
    context         state;                /* current context for process */
    short           pid;                  /* process id */
    int             priority;
    int (* start_func) (char *);   /* function where process begins -- launch */
    char            *stack;
    unsigned int    stackSize;
    int             status;         /* READY, BLOCKED, QUIT, etc. */
    /* other fields as needed... */
    int             startTime;				// process start time
    int             switchTime;				// last time switched
    int             totalCpuTime;           // total run time
    int             returnCode;             //
    //totalCpuTime += start time - current time/switchTime
};

/* Lowest Order PSR Bits*/
struct psr_bits {
    unsigned int cur_mode:1;        //variable:bitSize
    unsigned int cur_int_enable:1;
    unsigned int prev_mode:1;
    unsigned int prev_int_enable:1;
    unsigned int unused:28;
};

union psr_values {
    struct psr_bits bits;
    unsigned int integer_part;
};

/* Some useful constants.  Add more as needed... */
#define NO_CURRENT_PROCESS NULL
#define MINPRIORITY 5
#define MAXPRIORITY 1
#define SENTINELPID 1
#define SENTINELPRIORITY LOWEST_PRIORITY

