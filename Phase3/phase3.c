/* ------------------------------------------------------------------------
    phase3.c
    Applied Technology
    College of Applied Science and Technology
    The University of Arizona
    CSCV 452

    Kiera Conway
    Katelyn Griffith

------------------------------------------------------------------------ */

#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <sems.h>
#include <libuser.h>
#include <usyscall.h>
#include "phase3_helper.h"

/** ------------------------------ Prototypes ------------------------------ **/

int start2(char *); 
extern int start3();

/*** Functions Brought in From Phase 1 ***/
void check_kernel_mode(const char *functionName);
void DebugConsole3(char *format, ...);

/*** Others ***/
void nullsys3(sysargs *args);
void setUserMode();
static int spawn_launch(char *arg);
void syscall_Handler(sysargs *args);

/*** Provided Kernel Mode Functions **/
int kerSpawn(char *name,
               int (*func)(char *),
               char *arg,
               int stack_size,
               int priority);
int kerWait(int *status);
void kerTerminate(int exit_code);
int kerSemCreate(int init_value);
int kerSemP(int semaphore);
int kerSemV(int semaphore);
int kerSemFree(int semaphore);
//void kerGetTimeOfDay(int *time);
//void kerCPUTime(int *time);
//void kerGetPID(int *pid);

/*** Needed for sys_vec ***/
static void sysSpawn(sysargs *args);
static void sysWait(sysargs *args);
static void sysTerminate(sysargs *args);
static void sysSemCreate(sysargs *args);
static void sysSemV(sysargs *args);
static void sysSemP(sysargs *args);
static void sysSemFree(sysargs *args);
static void sysGetTimeOfDay(sysargs *args);
static void sysCPUTime(sysargs *args);
static void sysGetPID(sysargs *args);

/** ------------------------------ Globals ------------------------------ **/

/* Flags */
int debugFlag = 1;

/* General Globals */
int start2Pid = 0;           //start2 PID

/* Process Globals */
usr_proc_struct UsrProcTable[MAXPROC];  //Process Table
int totalProc;           //total Processes
unsigned int nextPID;    //next process id

/* Semaphore Globals */
semaphore_struct SemaphoreTable[MAXSEMS];   //Semaphore Table
int totalSem;            //total Semaphores
unsigned int nextSID;    //next semaphore id

/** ----------------------------- Functions ----------------------------- **/

/* ------------------------------------------------------------------------
  Name -           start2
  Purpose -
  Parameters -     *arg:   default arg passed by fork1
  Returns -        0:      indicates normal quit
  Side Effects -   lots since it initializes the phase3 data structures.
  ----------------------------------------------------------------------- */
