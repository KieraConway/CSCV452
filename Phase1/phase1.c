/* ------------------------------------------------------------------------
    phase1.c

    CSCV 452

    Katelyn Griffith
    Kiera Conway
   ------------------------------------------------------------------------ */
/* * * * * * * * * * * * * * * * * * * * *
 * Author Notes
 *  NEXT:
 *      (1) DumpProcesses
 *      (2) error messages to include
 *      function name
 *      "quit(): process 2 quit with active children. Halting..."
 *      (3)add error check from 689 to rest of code
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
int VerifyProcess(char *name, int priority, int *func, int stackSize);
void AddProcessLL(int proc_slot, StatusQueue * pStat);
int removeProcessLL(int pid, int priority, StatusQueue * pStat);
bool StatusQueueIsFull(const StatusQueue * pStat);
bool StatusQueueIsEmpty(const StatusQueue * pStat, int index);
static void CopyToQueue(int proc_slot, StatusStruct * pStat);
int block_me(int new_status);
int unblock_proc(int pid);
int Read_Cur_Start_Time(void);
void Time_Slice(void);
int ReadTime(void);
bool ProcessInList(int pid, StatusQueue * pStat);

/* -------------------------- Globals ------------------------------------- */

/* Patrick's debugging global variable... */
int debugflag = 0;

/* the process table */
proc_struct ProcTable[MAXPROC];

/* current process ID */
proc_ptr Current;

/* the next pid to be assigned */
unsigned int next_pid = SENTINELPID;

