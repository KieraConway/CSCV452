/* ------------------------------------------------------------------------
    phase1.c

    CSCV 452

    Katelyn Griffith
    Kiera Conway
   ------------------------------------------------------------------------ */
/* * * * * * * * * * * * * * * * * * * * *
 * Author Notes
 *  NEXT:
 *      (1) match function prototypes
 *          from phase1.c and phase1.h
 *      (2) removeList
 *      (3) addList - create blocked option
 *      (4) 429 - implement functions ^
 *
 *  for zapped processes -> use linked list
 *  dont worry about freeing malloc *yet*
 *  procTable[0] will remain empty
* * * * * * * * * * * * * * * * * * * * */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <phase1.h>
#include "kernel.h"

/* ------------------------- Prototypes ----------------------------------- */
int sentinel (char *);              //TODO: void-> char
extern int start1 (char *);
void dispatcher(void);
void launch();
static void enableInterrupts();
static void check_deadlock();

// added functions for processing kg
static void check_kernel_mode(const char *functionName);    //Check Kernel Mode
void ClockIntHandler(int dev, void *arg);                   //Clock handler
void DebugConsole(char *format, ...);                       //Debug console
void InitializeLists();                                     //List Initializer
int GetNextPid();
int check_io();
proc_ptr GetNextReadyProc();
int addProcessLL(int proc_slot, StatusQueue * pStat);
int removeProcessLL(int pid, int priority, StatusQueue * pStat);
bool StatusQueueIsFull(const StatusQueue * pStat);
bool StatusQueueIsEmpty(const StatusQueue * pStat, int index);
static void CopyToQueue(int proc_slot, StatusStruct * pStat);


/* -------------------------- Globals ------------------------------------- */

/* Patrick's debugging global variable... */
int debugflag = 1;

/* the process table */
proc_struct ProcTable[MAXPROC];

/* current process ID */
proc_ptr Current;

/* the next pid to be assigned */
unsigned int next_pid = SENTINELPID;

/* number of active processes */
unsigned int numProc = 0; 	// kg

/* Error Check: context_switch() Initializer */
int init = 1;

/* Total Process Counts */
int totalProc;
int totalReadyProc;
int totalBlockedProc;

/* Process lists  */
struct statusQueue ReadyList[6];      //Ready Processes | 6 to include sentinel
struct statusQueue BlockedList[6];    //Blocked Processes | 6 to include sentinel

/* -------------------------- Functions ----------------------------------- */
/* ------------------------------------------------------------------------
    Name - startup
    Purpose - Initializes process lists and clock interrupt vector.
        Start up sentinel process and the test process.
    Parameters - none, called by USLOSS
    Returns - nothing
    Side Effects - lots, starts the whole thing
   ----------------------------------------------------------------------- */
void startup()
{
    int i;      /* loop index */
    int result; /* value returned by call to fork1() */

    /* initialize the process table */
    memset(ProcTable, 0, sizeof(ProcTable)); // added in kg

    /* Initialize the total counters lists, etc. */
    DebugConsole("startup(): initializing the Ready & Blocked lists\n");
    int totalProc = 0;          //Total Processes
    int totalReadyProc = 0;     //Total Ready Processes
    int totalBlockedProc = 0;   //Total Blocked Processes
    InitializeLists();

    /* Initialize the clock interrupt handler */
    int_vec[CLOCK_INT] = &ClockIntHandler; // added in kg

    /* startup a sentinel process */
    DebugConsole("startup(): calling fork1() for sentinel\n");

    result = fork1("sentinel", sentinel, NULL, USLOSS_MIN_STACK, SENTINELPRIORITY); // call fork1

    if (result < 0)
    {
        DebugConsole("startup(): fork1 of sentinel returned error, halting...\n");
        halt(1);
    }

    init = 0; // added in kg

    /* start the test process */
    DebugConsole("startup(): calling fork1() for start1\n");

    result = fork1("start1", start1, NULL, 2 * USLOSS_MIN_STACK, 1);

    if (result < 0) {
        console("startup(): fork1 for start1 returned an error, halting...\n");
        halt(1);
    }

    DebugConsole("startup(): Should not see this message! ");
    DebugConsole("Returned from fork1 call that created start1\n");

    return;
} /* startup */