int start2(char *arg) {
    /*** Function Initialization ***/
    int		pid;
    int		status;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Initialize Global Tables ***/
    memset(UsrProcTable, -1, sizeof(UsrProcTable));
    memset(SemaphoreTable, -1, sizeof (SemaphoreTable));

    /*** Initialize Globals ***/
    totalProc = 0;
    totalSem = 0;
    start2Pid = getpid();

    /*** Initialize sys_vec ***/
    for (int i = 0; i < MAXSYSCALLS; i++) {
        sys_vec[i] = nullsys3;
    }

    /*** Set sys_vec Calls ***/
    sys_vec[SYS_SPAWN] = sysSpawn;
    sys_vec[SYS_WAIT] = sysWait;
    sys_vec[SYS_TERMINATE] = sysTerminate;
    sys_vec[SYS_SEMCREATE] = sysSemCreate;
    sys_vec[SYS_SEMV] = sysSemV;
    sys_vec[SYS_SEMP] = sysSemP;
    sys_vec[SYS_SEMFREE] = sysSemFree;
    sys_vec[SYS_GETTIMEOFDAY] = sysGetTimeOfDay;
    sys_vec[SYS_CPUTIME] = sysCPUTime;
    sys_vec[SYS_GETPID] = sysGetPID;

    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * Assumes kernel-mode versions of the system calls
     * with lower-case names.  I.e., Spawn is the user-mode function
     * called by the test cases; spawn is the kernel-mode function that
     * is called by the syscall_handler; spawn_real is the function that
     * contains the implementation and is called by spawn.
     *
     * Spawn() is in libuser.c.  It invokes usyscall()
     * The system call handler calls a function named spawn() -- note lower
     * case -- that extracts the arguments from the sysargs pointer, and
     * checks them for possible errors.  This function then calls spawn_real().
     *
     * Here, we only call spawn_real(), since we are already in kernel mode.
     *
     * spawn_real() will create the process by using a call to fork1 to
     * create a process executing the code in spawn_launch().  spawn_real()
     * and spawn_launch() then coordinate the completion of the phase 3
     * process table entries needed for the new process.  spawn_real() will
     * return to the original caller of Spawn, while spawn_launch() will
     * begin executing the function passed to Spawn. spawn_launch() will
     * need to switch to user-mode before allowing user code to execute.
     * spawn_real() will return to spawn(), which will put the return
     * values back into the sysargs pointer, switch to user-mode, and 
     * return to the user code that called Spawn.
     */

    /*** Add to ProcTable ***/
//    AddToProcTable(STATUS_READY,
//                   "start2",
//                   getpid(),
//                   0,
//                   NULL,
//                   0,
//                   5);
//TODO: do we need to at any time to reference the process in the list?


    pid = kerSpawn("start3", start3, NULL, 4*USLOSS_MIN_STACK, 3);
    pid = kerWait(&status);
//    kerTerminate(0);

    return 0;

} /* start2 */


/* ------------------------------------------------------------------------
  Name -           Spawn
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void sysSpawn(sysargs *args) {
    /*** Function Initialization ***/
    int (* func) (char *);
    char arg[MAXARG];
    unsigned int stackSize;
    int priority;
    char name[MAXNAME];
    int kidpid;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Check for Invalid Empty Args ***/
    if (args == NULL) {
        DebugConsole3("Error: Passing NULL for args\n");
        kerTerminate(1);
    }

    /*** Check if Current was Zapped ***/
    if (is_zapped()) {
        DebugConsole3("Process was zapped...\n");
        kerTerminate(1);
    }

    /*** Get Functions Arguments ***/
    func      = args->arg1;

    if (args->arg2) {
        strcpy(arg, args->arg2);
    }
//    else {
//        strcpy(arg, '\0');
//    }
//todo: delete if not needed

    stackSize = (int) args->arg3;
    priority  = (int) args->arg4;
    if (args->arg5) {
        strcpy(name, args->arg5);
    }
//    else {
//        strcpy(arg, '\0');
//    }
//todo: delete if not needed

    /*** Error Check: Invalid Values ***/
    /* Out-of-range Priorities */
    if ((priority > LOWEST_PRIORITY) || (priority < HIGHEST_PRIORITY)) {
        DebugConsole3("%s : Process priority is out of range [1 - 5].\n",
                      __func__);
        kerTerminate(1);
    }

    /* Function was Null */
    if (func == NULL) {
        DebugConsole3("%s : Function was Null.\n",
                      __func__);
        kerTerminate(1);
    }

    /* Name was Null */
    if (name == NULL) {
        DebugConsole3("%s : Name was Null.\n",
                      __func__);
        kerTerminate(1);
    }

    /* Invalid StackSize */
    if (stackSize < USLOSS_MIN_STACK) {
        DebugConsole3("%s : Stack size [%d] is less than minimum stack address [%d].\n",
                      __func__, stackSize, USLOSS_MIN_STACK);
        kerTerminate(1);
    }

    /*** Fork User Process ***/
    kidpid = kerSpawn(name, func, arg, stackSize, priority);

    /*** Pack output ***/
    args->arg1 = (void *) kidpid;

    /*** Check if the illegal values were used ***/
    kidpid == -1 ? (args->arg4 = (void *) -1): (args->arg4 = (void *) 0);

    /*** Check if User Process was Zapped ***/
    if (is_zapped()) {
        DebugConsole3("Process was zapped...\n");
        kerTerminate(1);
    }

    /*** Switch into User Mode ***/
    setUserMode();

} /* sysSpawn */