/* number of active processes */
unsigned int numProc = 0; 	// kg   //TODO: can we delete this?

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
    totalProc = 0;          //Total Processes
    totalReadyProc = 0;     //Total Ready Processes
    totalBlockedProc = 0;   //Total Blocked Processes
    InitializeLists();

    /* Initialize the clock interrupt handler */
    int_vec[CLOCK_INT] = &ClockIntHandler; // added in kg

    /* startup a sentinel process */
    result = fork1("sentinel", sentinel, NULL, USLOSS_MIN_STACK, SENTINELPRIORITY); // call fork1
    if (result < 0)
    {
        DebugConsole("startup(): fork1 of sentinel returned error. Halting...\n");
        halt(1);
    }

    init = 0; // added in kg

    /* start the test process */
    DebugConsole("startup(): calling fork1() for start1\n");

    result = fork1("start1", start1, NULL, 2 * USLOSS_MIN_STACK, 1);

    if (result < 0) {
        console("startup(): fork1 for start1 returned an error. Halting...\n");
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

    int proc_slot;
    int newPid; // added in kg

    DebugConsole("fork1(): creating process %s\n", name);

    /*** Error Check: Test if in kernel mode, halt if in user mode ***/
    check_kernel_mode(__func__);

    /*** Error Check: Verify process parameters are valid ***/
    int flag;
    if ((flag = VerifyProcess(name, priority, func, stackSize)) != 0){
        return flag;
    }


    /*** Update Process Table with new Process ***/
    if((newPid = GetNextPid()) == -1){              //get next process ID

        //Error Check: Exceeding Max Processes
        DebugConsole("Error: Attempting to exceed MAXPROC limit [%d].\n", MAXPROC);
        return -1;
    }
    proc_slot = newPid % MAXPROC;                   //assign slot
    ProcTable[proc_slot].pid = newPid;              //assign pid
    ProcTable[proc_slot].priority = priority;       //assign priority
    ProcTable[proc_slot].status = STATUS_READY;     //assign READY status
    ProcTable[proc_slot].stackSize = stackSize;     //assign stackSize
    ProcTable[proc_slot].stack = malloc(stackSize);//assign stack


    // Error Check: Verify malloc return value
    if (ProcTable[proc_slot].stack == NULL) {
        DebugConsole("fork1() Error: Out of memory. Halting...\n");
        halt(1);
    }

    // Error Check:  Check if name length is too large
    if (strlen(name) >= (MAXNAME - 1)) {
        DebugConsole("fork1() Error: Process name is too long. Halting...\n");
        halt(1);
    }

    //Add Process Name
    strcpy(ProcTable[proc_slot].name, name);

    //Add Process Function
    ProcTable[proc_slot].start_func = func;

    //Add Process Function Argument(s)
    if (arg == NULL)
        ProcTable[proc_slot].start_arg[0] = '\0';
    else if (strlen(arg) >= (MAXARG - 1)) {
        console("fork1() Error: Argument too long. Halting...\n");
        halt(1);
    } else
        strcpy(ProcTable[proc_slot].start_arg, arg);

    //Add the parent and child values if currently running process
    if (Current != NULL) {
        ProcTable[proc_slot].parent_proc_ptr = Current;

        //Add child to end of doubly linked child list
        AddProcessLL(proc_slot, &Current->activeChildren.procList);
    }
    else{   //if no current process, add NULL for parent
        ProcTable[proc_slot].parent_proc_ptr = NULL;
    }
    ///end Process Table update


    /*** Initialization ***/
    /*
     * Initialize context for this process, but use launch function pointer for
     * the initial value of the process's program counter (PC)
     */
    context_init(&(ProcTable[proc_slot].state), psr_get(),
                 ProcTable[proc_slot].stack,
                 ProcTable[proc_slot].stackSize, launch);

    /* for future phase(s) */
    p1_fork(ProcTable[proc_slot].pid);
    ///end initialization


    /*** Assign to Ready List and Increment counters ***/
    AddProcessLL(proc_slot, &ReadyList);
    totalProc++;


    /*** Call Dispatcher ***/
    if (!init)      //Prevents dispatcher from running without start1
    {
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

    DebugConsole("Process %d returned to launch\n", getpid());

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
    int childPriority;          //used for add/remove list
    int child_proc_slot;        //used for memset
    int curr_proc_slot;         //used for memset
    int childPID;               //return pid of quitting child
    int savedQuitTime;          //holds the lowest/oldest process quit time
    StatusStruct *pQuitChild = NULL;   //save QuitChild process
    StatusStruct *pChild;       //used to iterate quitChildren list
    StatusStruct *pSaveChild;   //used to iterate quitChildren list

    /*** Ensure kernel mode ***/
    check_kernel_mode(__func__);


    /*** Error Check: Process has no children ***/
    if(Current->activeChildren.total == 0 && Current->quitChildren.total == 0){
        DebugConsole("Error: %s has no children", Current->name);
        return (-2);    //return -2: process has no children
    }

    /*** Check: Process child quit before join() ***/
    if(Current->quitChildren.total > 0) {

        /* Iterate through quitChildren to find oldest quit child*/
        for (int index = 0; index < LOWEST_PRIORITY; index++) {   //For each index in array
            int init = 0;       //initialize Flag
            pChild = pSaveChild = Current->quitChildren.procList[index].pHeadProc;   //Set to pHead of index

            while(pChild){
                if(pChild->process->quitTime < pSaveChild->process->quitTime ) {
                    pSaveChild = pChild;
                }

                pChild = pChild->pNextProc; //increment pChild
            }

            // if pQuit is NULL or newer than pChild
            if (pSaveChild && (!pQuitChild || pQuitChild->process->quitTime > pSaveChild->process->quitTime)) {
                pQuitChild = pSaveChild;    //overwrite with pChild
            }
        }


        /*** Complete Child Cleanup ***/
        childPriority = pQuitChild->process->priority;
        child_proc_slot = (pQuitChild->process->pid) % MAXPROC; //find proc_slot

        *code = pQuitChild->process->returnCode;        //save quit status of pQuitChild
        childPID = pQuitChild->process->pid;            //save pid of quitting child

        removeProcessLL(childPID, childPriority, Current->quitChildren.procList);

        totalProc--;                            // decrement total proc

        free(pQuitChild->process->stack);   //free malloc
        memset(&ProcTable[child_proc_slot], 0, sizeof(proc_struct));  //reinitialize ProcTable

        return childPID;
    }

    /*** Error Check: No unjoined child has quit(), wait ***/
    if(Current->quitChildren.total == 0){
        Current->status = STATUS_JOIN_BLOCK;
        dispatcher();
    }

    /*** Get Return Code from new quitChild ***/
    /* Iterate through quitChildren to find oldest quit child*/
    for (int index = 0; index < LOWEST_PRIORITY; index++) {   //For each index in array

        if(Current->quitChildren.procList[index].pHeadProc){  //if found quit process
            pQuitChild = Current->quitChildren.procList[index].pHeadProc;  //set equal to quitChild
            break;
        }

    }


    /*** Complete Child Cleanup ***/
    childPriority = pQuitChild->process->priority;
    child_proc_slot = (pQuitChild->process->pid) % MAXPROC; //find proc_slot

    *code = pQuitChild->process->returnCode;        //save quit status of pQuitChild
    childPID = pQuitChild->process->pid;            //save pid of quitting child

    removeProcessLL(childPID, childPriority, Current->quitChildren.procList);

    free(pQuitChild->process->stack);   //free malloc
    memset(&ProcTable[child_proc_slot], 0, sizeof(proc_struct));  //reinitialize ProcTable

    /*** Error Check: Process was zapped in join ***/
    if(is_zapped()){
        return -1;
    }

    return childPID;

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
    StatusStruct *pActiveChild = NULL;  //used to iterate procList
    StatusStruct *pQuitChild = NULL;    //used to iterate procList
    StatusStruct *pZapper = NULL;       //used to iterate zapList
    StatusStruct *pNextZap = NULL;         //used to iterate zapList
    proc_ptr pParent;       //used to modify parent information

    /*** Error Check: Test if in kernel mode, halt if in user mode ***/
    check_kernel_mode(__func__);

    disableInterrupts();                        //prevent race conditions

    /*** Iterate through procList to check for active children ***/
    for (int index = 0; index < LOWEST_PRIORITY; index++) {   //For each index in array
        pActiveChild = Current->activeChildren.procList[index].pHeadProc; //Set to pHead of index

        while(pActiveChild && pActiveChild->process != NULL){                     //verify not end of procList
            if (pActiveChild->process->status == STATUS_READY){   //status redundancy check
                console("Error: Process with active children attempting to quit. Halting...\n");
                halt(1);
            }

            pActiveChild = pActiveChild->pNextProc; //iterate to next child on list
        }
    } //here if no active children

    /*** Iterate through procList to remove quit children ***/
    for (int index = 0; index < LOWEST_PRIORITY; index++) {   //For each index in array
        pQuitChild = Current->quitChildren.procList[index].pHeadProc;   //Set to pHead of index

        while(pQuitChild && pQuitChild->process != NULL){                        //verify not end of procList

            //remove from Current quit Children list
            removeProcessLL(pQuitChild->process->pid, pQuitChild->process->priority, Current->quitChildren.procList);

            pQuitChild = pQuitChild->pNextProc; //iterate to next zapper on list
        }
    } //here if no active children


    /*** Iterate through procList to check for zappers and unblock ALL ***/
    for (int index = 0; index < LOWEST_PRIORITY; index++) {   //For each index in array
        pZapper = pNextZap = Current->zappers.procList[index].pHeadProc; //Set to pHead of index

        while(pZapper && pZapper->process != NULL){             //verify not end of procList
            pNextZap = pZapper->pNextProc;

            if (pZapper->process->status == STATUS_ZAP_BLOCK){  //status redundancy check
                pZapper->process->status = STATUS_READY;        //unblock zapper

                //remove zap pointer
                pZapper->process->zapped = NULL;


                //add zapper to ready list
                int zap_proc_slot = (pZapper->process->pid) % MAXPROC;  //find proc_slot
                AddProcessLL(zap_proc_slot, &ReadyList);

                //remove from Current zappers list
                removeProcessLL(pZapper->process->pid, pZapper->process->priority, Current->zappers.procList);
            }
            else if (pZapper->process->status == STATUS_ZAPPED){  //if zapper has been zapped

                //Notice that status is NOT changed to STATUS_READY

                //remove zap pointer
                pZapper->process->zapped = NULL;

                //add zapper to ready list
                int zap_proc_slot = (pZapper->process->pid) % MAXPROC;  //find proc_slot
                AddProcessLL(zap_proc_slot, &ReadyList);

                //remove from Current zappers list
                removeProcessLL(pZapper->process->pid, pZapper->process->priority, Current->zappers.procList);
            }
            pZapper = pNextZap;     //iterate to next zapper on list
        }
    }  // all zapper unblocked


    /*** Check if Current has a Parent ***/
    if(Current->parent_proc_ptr != NULL){

        //save parent to pParent
        pParent = Current->parent_proc_ptr;


        //add to quitChildren list
        int curr_proc_slot = (Current->pid) % MAXPROC;     //find current proc_slot
        AddProcessLL(curr_proc_slot, &pParent->quitChildren.procList);

        //remove from activeChildren list
        removeProcessLL(Current->pid, Current->priority, pParent->activeChildren.procList);


        //Check if parent is blocked, unblock and add to ready table
        if(pParent->status == STATUS_JOIN_BLOCK || pParent->status == STATUS_ZAP_BLOCK){
            pParent->status = STATUS_READY;                    //update status
            int par_proc_slot = (pParent->pid) % MAXPROC;       //find parent proc_slot
            AddProcessLL(par_proc_slot, &ReadyList);    //add to ready list
        }

        //Check if parent is zapped while waiting for child to quit
        /*
         * when a function goes from blocked to zapped,
         * it needs to be re-added to the ready list,
         * without changing the status
         */
        if(pParent->status == STATUS_ZAPPED && !(ProcessInList(pParent->pid, &ReadyList))){
            int par_proc_slot = (pParent->pid) % MAXPROC;       //find parent proc_slot
            AddProcessLL(par_proc_slot, &ReadyList);    //add to ready list
        }

    }

    /* Update Current Information and Counters */
    Current->status = STATUS_QUIT;      // update status
    Current->quitTime = sys_clock();    // update quit time
    Current->returnCode = code;         // save return code to pass
    // Parent will do rest of cleanup in join()

    totalProc--;

    p1_quit(Current->pid);

    dispatcher();
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
    disableInterrupts();                        //prevent race conditions

    int testTime = sys_clock();

    prevProc = Current;     //save current process

    if (Current != NULL) {   //Verify currently running process exists
        Current->totalCpuTime += ReadTime(); //calculate total cpu time //TODO 215
        curPid = getpid();                    //set current pid
        proc_slot = (curPid) % MAXPROC;     //find current proc_slot


        /* verify time slice is not exceeded */
        if (Current->status == STATUS_RUNNING) {  //if current status is running (not blocked)
            if ((currentTime - ((Current->switchTime) / 1000)) > TIME_SLICE) {  //if exceeded

                AddProcessLL(proc_slot, &ReadyList);
                Current->status = STATUS_READY; //switch Current status to ready
            }
        }
    }
    nextProc = GetNextReadyProc(); //assign newProcess; added in kg

    //If there is no ready process found, exit program with error
    if(nextProc == NULL){
        console("Error: There are no ready processes. Halting...\n");
        halt(1);
    }

    if (nextProc != Current){       //if nextProc priority is different from Current ...

        if(Current != NULL && Current->status == STATUS_RUNNING){
            AddProcessLL(proc_slot, &ReadyList);
            Current->status = STATUS_READY; //switch Current status to ready
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
        //Current->totalCpuTime += ReadTime();     //calculate total cpu time      //TODO 215 REMOVE? add flag?
        Current->switchTime = currentTime;       //reset switch time
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
    int zapped_proc_slot = pid % MAXPROC;           //find zapped_proc_slot
    int cur_proc_slot = Current->pid % MAXPROC;     //find current proc_slot
    proc_ptr zappedProc;

    /* test if in kernel mode; halt if in user mode */
    check_kernel_mode(__func__);

    /*** Error Check: Process tried to Zap non-existent process ***/
    if(ProcTable[zapped_proc_slot].pid == pid){     //if pid of zappedProc matches
        zappedProc = &ProcTable[zapped_proc_slot];  //Create pointer to zapped process
    }
    else{
        console("Error: Process %d tried to Zap non-existent process [%d]. Halting...\n", getpid(), pid);
        halt(1);
    }

    /*** Error Check: Process tried to Zap itself ***/
    if(getpid() == pid) {   //if current pid is same as parameter pid
        console("Error: Process tried to Zap itself. Halting...\n");
        halt(1);
    }

    /*** Error Check: Zapped Process has called quit ***/
    if(zappedProc->status == STATUS_QUIT){
        console("Error: Zapped Process has called quit.\n");
        return 0;
    }

    /*** Update Zapped Process Details ***/


    //Change zapped process status
    ProcTable[zapped_proc_slot].status = STATUS_ZAPPED;

    //Configure zap pointer and zapper list
    Current->zapped = zappedProc;
    AddProcessLL(cur_proc_slot, zappedProc->zappers.procList);  //add current as zapper
    zappedProc->zappers.total++;

    //block current
    Current->status = STATUS_ZAP_BLOCK;

    //call dispatcher
    dispatcher();

    /*** Error Check: calling process itself was zapped while in zap ***/
    if(is_zapped()){
        console("Error: Calling process itself was zapped while in zap.\n");
        return -1;
    }

    return 0;   //zapped process has quit successfully
}

/* ------------------------------------------------------------------------
 * TODO: Debug to check if works - check non existent?
Name -          is_zapped
Purpose -
Parameters -    Checks if a process status is zapped
Returns -
Side Effects -
   ----------------------------------------------------------------------- */
int is_zapped(void) {
    return (Current->zappers.total > 0);
}

/* ------------------------------------------------------------------------
 * TODO: check if code works
Name -          getpid
Purpose -       To find and return the pid of Current
Parameters -    None
Returns -       Returns the pid of Current
Side Effects -
   ----------------------------------------------------------------------- */
int getpid(void) {
    return Current->pid;
}


/* ------------------------------------------------------------------------
 * TODO: Fix formatting
Name -          dump_processes
Purpose -
Parameters -    None
Returns -
Side Effects -
   ----------------------------------------------------------------------- */
void dump_processes(void)
{
    /*** Error Check: Test if in kernel mode, halt if in user mode ***/
    check_kernel_mode(__func__);

    // array so status will print word instead of value
    char* stats[8] = {"Empty", "RUNNING", "READY",
                      "QUIT", "ZAP_BLOCK", "JOIN_BLOCK",
                      "ZAPPED", "LAST"};

    /*** Print Header ***/
    console("\n\n%-10s %-10s %-10s %-10s %-10s %-10s %-10s\n",
            "PID", "Parent", "Priority", "Status", "# Kids", "CPUtime", "Name");
    console("--------------------------------------"
            "----------------------------------------\n");  // print separator

    /*** Dump Process Table ***/
    for (int i = 0; i < MAXPROC; i++) {
        proc_struct temp = ProcTable[i];

        // if no process in ProcTable[i]
        if (temp.status == STATUS_EMPTY){
            console("%-10s %-10s %-10s %-10s %-10s %-10s %-10s\n",
                    "-1", "-1", "-1", stats[temp.status], "0", "-1", "");
        }

            //if process does not have a parent
        else if(temp.parent_proc_ptr == NULL){
            console("%-10d %-10s %-10d %-10s %-10d %-10d %-10s\n",
                    temp.pid, "-1", temp.priority,
                    stats[temp.status], temp.activeChildren.total,
                    temp.totalCpuTime, temp.name);
        }

            //if process exists and has a parent
        else{
            console("%-10d %-10d %-10d %-10s %-10d %-10d %-10s\n",
                    temp.pid, temp.parent_proc_ptr->pid,
                    temp.priority, stats[temp.status],
                    temp.activeChildren.total, temp.totalCpuTime,
                    temp.name);
        }


        /*
        console("%d ", temp.pid);

        //if process does not have a parent
        if (!temp.parent_proc_ptr) {
            console("%s ", "-1");    //print -1
        } else {
            console("%d ", temp.parent_proc_ptr->pid);  //print parent pid
        }

        console("%d ", temp.priority);
        console("%s ", stats[temp.status]);
        console("%d ", temp.activeChildren.total);
        console("%d ", temp.totalCpuTime);
        console("%s \n", temp.name);
         */
    }
}

/* ------------------------------------------------------------------------
 * TODO
Name -          check_deadlock
Purpose -       check to determine if deadlock has occurred
Parameters -    None
Returns -       None
Side Effects -
   ----------------------------------------------------------------------- */
static void check_deadlock() {
    int readyCount = 0;
    StatusStruct *pSave;   //save previous, enables iteration

    //TODO: do we need to check blocked proc also? to just ready/active?

    /* Iterate through ReadyList */
    for (int index = 0; index < LOWEST_PRIORITY; index++) {   //For each index in array

        pSave = ReadyList[index].pHeadProc;    //Set pSave = to pHead of that index

        //todo: may need a if (pSave != NULL) before while loop

        while (pSave && pSave->process != NULL) {    //Verify not end of list
            readyCount += 1;
            pSave = pSave->pNextProc;   //iterate to next process on list
        }
    }

    /* Error Check - totalReadyProc and readyCount should be equal*/
    if (readyCount != totalReadyProc) {
        DebugConsole("Error: totalReadyProc [%d] and readyCount [%d] are not equal. Halting...\n", totalReadyProc, readyCount);
        halt(1);
    }

    if (readyCount == 0) {        //if ReadyList is empty
        if (totalProc > 1) {      //if there are any processes other than sentinel on the ProcTable
            console("check_deadlock Error: %d processes still active. Halting...\n", totalProc);
            halt(1);
        } else {
            console("Success: All processes completed.\n");
            halt(0);
        }
    }
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
        console("Kernel Error: Not in kernel mode, may not disable interrupts. Halting...\n");
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
        if (Current) {
            int cur_proc_slot = Current->pid % MAXPROC;
            console("%s: called while in user mode, by process %d. Halting...\n", functionName, cur_proc_slot);
        }
        console("%s: called while in user mode, by startup(). Halting...\n", functionName);
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

    if (totalProc < MAXPROC)
    {
        while ((totalProc < MAXPROC) && (ProcTable[procSlot].status != STATUS_EMPTY))
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
Name -          verifyProcess
Purpose -       Verify process input is correct
Parameters -
Returns -       0   Success
                -1  Invalid input
                -2  Invalid stackSize
Side Effects -
----------------------------------------------------------------------- */
int VerifyProcess(char *name, int priority, int *func, int stackSize){
    int flag;

    // Error Check Process Input
    if (totalProc > MAXPROC){   // Return if size is too small or proc table is full
        console("Error: Process Table is Full...\n");
        return flag = -1;
    }
    else if ((priority > LOWEST_PRIORITY) || (priority < HIGHEST_PRIORITY)){    //Out-of-range priorities
        console("Error: Process priority [%d] is out of range [1 - 5].\n", priority);
        return flag = -1;
    }
    else if (func == NULL){                     //Function is NULL
        console("Error: Function was NULL.\n");
        return flag = -1;
    }
    else if (name && !name[0]){                 //Name is NULL
        console("Error: Name was NULL.\n");
        return flag = -1;
    }
    else if(stackSize < USLOSS_MIN_STACK){     // Stacksize is less than USLOSS_MIN_STACK
        console("Error: stackSize [%d] was less than minimum stack address [%d].\n", stackSize, USLOSS_MIN_STACK);
        return flag = -2;
    }
    //No Errors
    return 0;

}



/* ------------------------------------------------------------------------
 * TODO:
Name -          AddProcessLL
Purpose -       Status List to add process to
Parameters -    proc_slot
Returns -       none, halt(1) on failure
Side Effects -
----------------------------------------------------------------------- */
void AddProcessLL(int proc_slot, StatusQueue * pStat){

    StatusStruct * pNew;    //new Node
    StatusStruct * pSave;   //save previous location in loop

    //check if queue is full
    if(StatusQueueIsFull(pStat)){
        console("Error: Queue is full.\n");
        halt(1);    //memory allocation failed
    }

    pNew = (StatusStruct *) malloc (sizeof (StatusStruct));

    //verify allocation
    if (pNew == NULL){
        console("Error: Failed to allocate memory for node in linked list. Halting...\n"); //Error Flag kg
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
        pNew->pPrevProc = pStat[index].pTailProc;         //assign pPrev to current tail
    }

    pStat[index].pTailProc = pNew;                        //reassign pTail to new Tail

    /*** Increment Counter ***/
    if (pStat == ReadyList){    //compare memory locations to verify list type
        totalReadyProc++;       //increment total ready processes
    }
    else if(pStat == BlockedList){ //compare memory locations to verify list type
        totalBlockedProc++;        //increment total blocked processes
    }
    else if(pStat == Current->activeChildren.procList){ //compare memory locations to verify list type
        Current->activeChildren.total++;                //increment total activeChildren processes
    }
    else if(pStat == Current->quitChildren.procList){   //compare memory locations to verify list type
        Current->quitChildren.total++;                  //increment total quitChildren processes
    }
    else if(pStat == Current->zappers.procList){        //compare memory locations to verify list type
        Current->zappers.total++;                       //increment total zappers processes
    }
    else if(pStat == Current->parent_proc_ptr->quitChildren.procList){
        Current->parent_proc_ptr->quitChildren.total++;
    }
    else if(pStat == Current->parent_proc_ptr->activeChildren.procList){
        Current->parent_proc_ptr->activeChildren.total++;
    }
    else if(pStat == Current->parent_proc_ptr->zappers.procList){
        Current->parent_proc_ptr->zappers.total++;
    }
}


/* ------------------------------------------------------------------------
 * TODO: Should we have it return a pointer to removed function? It will help w/ zappers
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

    if(pSave == NULL){  //Check that pSave returned process
        console("Error: Process %d is not on list. Halting...\n", pid );
        halt(1);
    }

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
        pSave = NULL;

        /*** Decrement Counter ***/
        if (pStat == ReadyList){    //compare memory locations to verify list type
            totalReadyProc--;       //decrement total ready processes
        }
        else if(pStat == BlockedList){ //compare memory locations to verify list type
            totalBlockedProc--;        //decrement total blocked processes
        }
        else if(pStat == Current->activeChildren.procList){ //compare memory locations to verify list type
            Current->activeChildren.total--;                //decrement total activeChildren processes
        }
        else if(pStat == Current->quitChildren.procList){   //compare memory locations to verify list type
            Current->quitChildren.total--;                  //decrement total quitChildren processes
        }
        else if(pStat == Current->zappers.procList){        //compare memory locations to verify list type
            Current->zappers.total--;                       //decrement total zappers processes
        }
        else if(pStat == Current->parent_proc_ptr->quitChildren.procList){
            Current->parent_proc_ptr->quitChildren.total--;
        }
        else if(pStat == Current->parent_proc_ptr->activeChildren.procList){
            Current->parent_proc_ptr->activeChildren.total--;
        }
        else if(pStat == Current->parent_proc_ptr->zappers.procList){
            Current->parent_proc_ptr->zappers.total--;
        }

        return 0;
    }
    else {
        if (pidFlag == 0){
            console("Error: Unable to locate process pid [%d]. Halting...\n", pid);
            halt(1);
        }
        else if (remFlag == 0) {
            console("Error: Removal of process [%d] unsuccessful. Halting...\n", remFlag);
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

        //if blocked list
    else if(pStat == BlockedList){
        return totalBlockedProc >= MAXPROC;
    }

        //if active children
    else if(pStat == Current->activeChildren.procList){
        return Current->activeChildren.total >= MAXPROC;
    }

        //if quit children
    else if(pStat == Current->quitChildren.procList){
        return Current->quitChildren.total >= MAXPROC;
    }

        //if zappers
    else if(pStat == Current->zappers.procList){
        return Current->zappers.total >= MAXPROC;
    }

        //if parent quit children
    else if(pStat == Current->parent_proc_ptr->quitChildren.procList){
        return Current->parent_proc_ptr->quitChildren.total >= MAXPROC;
    }

        //if parent active children
    else if(pStat == Current->parent_proc_ptr->activeChildren.procList){
        return Current->parent_proc_ptr->activeChildren.total >= MAXPROC;
    }

        //if parent zappers
    else if(pStat == Current->parent_proc_ptr->zappers.procList){
        return Current->parent_proc_ptr->zappers.total >= MAXPROC;
    }

        //if process being zapped
    else if(pStat == Current->zapped->zappers.procList){
        return Current->zapped->zappers.total >= MAXPROC;
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

/* ------------------------------------------------------------------------
 * TODO: Phase 2
Name -          block_me
Purpose -       Blocks the calling process
Parameters -    new_status: indicates the status of the process in the
                            dump processes command
Returns -        0: success
                -1: process was zapped while blocked
Side Effects -  blocks calling process
----------------------------------------------------------------------- */
int block_me(int new_status){

    /*** Error Check: new_status is not larger than 10 ***/
    if(new_status <= 10){
        console("block_me Error: new_status [%d] "
                "is not greater than 10. Halting...\n", new_status);
        halt(1);
    }

    /*** Error Check: Process was zapped before block ***/
    if(is_zapped()){
        return -1;
    }

    /*** Block the Calling Process ***/
    /* Update Process Status */
    Current->status = new_status;

    dispatcher();

    /*** Error Check: Process was zapped during block ***/
    if(is_zapped()){
        return -1;
    }

    return 0;

}

/* ------------------------------------------------------------------------
 * TODO: Phase 2
Name -          unblock_proc
Purpose -       Unblocks process with pid that had previously been
                    blocked by calling block_me()
Parameters -    pid:    previously blocked process from block_me
Returns -        0: Success
                -1: Calling process was zapped
                -2: If calling process: was not blocked,
                                        does not exist,
                                        is current process,
                                        is blocked on a status <= 10
Side Effects - changes status of process with specified PID
----------------------------------------------------------------------- */
int unblock_proc(int pid){
    int unblock_proc_slot = pid % MAXPROC;                  //find unblock_proc_slot
    proc_ptr unblockProc = &ProcTable[unblock_proc_slot];   //points to process to unblock

    /*** Error Check: Calling process was zapped ***/
    if(Current->zappers.total > 0){
        console("unblock_proc(): Calling process [%d] was zapped\n", getpid());
        return -1;
    }

    /*** Error Check: Called process was not blocked ***/
    if(unblockProc->status == STATUS_JOIN_BLOCK || unblockProc->status == STATUS_ZAP_BLOCK ){
        console("unblock_proc(): Called process [%d] was not blocked\n", unblockProc->pid);
        return -2;
    }

    /*** Error Check: Called process does not exist ***/
    if(unblockProc->status == STATUS_EMPTY){
        console("unblock_proc(): Called process [%d] does not exist\n", getpid());
        return -2;
    }

    /*** Error Check: Called process is current process ***/
    if(pid == getpid()){
        console("unblock_proc(): Called process [%d] does not exist\n", getpid());
        return -2;
    }

    /*** Error Check: Called process is blocked on a status <= 10 ***/
    if(unblockProc <= 10){
        console("unblock_proc(): Called process [%d] is blocked on a status <= 10 \n", getpid());
        return -2;
    }

    /*** Unblock Process with Specified PID ***/
    unblockProc->status = STATUS_READY;                            //update status
    AddProcessLL(unblock_proc_slot, &ReadyList);    //add to ReadyList

    dispatcher();

    return 0;

}

/* ------------------------------------------------------------------------
 * TODO:
Name -          read_cur_start_time
Purpose -       returns the time (in microseconds) at which the currently
                    executing process began its current time slice
Parameters -    none
Returns -
Side Effects -  none
----------------------------------------------------------------------- */
int Read_Cur_Start_Time(void){

}

/* ------------------------------------------------------------------------
 * TODO:
Name -          time_slice
Purpose -       calls the dispatcher if the currently executing process
                    has exceeded its time slice
Parameters -    none
Returns -       none
Side Effects -  none
----------------------------------------------------------------------- */
void Time_Slice(void){

}

/* ------------------------------------------------------------------------
Name -          ReadTime
Purpose -       returns the CPU time (in milliseconds) used by the
                    current process
Parameters -    none
Returns -       none
Side Effects -  none
----------------------------------------------------------------------- */
int ReadTime(void){
    int currentTime = sys_clock();
    return currentTime - Current->switchTime;
}

/* ------------------------------------------------------------------------
Name -          ProcessInList
Purpose -       iterates through list pStat to find process using pid
Parameters -      pid:  pid of process to find
                pStat:  list to search
Returns -       0:  not found
                1:  found
Side Effects -  none
----------------------------------------------------------------------- */
bool ProcessInList(int pid, StatusQueue * pStat){
    int proc_slot = pid % MAXPROC;                  //find zapped_proc_slot
    proc_ptr Process = &ProcTable[proc_slot];       //pointer to zapped process
    StatusStruct * pSave;                           //used to iterate through ReadyList

    int index = (Process->priority)-1;   //index in list
    pSave = pStat[index].pHeadProc;     //Find correct index, Start at head

    if(pSave == NULL){  //if pSave is NULL, process not found
        return 0;
    }

    //iterate through list
    while(pSave->process != NULL){          //verify not end of list

        if(pSave->process->pid == pid){     //if PIDs match,
            return 1;                       //process found
        }
        else{
            pSave = pSave->pNextProc;       //iterate to next on list
        }
    }
    return 0;   //not found
}