/* ------------------------------------------------------------------------
   Name - finish
   Purpose - Required by USLOSS
   Parameters - none
   Returns - nothing
   Side Effects - none
   ----------------------------------------------------------------------- */
void finish()
{
    DebugConsole("in finish...\n");
} /* finish */

/* ------------------------------------------------------------------------
   Name - fork1
   Purpose - Gets a new process from the process table and initializes
             information of the process.  Updates information in the
             parent process to reflect this child process creation.
   Parameters - the process procedure address, the size of the stack and
                the priority to be assigned to the child process.
   Returns - the process id of the created child or -1 if no child could
             be created or if priority is not between max and min priority.
   Side Effects - ReadyList is changed, ProcTable is changed, Current
                  process information changed
   ------------------------------------------------------------------------ */
int fork1(char *name, int (*func)(char *), char *arg, int stackSize, int priority)
{
    /* * * * * * * * * * * * * * * * * * * * * * * *
     * Notes:
     *      Forked processes should start ready
     * * * * * * * * * * * * * * * * * * * * * * * */
    int proc_slot;
    int newPid; // added in kg

    DebugConsole("fork1(): creating process %s\n", name);

    /* Error Checking */
    /* test if in kernel mode; halt if in user mode */
    check_kernel_mode(__func__);

    /* Return if stack size is too small-> no empty slots in process table*/
    // Added flag checks in kg
    if (totalProc > MAXPROC)
    {
        console("There are no empty slots in the process table...\n");
        return -1;
    }

    // Out-of-range priorities
    if ((priority > LOWEST_PRIORITY) || (priority < HIGHEST_PRIORITY))
    {
        console("Process priority is out of range. Must be 1 - 5...\n");
        return -1;
    }

    // func is NULL
    if (func == NULL)
    {
        console("func was null...\n");
        return -1;
    }

    // name is NULL
    if (name && !name[0])
    {
        console("name was null...\n");
        return -1;
    }

    // Stacksize is less than USLOSS_MIN_STACK
    if (stackSize < USLOSS_MIN_STACK)
    {
        console("stackSize was less than minimum stack address...\n");
        return -2;
    }
    /* End of Error Check */

    /* find an empty slot in the process table */
    newPid = GetNextPid();  						//get next process ID
    proc_slot = newPid % MAXPROC;   				//assign slot
    ProcTable[proc_slot].pid = newPid;  			//assign pid
    ProcTable[proc_slot].priority = priority; 		//assign priority
    ProcTable[proc_slot].status = STATUS_READY; 	//assign READY status
    ProcTable[proc_slot].stackSize = stackSize; 	//assign stackSize
    ProcTable[proc_slot].stack = malloc(stackSize) ;//assign stack

    // Check if out of memory - malloc return value
    if (ProcTable[proc_slot].stack == NULL)
    {
        DebugConsole("Out of memory.\n");
        halt(1);
    }

    totalProc++;    //increment total process table count

    /* fill-in entry in process table */
    if ( strlen(name) >= (MAXNAME - 1) ) {
        DebugConsole("fork1(): Process name is too long.  Halting...\n");
        halt(1);
    }

    //Process Name
    strcpy(ProcTable[proc_slot].name, name);

    //Process Function
    ProcTable[proc_slot].start_func = func;


    //Process Function Argument(s)
    if ( arg == NULL )
        ProcTable[proc_slot].start_arg[0] = '\0';
    else if ( strlen(arg) >= (MAXARG - 1) ) {
        console("fork1(): argument too long.  Halting...\n");
        halt(1);
    }
    else
        strcpy(ProcTable[proc_slot].start_arg, arg);

    /* * * * * * * * * * * * * * * *
     * TODO:
     * if(Current != NULL){
     *      add to ready list
     *      connect parents children
     * }
     * * * * * * * * * * * * * * * * /
    /* set the parent and child values */
    if(Current != NULL) {
        ProcTable[proc_slot].parent_proc_ptr = Current;
        Current->child_proc_ptr = &ProcTable[proc_slot];
    }
    else{
        ProcTable[proc_slot].parent_proc_ptr = NULL;
    }

    /* Assign to Ready List */
    addProcessLL(proc_slot, &ReadyList);   //add process to linked list



    /* Initialize context for this process, but use launch function pointer for
     * the initial value of the process's program counter (PC)
     */
    context_init(&(ProcTable[proc_slot].state), psr_get(),
                 ProcTable[proc_slot].stack,
                 ProcTable[proc_slot].stackSize, launch);

    /* for future phase(s) */
    p1_fork(ProcTable[proc_slot].pid);

    if (!init)
    {
        // Call dispatcher
        dispatcher();
    }

    return newPid;
} /* fork1 */