/* ------------------------------------------------------------------------
   Name -           kerSpawn
   Purpose -        Create a user-level process
   Parameters -     name:       character string containing process’s name
                    func:       address of the function to spawn
                    arg:        parameter passed to spawned function
                    stack_size: stack size (in bytes)
                    priority:   priority
   Returns -        >0: PID of the newly created process
                    -1: Invalid Parameters Given,
                        Process Could not be Created
                     0: Otherwise
   Side Effects -   none
   ----------------------------------------------------------------------- */
int kerSpawn(char *name, int (*func)(char *), char *arg, int stack_size, int priority) {

    //TODO: FUNCTION NOTES
    //
    //If the spawned function returns, it should have the same effect as calling Terminate.
    //Then synchronize with the child using a mailbox
    //result = MboxSend(ProcTable[kid_location].start_mbox,
    //       &my_location, sizeof(int));
    //more to add

    /*** Function Initialization ***/
    int kidPID;
    int curIndex;   // parent's location in process table
    int kidIndex;   // child's location in process table
    int result;
    usr_proc_ptr pKid, pCur;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Find Current Index ***/
    curIndex = GetProcIndex(getpid());

    /*** Create Child Process ***/
    kidPID = fork1(name, spawn_launch, arg, stack_size, priority);

    /* Error Check: Valid PID */
    if(kidPID < 0){
        DebugConsole3("%s : Fork Failure\n", __func__);
        return -1;
    }

    /*** Update Processes ***/
    /* Add Kid to Process Table */
    AddToProcTable(STATUS_READY,
                   name,
                   kidPID,
                   func,
                   arg,
                   stack_size,
                   priority);

    /* Add Kid to Parent Child List */
    kidIndex = GetProcIndex(kidPID);    //find kid index
    pKid = &UsrProcTable[kidIndex];         //create pointer to kid

    if(pKid->parentPID > start2Pid){                //if kid has valid Parent
        pCur = &UsrProcTable[curIndex];
        AddProcessLL(pKid, &pCur->children);
    }

    /*** Create Mailbox ***/
    if(pKid->mboxID == -1){
        pKid->mboxID = MboxCreate(0, sizeof(int));

        /** Error Check: Mailbox Creation Failure **/
        if (pKid->mboxID < 0){
            DebugConsole3("%s : Mailbox creation failure for process %d.\n",
                          __func__, kidPID);
            return -1;
        }
    }

    result = MboxSend(pKid->mboxID, NULL, 0);

    /*** Check Send Result ***/
    if (result == -1) {
        DebugConsole3("%s : Illegal arguments used within MboxSend().\n",
                      __func__);
        return -1;
    }
    else if (result == -3) {
        DebugConsole3("%s : Process was zapped or released while blocked.\n",
                      __func__);
        return -1;
    }

    return kidPID;

} /* kerSpawn */


/* ------------------------------------------------------------------------
  Name -           sysWait
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void sysWait(sysargs *args) {
    /*** Function Initialization ***/
    int status;
    int pid;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    pid = kerWait(&status);

    /*** Check if Current was Zapped ***/
    if (is_zapped()) {
        DebugConsole3("Process was zapped...\n");
        kerTerminate(1);
    }

    /*** Pack output ***/
    args->arg1 = (void *) pid;
    args->arg2 = (void *) status;

    /*** Check if the process didn't have children ***/
    pid == -2 ? (args->arg4 = (void *) -1): (args->arg4 = (void *) 0);

    /*** Switch into User Mode ***/
    setUserMode();

} /* sysWait */


/* ------------------------------------------------------------------------
Name -           kerWait
Purpose -
Parameters -
Returns -
Side Effects -
----------------------------------------------------------------------- */
int kerWait(int *status) {
    /*** Function Initialization ***/
    int index = 0;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    index = getpid() % MAXPROC;
    UsrProcTable[index].status == STATUS_JOIN_BLOCK;

    return join(status);

//    if (strt2 == 0){
//        index = getpid() % MAXPROC;
//        UsrProcTable[index].status == STATUS_JOIN_BLOCK;
//        return join(status);
//    }
//    else {
//        strt2 = 0;
//    }

} /* kerWait */


