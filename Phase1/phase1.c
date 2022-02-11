/* ------------------------------------------------------------------------
    phase1.c

    CSCV 452

    Katelyn Griffith
    Kiera Conway
   ------------------------------------------------------------------------ */
/* * * * * * * * * * * * * * * * * * * * *
 * Author Notes
 *  NEXT:
 *      (1) Join() - Psuedo code listed in function
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
static void disableInterrupts();
static void check_deadlock();

// added functions for processing kg
static void check_kernel_mode(const char *functionName);    //Check Kernel Mode
void ClockIntHandler(int dev, void *arg);                   //Clock handler
void DebugConsole(char *format, ...);                       //Debug console
void InitializeLists();                                     //List Initializer
int GetNextPid();
int check_io();
proc_ptr GetNextReadyProc();
void verifyProcess(char *name, int priority, int *func, int stackSize);
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
Name -          startup
Purpose -       Initializes process lists and clock interrupt vector.
                Start up sentinel process and the test process.
Parameters -    none, called by USLOSS
Returns -       nothing
Side Effects -  lots, starts the whole thing
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
Name -          finish
Purpose -       Required by USLOSS
Parameters -    none
Returns -       nothing
Side Effects -  none
   ----------------------------------------------------------------------- */
void finish()
{
    DebugConsole("in finish...\n");
} /* finish */