/* ------------------------------------------------------------------------
   Name - launch
   Purpose - Dummy function to enable interrupts and launch a given process
             upon startup.
   Parameters - none
   Returns - nothing
   Side Effects - enable interrupts
   ------------------------------------------------------------------------ */
void launch()
{
    int result;

    DebugConsole("launch(): started\n");

    /* Enable interrupts */
    enableInterrupts();

    /* Call the function passed to fork1, and capture its return value */
    result = Current->start_func(Current->start_arg);

    DebugConsole("Process %d returned to launch\n", Current->pid);

    quit(result);

} /* launch */

/* ------------------------------------------------------------------------
   Name - join
   Purpose - Wait for a child process (if one has been forked) to quit.  If
             one has already quit, don't wait.
   Parameters - a pointer to an int where the termination code of the
                quitting process is to be stored.
   Returns - the process id of the quitting child joined on.
		-1 if the process was zapped in the join
		-2 if the process has no children
   Side Effects - If no child process has quit before join is called, the
                  parent is removed from the ready list and blocked.
   ------------------------------------------------------------------------ */
int join(int *code)
{
    check_kernel_mode(__func__);

    // if no children return -2

    // children identified -> block parent process until one child exits
    Current->status = STATUS_BLOCKED;

    dispatcher();

} /* join */


/* ------------------------------------------------------------------------
   Name - quit
   Purpose - Stops the child process and notifies the parent of the death by
             putting child quit info on the parents child completion code
             list.
   Parameters - the code to return to the grieving parent
   Returns - nothing
   Side Effects - changes the parent of pid child completion status list.
   ------------------------------------------------------------------------ */
void quit(int code)
{
    p1_quit(Current->pid);

    //kill the process

    //context switch

} /* quit */

/* ------------------------------------------------------------------------
   Name - GetNextReadyProc
   Purpose -
   Parameters - none
   Returns - nothing
   Side Effects - the context of the machine is changed
   ----------------------------------------------------------------------- */
proc_ptr GetNextReadyProc()
{
    int highestPrior = 6;
    proc_ptr pNextProc = NULL;

    /*
     * TODO
     * Check is sentinel is only process
     */

    //iterate through 1, 2, 3, 4, 5,

    for (int i = 0; i < (MAXPROC); i++)
    {
        //Iterate through ReadyList by index to find next process
        if(ReadyList[i].pHeadProc){
            pNextProc = ReadyList[i].pHeadProc->process;
            break;
        }
    }

    return pNextProc; // return pointer to next ready proc
}


/* ------------------------------------------------------------------------
   Name - dispatcher
   Purpose - dispatches ready processes.  The process with the highest
             priority (the first on the ready list) is scheduled to
             run.  The old process is swapped out and the new process
             swapped in.
   Parameters - none
   Returns - nothing
   Side Effects - the context of the machine is changed
   ----------------------------------------------------------------------- */