/* ------------------------------------------------------------------------
  Name -           sysTerminate
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void sysTerminate(sysargs *args) {
    /*** Function Initialization ***/
    int status;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    status = (int) args->arg1;
    kerTerminate(status);

    /*** Switch into User Mode ***/
    setUserMode();

} /* sysTerminate */


/* ------------------------------------------------------------------------
  Name -           kerTerminate
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void kerTerminate(int exit_code) {
    /*** Function Initialization ***/
    int pid;
    int parentID;
    usr_proc_ptr p_ptr, parent_ptr;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Get pid of current running process and zap all children ***/
    p_ptr = &UsrProcTable[getpid() % MAXPROC];

    while (p_ptr->children.total > 0) {
        pid = RemoveProcessLL(p_ptr->children.pHeadProc->pProc->pid, &p_ptr->children);
        zap(pid);
    }

    /*** Remove child process from Parent List ***/
    parentID = p_ptr->parentPID;

    /** Find Parent Index **/
    if (parentID > -1) {
        parent_ptr = &UsrProcTable[parentID % MAXPROC];
        RemoveProcessLL(p_ptr->pid, &parent_ptr->children);
    }

    /** Clear out Child Proc Slot **/
    ProcessInit(p_ptr->pid % MAXPROC, p_ptr->pid);

    quit(exit_code);

} /* kerTerminate */


/* ------------------------------------------------------------------------
  Name -           sysSemCreate
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void sysSemCreate(sysargs *args) {
    /*** Function Initialization ***/
    int handle;
    int semaphore;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    handle = args->arg1;

    semaphore = kerSemCreate(handle);

    /*** Pack output ***/
    args->arg1 = (void *) semaphore;

    /*** Check if initial value is negative or no more semaphores available ***/
    if (semaphore == -1) {
        args->arg4 = -1;
    }
    else {
        args->arg4 = 0;
    }

    /*** Switch into User Mode ***/
    setUserMode();


} /* sysSemCreate */


/* ------------------------------------------------------------------------
  Name -           kerSemCreate
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
int kerSemCreate(int init_value) {
    /*** Function Initialization ***/
    int sid;
    int mbox;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /** Error Check: Invalid init_value ***/
    if (init_value < 0) {
        DebugConsole3("%s: init_value was negative...\n", __func__);
        return -1;
    }

    /*** Error Check: Verify Semaphores are available ***/
    if (totalSem > MAXSEMS) {
        DebugConsole3("%s: No more semaphores are available...\n", __func__);
        return -1;
    }

    /*** Get next Available Semaphore ***/
    for (int i = 0; i < MAXSEMS; i++) {
        if (SemaphoreTable[i].sid == -1) {
            sid = i;
            break;
        }
    }

    totalSem++; // Increment total semaphores
    mbox = MboxCreate(0, sizeof(int)); // get mbox for semaphore

    /*** Add Semaphore to Table ***/
    AddToSemTable(sid, 0, SEM_READY, mbox, 1);

    return sid;

} /* kerSemCreate */


/* ------------------------------------------------------------------------
  Name -           sysSemV [SemV]
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void sysSemV(sysargs *args) {
    /*** Function Initialization ***/
    int handler;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    handler = args->arg1;

    if (SemaphoreTable[handler % MAXSEMS].sid != handler) {
        args->arg4 = (void *) -1;

        /*** Switch into User Mode ***/
        setUserMode();

    return;
    }

    args->arg4 = (void *) kerSemV(handler);

    /*** Switch into User Mode ***/
    setUserMode();

} /* sysSemV */