/* ------------------------------------------------------------------------
Name -          fork1
Purpose -       Gets a new process from the process table and initializes
                    information of the process.  Updates information in the
                    parent process to reflect this child process creation.
Parameters -    The process procedure address, the size of the stack and
                    the priority to be assigned to the child process.
Returns -       The process id of the created child or -1 if no child could
                    be created or if priority is not between max and min priority.
Side Effects -  ReadyList is changed, ProcTable is changed, Current
                    process information changed
------------------------------------------------------------------------ */
int fork1(char *name, int (*func)(char *), char *arg, int stackSize, int priority) {
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

    verifyProcess(name, priority, func, stackSize);

    /* find an empty slot in the process table */
    newPid = GetNextPid();                        //get next process ID
    proc_slot = newPid % MAXPROC;                //assign slot
    ProcTable[proc_slot].pid = newPid;            //assign pid
    ProcTable[proc_slot].priority = priority;        //assign priority
    ProcTable[proc_slot].status = STATUS_READY;    //assign READY status
    ProcTable[proc_slot].stackSize = stackSize;    //assign stackSize
    ProcTable[proc_slot].stack = malloc(stackSize);//assign stack

    // Check if out of memory - malloc return value
    if (ProcTable[proc_slot].stack == NULL) {
        DebugConsole("Out of memory.\n");
        halt(1);
    }

    totalProc++;    //increment total process table count

    /* fill-in entry in process table */
    if (strlen(name) >= (MAXNAME - 1)) {
        DebugConsole("fork1(): Process name is too long.  Halting...\n");
        halt(1);
    }

    //Process Name
    strcpy(ProcTable[proc_slot].name, name);

    //Process Function
    ProcTable[proc_slot].start_func = func;


    //Process Function Argument(s)
    if (arg == NULL)
        ProcTable[proc_slot].start_arg[0] = '\0';
    else if (strlen(arg) >= (MAXARG - 1)) {
        console("fork1(): argument too long.  Halting...\n");
        halt(1);
    } else
        strcpy(ProcTable[proc_slot].start_arg, arg);

    /* * * * * * * * * * * * * * * *
     * TODO:
     * if(Current != NULL){
     *      add to ready list
     *      connect parents children
     * }
     * * * * * * * * * * * * * * * * /
    /* set the parent and child values */
    if (Current != NULL) {
        ProcTable[proc_slot].parent_proc_ptr = Current;

        //Add child to end of doubly linked child list
        if ((addProcessLL(proc_slot, &Current->activeChildren.childList)) == -1){
            console("Error: Unable to add %s to list of children processes.", ProcTable[proc_slot].name);
            halt(1);
        }
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
Name -           launch
Purpose -        Dummy function to enable interrupts and launch a given process
                    upon startup.
Parameters -     none
Returns -        nothing
Side Effects -   enable interrupts
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
Name -          join
Purpose -       Wait for a child process (if one has been forked) to quit.  If
                    one has already quit, don't wait.
Parameters -    A pointer to an int where the termination code of the
                    quitting process is to be stored.
Returns -       The process id of the quitting child joined on.
		            -1 if the process was zapped in the join
		            -2 if the process has no children
Side Effects -  If no child process has quit before join is called, the
                    parent is removed from the ready list and blocked.
   ------------------------------------------------------------------------ */
int join(int *code)
{
    //Ensure kernel mode
    check_kernel_mode(__func__);

    //todo: maybe?
    //disableInterrupts();

    //Error Check: Process has no children
    if(Current->activeChildren.total == 0 && Current->quitChildren.total == 0){
        console("Error: %s has no children", Current->name);
        return (-2);
    }
    //TODO:
    // Error Check: child(ren) quit before join() occurred //if quitChildren == 0
        //return pid and quit status of child
        //clean child from process table

    Current->status = STATUS_JOIN_BLOCK;    //change parent status to join_block
    removeProcessLL(Current->pid, Current->priority, ReadyList); //remove from ReadyList

    // Call dispatcher
    dispatcher();


    //DONE//block parent
        //DONE//Current->status = STATUS_JOIN_BLOCK;
    //todo: lines below
    //execute child until quit (save returnCode)
        //*code = child->returnCode
    //remove child from procTable
        //free(pChild->stack)
        //memset(&ProcTable[child_slot], 0, sizeof(proc)struct));
    //put parent on ready list






    dispatcher();

} /* join */


/* ------------------------------------------------------------------------
Name -          quit
Purpose -       Stops the child process and notifies the parent of the death by
                    putting child quit info on the parents child completion code
                    list.
Parameters -    the code to return to the grieving parent
Returns -       nothing
Side Effects -  changes the parent of pid child completion status list.
------------------------------------------------------------------------ */
void quit(int code)
{
    //notify parent that proc is quitting
    //pass code to parent (join returns, join (code))
    //add code to process table (i.e name, pid, etc)
    //quit(-1) = the code is -1

    //save return code
    //Current->resultCode = code;

    //tell the parent process to un-block
    //Current-> status = status ready

    //change status to quit
    //current status = quit;
    //do NOT kill here, parent will kill

    p1_quit(Current->pid);

    //dispatcher();
} /* quit */

/* ------------------------------------------------------------------------
Name -          GetNextReadyProc
Purpose -
Parameters -    None
Returns -       pointer to next ready process
Side Effects -  the context of the machine is changed
----------------------------------------------------------------------- */
proc_ptr GetNextReadyProc()
{
    proc_ptr pNextProc = NULL;          //saves found process
    int priorityLimit = LOWEST_PRIORITY;

    if(Current && Current->status == STATUS_RUNNING){   //If there is current and its running

        pNextProc = Current;                            //Set to current

        priorityLimit = (Current->priority);    //Set limit to current priority
    }


    for (int i = 0; i < priorityLimit; i++)
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
Name -          dispatcher
Purpose -       Dispatches ready processes.  The process with the highest
                    priority (the first on the ready list) is scheduled to
                    run.  The old process is swapped out and the new process
                    swapped in.
Parameters -    None
Returns -       Nothing
Side Effects -  The context of the machine is changed
----------------------------------------------------------------------- */
void dispatcher(void)
{
    proc_ptr prevProc;  //previously running process
    proc_ptr nextProc;  //next process to run
    int proc_slot;
    int curPid;
    int currentTime = sys_clock();
    disableInterrupts();

    int testTime = sys_clock();

    prevProc = Current;     //save current process

    if (Current != NULL) {   //Verify currently running process exists
        Current->totalCpuTime += currentTime - Current->switchTime; //calculate total cpu time
        proc_slot = (Current->pid) % MAXPROC;     //find current proc_slot
        curPid = Current->pid;                    //set current pid

        /* verify time slice is not exceeded */
        if (Current->status == STATUS_RUNNING) {  //if current status is running (not blocked)
            if ((currentTime - ((Current->switchTime) / 1000)) > TIME_SLICE) {  //if exceeded

                if (addProcessLL(proc_slot, &ReadyList) != 0) {     //add Current to ready List

                    console("Error: %s was not added to ReadyList\nExiting Program\n", Current->name);
                    halt(1);
                }
                Current->status = STATUS_READY; //switch Current status to ready
            }
        }
    }
    nextProc = GetNextReadyProc(); //assign newProcess; added in kg

    //If there is no ready process found, exit program with error
    if(nextProc == NULL){
        console("Error: There are no ready processes");
        halt(1);
    }

    if (nextProc != Current){       //if nextProc priority is different from Current ...

        if(Current != NULL && Current->status == STATUS_RUNNING){
            if (addProcessLL(proc_slot, &ReadyList) != 0) {     //add Current to ready List

                console("Error: %s was not added to ReadyList\nExiting Program\n", Current->name);
                halt(1);
            }
        }

        /* Updating nextProc */
        removeProcessLL(nextProc->pid, nextProc->priority, ReadyList);    //remove from readyList
        nextProc->status = STATUS_RUNNING;      //switch status
        nextProc->switchTime = currentTime;     //save time of process switch
        p1_switch(curPid, nextProc->pid);
        Current = nextProc;     //switch current to nextProc

        enableInterrupts();

        //Switch and Start current process
        context_switch((prevProc == NULL) ? NULL : &prevProc->state, &nextProc->state);
    }
    else{
        Current->totalCpuTime += currentTime - Current->switchTime;     //calculate total cpu time
        Current->switchTime = currentTime;                             //reset switch time
        if(Current->status == STATUS_READY) {
            removeProcessLL(Current->pid, Current->priority, ReadyList);
            Current->status = STATUS_RUNNING; //switch Current status to ready
        }
    }
} /* end of dispatcher */


/* ------------------------------------------------------------------------
Name -          sentinel
Purpose -       The purpose of the sentinel routine is two-fold.  One
                    responsibility is to keep the system going when all other
	                processes are blocked.  The other is to detect and report
	                simple deadlock states.
Parameters -    none
Returns -       nothing
Side Effects -  if system is in deadlock, print appropriate error
		        and halt.
----------------------------------------------------------------------- */
int sentinel (char * dummy)
{
    //determine deadlock scenario
    //determine if no running processes

    DebugConsole("sentinel(): called\n");

    while (1)
    {
        check_deadlock();
        waitint();
    }
} /* sentinel */

/* ------------------------------------------------------------------------
 * TODO
Name -          zap
Purpose -
Parameters -    Pid of function to zap
Returns -       0:  zapped process has called quit
                -1: calling process itself was zapped while in zap
Side Effects -
----------------------------------------------------------------------- */
int zap(int pid)
{
    // TODO
    //zap specifies specific code
    //call is_zapped()
    // won't return until zapped prcoess has called quit
    // print error msg and halt(1) if process tries to zap itself
    // or attempts to zap a non-existent prcoess

}

/* ------------------------------------------------------------------------
 * TODO
Name -          getpid
Purpose -       To find and return the pid of Current
Parameters -    None
Returns -       Returns the pid of Current
Side Effects -
   ----------------------------------------------------------------------- */
int getpid(void)
{
    // TODO: check if code works
    return Current->pid;
}


/* ------------------------------------------------------------------------
 * TODO
Name -          is_zapped
Purpose -
Parameters -    Checks if a process status is zapped
Returns -
Side Effects -
   ----------------------------------------------------------------------- */
int is_zapped(void)
{
    // TODO
    // return 0 if not zapped
    // return 1 if zapped
    //check non existent
}

/* ------------------------------------------------------------------------
 * TODO
Name -          dump_processes
Purpose -
Parameters -    None
Returns -
Side Effects -
   ----------------------------------------------------------------------- */
void dump_processes(void)
{
    // TODO
    // prints process ifnro to console
    // for each PCB, output:
    // PID, parent' PID, priority, process status, # children, CPU time consumed, and name
}

/* ------------------------------------------------------------------------
 * TODO
Name -          check_deadlock
Purpose -       check to determine if deadlock has occurred
Parameters -    None
Returns -
Side Effects -
   ----------------------------------------------------------------------- */
static void check_deadlock()
{
    if (check_io() == 1)
        return;

    //NOTES/SCREENSHOTS IN WEEK 5 MEETING - KC

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



}


/* ------------------------------------------------------------------------
 * TODO
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

    //TODO: Check if already enabled

    int curPsr = psr_get();

    curPsr = curPsr | PSR_CURRENT_INT;

    psr_set(curPsr);
}


/* ------------------------------------------------------------------------
 * TODO
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
        console("Kernel Error: Not in kernel mode, may not disable interrupts\n");
        halt(1);
    } else
        /* We ARE in kernel mode */
        psr_set( psr_get() & ~PSR_CURRENT_INT );
}


/* ------------------------------------------------------------------------
Name -          check_kernel_mode
Purpose -       Checks the PSR for kernel mode and halts if in user mode
Parameters -    functionName
Returns -       nothing
Side Effects -  Will halt if not kernel mode
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
Name -          DebugConsole
Purpose -       Prints the message to the console if in debug mode
Parameters -    format string and va args
Returns -       nothing
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
Name -          ClockIntHandler
Purpose -
Parameters -
Returns -       nothing
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
Name -          GetNextPid
Purpose -       Obtain next pid whose % MAXPROC is open in the ProcTable
Parameters -
Returns -       NewPid or -1 if ProcTable is full
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
Name - verifyProcess
Purpose - Verify process input is correct
Parameters -
Returns -
Side Effects -
----------------------------------------------------------------------- */
void verifyProcess(char *name, int priority, int *func, int stackSize){
    // Error Check Process Input
    if (totalProc > MAXPROC){   // Return if size is too small or proc table is full
        console("Error: Process Table is Full...\n");
        halt(1);
    }
    else if ((priority > LOWEST_PRIORITY) || (priority < HIGHEST_PRIORITY)){    //Out-of-range priorities
        console("Error: Process priority [%d] is out of range [1 - 5].\n", priority);
        halt(1);
    }
    else if (func == NULL){                     //Function is NULL
        console("Error: Function was NULL.\n");
        halt(1);
    }
    else if (name && !name[0]){                 //Name is NULL
        console("Error: Name was NULL.\n");
        halt(1);
    }
    else if(stackSize < USLOSS_MIN_STACK){     // Stacksize is less than USLOSS_MIN_STACK
        console("Error: stackSize [%d] was less than minimum stack address[%d].\n", stackSize, USLOSS_MIN_STACK);
        halt(1);
    }
    //No Errors
    return;

}



/* ------------------------------------------------------------------------
 * TODO:
Name -          addProcessLL
Purpose -       Status List to add process to
Parameters -    proc_slot
Returns -       0 Success, -1 Failure
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
    else if(pStat == Current->activeChildren.childList){
        Current->activeChildren.total++;
    }
    else if(pStat == Current->quitChildren.childList){
        Current->quitChildren.total++;
    }
    return 0;
}


/* ------------------------------------------------------------------------
 * TODO:
Name -          removeProcessLL
Purpose -       Status List to remove process from
Parameters -    proc_slot
Returns -       0 Success, halt(1) on Failure
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
                if(pStat[index].pHeadProc != NULL) {           //if NOT only one node on list
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


/* ------------------------------------------------------------------------
Name -          StatusQueueIsFull
Purpose -       Checks if status queue pStat is Full
Parameters -    pStat: Address of status queue
Returns -       0 if not full, 1 if full
Side Effects -  none
----------------------------------------------------------------------- */
bool StatusQueueIsFull(const StatusQueue * pStat){
    //if readyList
    if (pStat == ReadyList){    //compare memory locations to verify list type
        return totalReadyProc >= MAXPROC;
    }
    else if(pStat == BlockedList){
        return totalBlockedProc >= MAXPROC;
    }
    else if(pStat == Current->activeChildren.childList){
        return Current->activeChildren.total >= MAXPROC;
    }
    else if(pStat == Current->quitChildren.childList){
    return Current->quitChildren.total >= MAXPROC;
    }
}


/* ------------------------------------------------------------------------
Name -          StatusQueueIsEmpty
Purpose -       Checks if status queue pStat is empty
Parameters -    pStat: Address of status queue
                index: index of pStat to check
Returns -       0 if not empty, 1 if empty
Side Effects -  none
----------------------------------------------------------------------- */
bool StatusQueueIsEmpty(const StatusQueue * pStat, int index){
    return pStat[index].pHeadProc == NULL;

    //if ready
    //check ready
    //if blocked
    //check blocked

}


/* ------------------------------------------------------------------------
Name -          CopyToQueue
Purpose -       Copies process at Proctable[proc_slot] to pStat
Parameters -    proc_slot: location of process in ProcTable
                pStat: Address of status queue
Returns -       none
Side Effects -  none
----------------------------------------------------------------------- */
static void CopyToQueue(int proc_slot, StatusStruct * pStat){
    pStat->process = &ProcTable[proc_slot];
}