void dispatcher(void)
{
    proc_ptr oldProcess; // added in kg
    proc_ptr next_Process;

    //p1_switch(Current->pid, next_process->pid); TODO

    check_kernel_mode(__func__);

    /* Make sure Current is pointing to the process we are switching to
    must be done Before context_switch() */
    next_Process = GetNextReadyProc(); //assign newProcess; added in kg
    oldProcess = Current;   // added in kg
    Current = next_Process; // added in kg

    /* Update Status Values*/
    //update Current
    Current->status = STATUS_RUNNING;       //update to Running Status
    removeProcessLL(next_Process->pid, next_Process->priority, ReadyList);    //remove from readyList

    //update oldProcess //TODO BLOCKED OR READY?
    if(oldProcess != NULL) {
        /*
         * if oldProcess ?
         *      add to readyList
         * else if oldProcess ?
         *      add to blockedList
         */

        int proc_slot = (oldProcess->pid)%MAXPROC;

        //Set oldProcess status to ready
        oldProcess->status = STATUS_READY;

        //add old process to ready list
        if (addProcessLL(proc_slot, &ReadyList) != 0) {
            console("Error: %s was not added to ReadyList\nExiting Program\n", oldProcess->name);
            halt(1);
        }
    }

    //Switch and Start current process
    context_switch((oldProcess == NULL) ? NULL : &oldProcess->state, &next_Process->state);


    //Set next_process to Running
    next_Process->status == STATUS_RUNNING;

    //take next proc off ready list
    //int check = removeProcessLL(oldProcess.pid, oldProcess.priority, ReadyList); // added in kg

    // At this point, we need to figure out whether an old process needs to be put into blocked or ready

    /*
        if blocked -> set to ready eventually
        if running -> can set to either blocked or ready
        if ready -> can set to running

        I was doing some research and I found some info on the processes on slide 8
        I also found some info on context switching on slide 28 if we needed to look at it
        https://www.cs.swarthmore.edu/~kwebb/cs45/s18/03-Process_Context_Switching_and_Scheduling.pdf

        I also found this: https://www.javatpoint.com/what-is-the-context-switching-in-the-operating-system
        It also has some sections on deadlocks as well
    */

} /* dispatcher */


/* ------------------------------------------------------------------------
   Name - sentinel
   Purpose - The purpose of the sentinel routine is two-fold.  One
             responsibility is to keep the system going when all other
	     processes are blocked.  The other is to detect and report
	     simple deadlock states.
   Parameters - none
   Returns - nothing
   Side Effects -  if system is in deadlock, print appropriate error
		   and halt.
   ----------------------------------------------------------------------- */
int sentinel (char * dummy)
{
    DebugConsole("sentinel(): called\n");

    while (1)
    {
        check_deadlock();
        waitint();
    }
} /* sentinel */

int zap(int pid)
{
    // TODO
    //call is_zapped()
    // won't return until zapped prcoess has called quit
    // print error msg and halt(1) if process tries to zap itself
    // or attempts to zap a non-existent prcoess

    // return values:
    // -1 - calling process itself was zapped while in zap
    // 0 - zapped process has called quit
}


/*
 * Returns the pid of Current
*/
int getpid(void)
{
    // TODO: check if code works
    return Current->pid;
}



/*
 * Checks if a process status is zapped
*/
int is_zapped(void)
{
    // TODO
    // return 0 if not zapped
    // return 1 if zapped
    //check non existent
}


void dump_processes(void)
{
    // TODO
    // prints process ifnro to console
    // for each PCB, output:
    // PID, parent' PID, priority, process status, # children, CPU time consumed, and name
}


/* check to determine if deadlock has occurred... */
static void check_deadlock()
{
    if (check_io() == 1)
        return;

    // TODO
    /* Has everyone terminated? */
    // check the number of process
    // if there is only one active prcoess
    // halt(0);
    //otherwise
    //halt(1);

    /* Code from Office Hours
    int readyCount = 0;

    for (int i = HIGHEST_PRIORITY; i < LOWEST_PRIORITY; ++i){
        readyCount += totalReadyProc[i-1].count;
    }

    // Has everyone terminated?
    if(readyCount == 0) {
        if (numProc > 1) {
            console("check_deadlock: numProc is: %d\n, numProc");
            console("check_deadlock: processes still present, halting...\n");
            halt(1);
        } else {
            console("All Processes Completed.\n");
            halt(0);
        }
    }
    */



} /* check_deadlock */


/*
 * Enable the interrupts.
 */
static void enableInterrupts()
{
    check_kernel_mode(__func__);
    /*Confirmed Kernel Mode*/

    //TODO: Check if already enabled

    int curPsr = psr_get();

    curPsr = curPsr | PSR_CURRENT_INT;

    psr_set(curPsr);

} /* enableInterrupts */


/*
 * Disables the interrupts.
 */
void disableInterrupts()
{
    /* turn the interrupts OFF if we are in kernel mode */
    if((PSR_CURRENT_MODE & psr_get()) == 0) {
        //not in kernel mode
        console("Kernel Error: Not in kernel mode, may not disable interrupts\n");
        halt(1);
    } else
        /* We ARE in kernel mode */
        psr_set( psr_get() & ~PSR_CURRENT_INT );
} /* disableInterrupts */