/* ------------------------------------------------------------------------
  Name -           kerSemV
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
int kerSemV(int semaphore) {
    /*** Function Initialization ***/
    sem_struct_ptr sem;
    usr_proc_ptr proc;
    int pid;
    int headPID;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    sem = &SemaphoreTable[semaphore % MAXSEMS];

    /*** There are Blocked Processes-> unblock head process ***/
    if (sem->blockedProcs.total > 0) {

        /*** Unblock Process from Blocked List ***/
        pid = sem->blockedProcs.pHeadProc->pProc->pid;
        headPID = RemoveProcessLL(pid, &sem->blockedProcs);
        proc = &UsrProcTable[headPID % MAXPROC];

        MboxSend(proc->mboxID, NULL, 0);
    }
    /*** Sem Value == 0-> increment ***/
    else {
        sem->value++; // increment semaphore value
    }

    return 0;

} /* kerSemV */


/* ------------------------------------------------------------------------
  Name -           sysSemP [SemP]
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void sysSemP(sysargs *args) {
    /*** Function Initialization ***/
    int handler;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    handler = args->arg1;

    if (SemaphoreTable[handler % MAXSEMS].sid != handler) {
        args->arg4 = (void *) -1;

        /*** Switch into User Mode ***/
        setUserMode();

        return;
    }

    args->arg4 = (void *) kerSemP(handler);

    /*** Switch into User Mode ***/
    setUserMode();

} /* sysSemP */


/* ------------------------------------------------------------------------
  Name -            kerSemP [semP_real]
  Purpose -         Decrements the semaphore count by one.

  Parameters -      semaphore from SemTable
                    arg1: semaphore handle (SID)
  Returns -         arg4:
                        -1 if semaphore handle is invalid
                         0 otherwise
  Side Effects -    Blocks process if the count is zero.
  ----------------------------------------------------------------------- */
int kerSemP(int semaphore) {
    /*** Function Initialization ***/
    sem_struct_ptr sem;
    usr_proc_ptr  proc;
    int result;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    sem = &SemaphoreTable[semaphore % MAXSEMS];

    /*** Sem Count > 0 -> Decrement Count ***/
    if (sem->value > 0) {
        sem->value--; // decrement semaphore value
    }
    /*** Sem Count <= 0 -> block ***/
    else {
        /*** Add Process to Blocked List ***/
        proc = &UsrProcTable[getpid() % MAXPROC];
        AddProcessLL(proc, &sem->blockedProcs);

        /*** Block ***/
        MboxReceive(proc->mboxID, NULL, 0);

        /*** Check if the Semaphore was Freed while Blocked ***/
        if (sem->status == FREEING) {
            setUserMode();
            Terminate(1);
        }
    }

    return 0;

} /* kerSemP */


/* ------------------------------------------------------------------------
  Name -           SemFree
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void sysSemFree(sysargs *args) {
    int handler;

    handler = args->arg1;

    if (SemaphoreTable[handler % MAXSEMS].sid != handler) {
        args->arg4 = (void *) -1;
    return;
    }

    args->arg4 = (void *) kerSemFree(handler);

    setUserMode();

} /* sysSemFree */


/* ------------------------------------------------------------------------
  Name -           kersSemFree
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
int kerSemFree(int semaphore) {
    /*** Function Initialization ***/
    sem_struct_ptr sem;
    usr_proc_ptr proc;
    int pid;
    int headPID;
    int result;

    sem = &SemaphoreTable[semaphore % MAXSEMS];
    // remove processes from list
    // if any are blocked -> arg4 = 1
    // Terminate processes
    // Clear out semaphore

    // no blocked processes -> just empty semaphore in table
    // blocked processes -> FREEING status -> empty status
    if (sem->blockedProcs.total == 0) {
        SemaphoreInit(semaphore % MAXSEMS,sem->sid);
        totalSem--;
    }
    else {
        sem->status = FREEING;

        while (sem->blockedProcs.total > 0) {
            pid = sem->blockedProcs.pHeadProc->pProc->pid;
            headPID = RemoveProcessLL(pid, &sem->blockedProcs);
            proc = &UsrProcTable[pid % MAXPROC];

            result = MboxSend(proc->mboxID, NULL, 0);
        }

        return 1;
    }

    return 0;

} /* kerSemFree */