/* ------------------------------------------------------------------------
   Name - check_kernel_mode
   Purpose - Checks the PSR for kernel mode and halts if in user mode
   Parameters - functionName
   Returns - nothing
   Side Effects - Will halt if not kernel mode
   ----------------------------------------------------------------------- */
static void check_kernel_mode(const char *functionName)
{
    union psr_values psrValue; /* holds caller's psr values */

    DebugConsole("check_kernel_node(): verifying kernel mode for %s\n", functionName);

    /* test if in kernel mode; halt if in user mode */
    psrValue.integer_part = psr_get();  //returns int value of psr

    if (psrValue.bits.cur_mode == 0)
    {
        console("Kernel mode expected, but function called in user mode.\n");
        halt(1);
    }

    DebugConsole("Function is in Kernel mode (:\n");
}


/* ------------------------------------------------------------------------
Name - DebugConsole
Purpose - Prints the message to the console if in debug mode
Parameters - format string and va args
Returns - nothing
Side Effects -
----------------------------------------------------------------------- */
void DebugConsole(char *format, ...)
{
    if (DEBUG && debugflag)
    {
        va_list argptr;
        va_start(argptr, format);
        console(format, argptr);
        va_end(argptr);


    }
}


/* ------------------------------------------------------------------------
Name - ClockIntHandler
Purpose -
Parameters -
Returns - nothing
Side Effects -
----------------------------------------------------------------------- */
void ClockIntHandler(int dev, void *arg)
{
    int i = 0;
    // time-slice = 80 milliseconds
    // detect 80 ms -> switch to next highest priority processor
    //if (Current->runtime > 80ms) dispatcher();

    sys_clock(); //returns time in microseconds

    //TODO
    // phase1notes pg 6
    /*If current process exceeds 80ms
        call dispatcher
    else
        return
    */
    /*
    * if (current->runTime > 80ms)
    *  dispatcher();
    */

    return;
}


/* ------------------------------------------------------------------------
Name - GetNextPid
Purpose - Obtain next pid whose % MAXPROC is open in the ProcTable
Parameters -
Returns - NewPid or -1 if ProcTable is full
Side Effects -
----------------------------------------------------------------------- */
int GetNextPid()
{
    int newPid = -1;
    int procSlot = next_pid % MAXPROC;

    if (numProc < MAXPROC)
    {
        while ((numProc < MAXPROC) && (ProcTable[procSlot].status != STATUS_EMPTY))
        {
            next_pid++;
            procSlot = next_pid % MAXPROC;
        }

        newPid = next_pid++;	//assign newPid *then* increment next_pid
    }

    return newPid;
}


/* ------------------------------------------------------------------------
 * TODO:
Name - check_io
Purpose -
Parameters -
Returns -
Side Effects -
----------------------------------------------------------------------- */
// This is apparently all we need -> will be important for phase 2
int check_io()
{
    return 0;
}


/* ------------------------------------------------------------------------
 * TODO:
Name - InitializeLists
Purpose - Initialize the values in the Status List
Parameters -
Returns -
Side Effects -
----------------------------------------------------------------------- */
void InitializeLists(){
    int i;

    //initialize head/tail of ReadyList and BlockedList
    for(i=0; i<5; i++){
        ReadyList[i].pHeadProc = ReadyList[i].pTailProc = NULL;
        BlockedList[i].pHeadProc = BlockedList[i].pTailProc = NULL;
    }

    return;
}


/* ------------------------------------------------------------------------
 * TODO:
Name - addProcessLL
Purpose - Status List to add process to
Parameters - proc_slot
Returns - 0 Success, -1 Failure
Side Effects -
----------------------------------------------------------------------- */
int addProcessLL(int proc_slot, StatusQueue * pStat){

    StatusStruct * pNew;    //new Node
    StatusStruct * pSave;   //save previous location in loop

    //check if queue is full
    if(StatusQueueIsFull(pStat)){
        return -1;
    }

    pNew = (StatusStruct *) malloc (sizeof (StatusStruct));

    //verify allocation
    if (pNew == NULL){
        console("Error: Failed to allocate memory for node in linked list"); //Error Flag kg
        halt(1);    //memory allocation failed
    }

    CopyToQueue(proc_slot, pNew);
    int index = (pNew->process->priority)-1;

    pNew->pNextProc = NULL;

    if(StatusQueueIsEmpty(pStat, index)){           //if index is empty...
        pStat[index].pHeadProc = pNew;                    //add to front of list/index
    }
    else{
        pStat[index].pTailProc->pNextProc = pNew;         //add to end of list/index
    }

    pStat[index].pTailProc = pNew;                        //reassign pTail to new Tail

    //if readyList
    if (pStat == ReadyList){    //compare memory locations to verify list type
        totalReadyProc++;       //increment total ready processes
    }
        //if blockedList
    else if(pStat == BlockedList){ //compare memory locations to verify list type
        totalBlockedProc++;        //increment total ready processes
    }

    return 0;
}


/* ------------------------------------------------------------------------
 * TODO:
Name - removeProcessLL
Purpose - Status List to remove process from
Parameters - proc_slot
Returns - 0 Success, halt(1) on Failure
Side Effects -
----------------------------------------------------------------------- */
int removeProcessLL(int pid, int priority, StatusQueue * pStat){
    int index = priority - 1;
    StatusStruct * pSave;   //pointer for LL
    int pidFlag = 0;        //verify pid found in list
    int remFlag = 0;        //verify process removed successfully

    pSave = pStat[index].pHeadProc;    //Find correct index, Start at head

    //iterate through list
    while(pSave->process != NULL){    //verify not end of list

        if(pSave->process->pid == pid){ //if found process to remove
            pidFlag = 1;                   //Trigger Flag - PID found

            /* If process to be removed is at head */
            if(pSave == pStat[index].pHeadProc){
                pStat[index].pHeadProc = pSave->pNextProc; //Set next process as head
                pSave->pNextProc = NULL;                   //Set old head next pointer to NULL
                if(pStat[index].pHeadProc != NULL) {           //if not only one node on list
                    pStat[index].pHeadProc->pPrevProc = NULL;  //Set new head prev pointer to NULL
                }
                else{           //if only node on list
                    pSave->pPrevProc = NULL;
                    pStat[index].pTailProc = NULL;

                }
                remFlag = 1;                               //Trigger Flag - Process Removed
                break;
            }

            /* If process to be removed is at tail */
            else if(pSave == pStat[index].pTailProc){
                pStat[index].pTailProc = pSave->pPrevProc;  //Set prev process as tail
                pSave->pPrevProc = NULL;                   //Set old tail prev pointer to NULL
                pStat[index].pTailProc->pNextProc = NULL; //Set new tail next pointer to NULL
                remFlag = 1;
                break;
            }

            /* If process to be removed is in middle */
            else{
                pSave->pPrevProc->pNextProc = pSave->pNextProc; //Set pSave prev next to pSave next
                pSave->pNextProc->pPrevProc = pSave->pPrevProc; //Set pSave next prev to pSave prev
                pSave->pNextProc = NULL;    //Set old pNext to NULL
                pSave->pPrevProc = NULL;    //Set old pPrev to NULL
                remFlag = 1;
                break;
            }

        }
        else{
            pSave = pSave->pNextProc;
        }

    }

    if(pidFlag && remFlag){
        free(pSave);

        //if readyList
        if (pStat == ReadyList){    //compare memory locations to verify list type
            totalReadyProc--;       //decrement total ready processes
        }
        //if blockedList
        else if(pStat == BlockedList){
            totalBlockedProc--;
        }

        return 0;
    }
    else {
        if (pidFlag == 0){
            console("Error: Unable to locate process pid [%d]\n", pid);
            halt(1);
        }
        else if (remFlag == 0) {
            console("Error: Removal of process [%d] unsuccessful\n", pid);
            halt(1);
        }
    }
}


/*TODO
 * Check if StatusQueue is Full
 */
bool StatusQueueIsFull(const StatusQueue * pStat){
    //if readyList
    if (pStat == ReadyList){    //compare memory locations to verify list type
        return totalReadyProc >= MAXPROC;
    }
    else if(pStat == BlockedList){
        return totalBlockedProc >= MAXPROC;
    }
}

bool StatusQueueIsEmpty(const StatusQueue * pStat, int index){
    return pStat[index].pHeadProc == NULL;

    //if ready
    //check ready
    //if blocked
    //check blocked

}
/*
 * Copy to Queue
 */
static void CopyToQueue(int proc_slot, StatusStruct * pStat){
    pStat->process = &ProcTable[proc_slot];
}