/* ------------------------------------------------------------------------
  Name -           sysGetTimeOfDay
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void sysGetTimeOfDay(sysargs *args) {
    /*** Function Initialization ***/
    int time;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Get Time ***/
    time = sys_clock();

    /*** Pack output ***/
    args->arg1 = (void *) time;

    /*** Switch into User Mode ***/
    setUserMode();

} /* sysGetTimeOfDay */


/* ------------------------------------------------------------------------
  Name -           sysCPUTime
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void sysCPUTime(sysargs *args) {
    /*** Function Initialization ***/
    int cpuTime;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Get Time ***/
    cpuTime = readtime();

    /*** Pack output ***/
    args->arg1 = (void *) cpuTime;

    /*** Switch into User Mode ***/
    setUserMode();

} /* sysCPUTime */


/* ------------------------------------------------------------------------
  Name -           sysGetPID
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void sysGetPID(sysargs *args) {
    /*** Function Initialization ***/
    int pid;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Get PID ***/
    pid = getpid();

    /*** Pack output ***/
    args->arg1 = (void *) pid;

    /*** Switch into User Mode ***/
    setUserMode();

} /* sysGetPID */


/***---------------------------------------------- Phase 1 Functions -----------------------------------------------***/

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

    //DebugConsole3("check_kernel_node(): verifying kernel mode for %s\n", functionName);

    /* test if in kernel mode; halt if in user mode */
    psrValue.integer_part = psr_get();

    if (psrValue.bits.cur_mode == 0)
    {
        console("Kernel mode expected, but function called in user mode.\n");
        halt(1);
    }

    //DebugConsole3("Function is in Kernel mode (:\n");

} /* check_kernel_mode */


/* ------------------------------------------------------------------------
   Name -           DebugConsole3
   Purpose -        Prints the message to the console if in debug mode
   Parameters -     *format:    Format String to Display
                    ...:        Optional Corresponding Arguments
   Returns -        none
   Side Effects -   none
   ----------------------------------------------------------------------- */
void DebugConsole3(char *format, ...)
{
    if (DEBUG3 && debugFlag)
    {
        va_list argptr;
        va_start(argptr, format);
        vfprintf(stdout, format, argptr);
        fflush(stdout);
        va_end(argptr);
    }

} /* DebugConsole2 */


/***----------------------------------------------- Other Functions ------------------------------------------------***/

/* ------------------------------------------------------------------------
  Name -           nullsys3
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void nullsys3(sysargs *args) {
    console("nullsys(): Invalid syscall %d. Halting...\n", args->number);
    kerTerminate(1);

} /* nullsys3 */


/* ------------------------------------------------------------------------
  Name -           setUserMode
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void setUserMode() {
    psr_set(psr_get() & ~PSR_CURRENT_MODE);

} /* setUserMode */


/* ------------------------------------------------------------------------
  Name -           spawn_launch
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static int spawn_launch(char *arg) {
    /*** Function Initialization ***/
    int my_location;
    int result;
    int (* start_func) (char *);
    usr_proc_ptr pProc;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Create Current Pointer ***/
    my_location = getpid() % MAXPROC;
    pProc = &UsrProcTable[my_location];

    /*** Error Check: Process Not Creation Not Completed ***/
    if (UsrProcTable[my_location].pid != getpid()) {
        UsrProcTable[my_location].mboxID = MboxCreate(0, sizeof(int));
    }

    /*** Sync with Parent Process ***/
    MboxReceive(pProc->mboxID, 0, 0);

    UsrProcTable[my_location].status = STATUS_READY; //todo is this status right?

    // get start functions and argument
    if (!is_zapped()) {
        setUserMode();

        if (*arg == -1 || *arg == 0) {
            result = pProc->startFunc(NULL);
        }
        else {
            result = pProc->startFunc(arg); //todo: why not pProc->startArg ?
        }

        Terminate(result);
    }
    else {
        kerTerminate(0);
    }

    /*** Error Check: Termination Failure ***/
    DebugConsole3("%s : Termination Failure.\n", __func__);

    return 0;

} /* spawn_launch */


/* ------------------------------------------------------------------------
  Name -           syscall_Handler
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void syscall_Handler(sysargs *args) {

} /* syscall_Handler */