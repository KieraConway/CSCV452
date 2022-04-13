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
#include "usloss/include/phase2.h"

/** ------------------------------ Prototypes ------------------------------ **/

/*** Start Functions ***/
int start2(char *);
extern int start3();

/*** Kernel Mode Functions **/
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

/*** sys_vec Functions ***/
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

/*** Functions Brought in From Phase 1 ***/
void check_kernel_mode(const char *functionName);
void DebugConsole3(char *format, ...);

/*** Others ***/
void nullsys3(sysargs *args);
void setUserMode();
static int spawn_launch(char *arg);

/** ------------------------------ Globals ------------------------------ **/

/* Flags */
int debugFlag = 1;

/* General Globals */
int start2Pid = 0;      //start2 PID

/* Process Globals */
usr_proc_struct UsrProcTable[MAXPROC];      //Process Table
int totalProc;           //total Processes
unsigned int nextPID;    //next process id

/* Semaphore Globals */
semaphore_struct SemaphoreTable[MAXSEMS];   //Semaphore Table
int totalSem;            //total Semaphores
unsigned int nextSID;    //next semaphore id

/** ----------------------------- Functions ----------------------------- **/

/* ------------------------------------------------------------------------
  Name -            start2
  Purpose -         Phase 3 entry point. Initializes UsrProcTable,
                    SemaphoreTable, and sys_vec and creates process
                    mailboxes
  Parameters -      *arg:   default arg passed by fork1
  Returns -         0:      indicates normal quit
  Side Effects -    lots since it initializes the phase3 data structures.
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

    /*** Create Process Mailboxes ***/
    for(int i=0; i< MAXPROC ; i++){
        UsrProcTable[i].mboxID = MboxCreate(1,4);
    }

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

    /*** Create and Run start3 ***/
    pid = kerSpawn("start3", start3, NULL, 4*USLOSS_MIN_STACK, 3);
    pid = kerWait(&status);

    return 0;

} /* start2 */


/* ------------------------------------------------------------------------
  Name -            sysSpawn [spawn]
  Purpose -         Spawns user level process
  Parameters -      systemArgs *args
                        args->arg1: address of the function to spawn
                        args->arg2: parameter passed to spawned function
                        args->arg3: stack size (in bytes)
                        args->arg4: priority
                        args->arg5: process name
  Returns -         void, sets arg values
                    args->arg1:    -1:  process could not be created
                    args->arg4:    -1:  illegal values are given as input
                                    0:  otherwise.
  Side Effects -    calls kerSpawn
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
    /* Get Address of Process to Spawn */
    func = args->arg1;

    /* Get parameters of Process */
    if (args->arg2) {
        strcpy(arg, args->arg2);
    }

    /* Get Stack Size of Process */
    stackSize = (int) args->arg3;

    /* Get Priority of Process */
    priority  = (int) args->arg4;

    /* Get Name of Process */
    if (args->arg5) {
        strcpy(name, args->arg5);
    }

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

    return;

} /* sysSpawn */


/* ------------------------------------------------------------------------
  Name -            kerSpawn    [spawnReal]
  Purpose -         Create a user-level process
                    SemaphoreTable, and sys_vec and creates process
                    mailboxes
  Parameters -      name:       character string containing process’s name
                    func:       address of the function to spawn
                    arg:        parameter passed to spawned function
                    stack_size: stack size (in bytes)
                    priority:   priority
  Returns -         >0: PID of the newly created process
                    -1: Invalid Parameters Given,
                        Process Could not be Created
                     0: Otherwise
  Side Effects -    calls fork1 and updates UsrProcTable and process lists
  ----------------------------------------------------------------------- */
int kerSpawn(char *name, int (*func)(char *), char *arg, int stack_size, int priority) {

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

    /*** Error Check: Valid PID ***/
    if(kidPID < 0){
        DebugConsole3("%s : Fork Failure\n", __func__);
        return -1;
    }

    /*** Update Processes ***/
    /** Add Kid to Process Table **/
    AddToProcTable(STATUS_READY,
                   name,
                   kidPID,
                   func,
                   arg,
                   stack_size,
                   priority);

    /** Add Process to Parent Child List **/
    /* Create Pointer to Child */
    kidIndex = GetProcIndex(kidPID);    //find kid index
    pKid = &UsrProcTable[kidIndex];         //create pointer to kid

    /* Add to Parent's Child List */
    if(pKid->parentPID > start2Pid){                //if kid has valid Parent
        pCur = &UsrProcTable[curIndex];
        AddProcessLL(pKid, &pCur->children);
    }

    /*** Synchronize with Child Process ***/
    result = MboxCondSend(pKid->mboxID, NULL, 0);

    /*** Check Conditional Send Result ***/
    if (result == -1) {
        DebugConsole3("%s : Illegal arguments used within MboxCondSend().\n",
                      __func__);
        return -1;
    }
    else if (result == -3) {
        DebugConsole3("%s : Process was zapped or mailbox was released while the process was blocked.\n",
                      __func__);
        return -1;
    }

    return kidPID;

} /* kerSpawn */


/* ------------------------------------------------------------------------
  Name -            spawn_launch
  Purpose -         Executes process passed to sysSpawn
  Parameters -      arg:    function argument
  Returns -          0: Success
                    -1: Illegal arguments used within MboxSend(),
                        Sent message exceeds max size,
                        Process was zapped,
                        mailbox was released while process was blocked
  Side Effects -    Provides process error checking, synchronization,
                    function execution, and termination
  ----------------------------------------------------------------------- */
static int spawn_launch(char *arg) {
    /*** Function Initialization ***/
    int my_location;
    int result;
    usr_proc_ptr pProc;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Create Current Pointer ***/
    my_location = getpid() % MAXPROC;
    pProc = &UsrProcTable[my_location];

    /*** Error Check: Process Not on ProcTable ***/
    if (pProc->pid != getpid()) {
        int mailboxID = MboxCreate(0, 0);
        UsrProcTable[my_location].mboxID = mailboxID;
        result = MboxReceive(mailboxID, NULL, 0);

        /*** Error Check: MboxReceive Return Value ***/
        if (result == -1) {
            DebugConsole3("%s : Illegal arguments used within MboxReceive() or message sent exceeds max size.\n",
                          __func__);
            return -1;
        }
        else if (result == -3) {
            DebugConsole3("%s : Process was zapped or mailbox was released while the process was blocked.\n",
                          __func__);
            return -1;
        }
    }
    else{
        /*** Synchronize with Parent Process ***/
        result = MboxReceive(pProc->mboxID, 0, 0);

        /*** Error Check: MboxReceive Return Value ***/
        if (result == -1) {
            DebugConsole3("%s : Illegal arguments used within MboxReceive() or message sent exceeds max size.\n",
                          __func__);
            return -1;
        }
        else if (result == -3) {
            DebugConsole3("%s : Process was zapped or mailbox was released while the process was blocked.\n",
                          __func__);
            return -1;
        }
    }

    UsrProcTable[my_location].status = STATUS_READY; // update status

    /*** Get Start Function and Arguments ***/
    if (!is_zapped()) {
        setUserMode();

        if (*arg == -1 || *arg == 0) {
            result = pProc->startFunc(NULL);
        }
        else {
            result = pProc->startFunc(pProc->startArg);
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
  Name -            sysWait  [wait]
  Purpose -         Wait for child process to terminate
  Parameters -      systemArgs *args
  Returns -         void, sets arg values
                    args->arg1:     pid of terminating child
                    args->arg2:     termination status of the child
                    args->arg4: -1: process has no children
                                 0: otherwise.
  Side Effects -    Process terminates if zapped while waiting
                    Calls kerWait
  ----------------------------------------------------------------------- */
static void sysWait(sysargs *args) {
    /*** Function Initialization ***/
    int status;
    int pid;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Call Wait ***/
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

    return;

} /* sysWait */


/* ------------------------------------------------------------------------
  Name -            kerWait [waitReal]
  Purpose -         Called by sysWait, updates UsrProcTable and calls join
  Parameters -      status:     termination status of the child
  Returns -         >0:     PID of quitting child
                    -1:     Process was zapped in Join
		            -2:     Process has no children
  Side Effects -    none, value returned to sysWait
  ----------------------------------------------------------------------- */
int kerWait(int *status) {
    /*** Function Initialization ***/
    int index = 0;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Update Process Status ***/
    index = getpid() % MAXPROC;
    UsrProcTable[index].status == STATUS_JOIN_BLOCK;

    return join(status);

} /* kerWait */


/* ------------------------------------------------------------------------
  Name -            sysTerminate [terminate]
  Purpose -         Terminates invoking process and all of its children
                    and synchronizes with its parent’s Wait system call
  Parameters -      systemArgs *args
                        args->arg1: termination code for the process
  Returns -         none
  Side Effects -    Uses status from args->arg1 to call kerTerminate
  ----------------------------------------------------------------------- */
static void sysTerminate(sysargs *args) {
    /*** Function Initialization ***/
    int status;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Get Status ***/
    status = (int) args->arg1;

    /*** Call Terminate ***/
    kerTerminate(status);

    /*** Switch into User Mode ***/
    setUserMode();

    return;

} /* sysTerminate */


/* ------------------------------------------------------------------------
  Name -            kerTerminate
  Purpose -         Terminates invoking process and all of its children
                    and synchronizes with its parent’s Wait system call
  Parameters -      exit_code:  termination status of the child
  Returns -         none
  Side Effects -    Zaps remaining children, updates parent's child list,
                    initializes UserProcTable slot, and calls quit
  ----------------------------------------------------------------------- */
void kerTerminate(int exit_code) {
    /*** Function Initialization ***/
    int pid;
    int parentID;
    usr_proc_ptr pChild, pCurrent, pParent;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Zap Children ***/
    /* Get Current PID */
    pCurrent = &UsrProcTable[getpid() % MAXPROC];

    /* Check for Remaining Children */
    while (pCurrent->children.total > 0) {

        /* Remove Child From List */
        pid = RemoveProcessLL(pCurrent->children.pHeadProc->pProc->pid,
                              &pCurrent->children);                     //remove child from list
        pChild = &UsrProcTable[pid % MAXPROC];  //create pointer to child
        pChild->parentPID = -1;                 //update child's parent PID

        /* Zap Child */
        zap(pid);
    }

    /*** Remove child process from Parent List ***/
    parentID = pCurrent->parentPID;

    /* Find Parent Index */
    if (parentID > -1) {
        pParent = &UsrProcTable[parentID % MAXPROC];
        RemoveProcessLL(pCurrent->pid, &pParent->children);
    }

    /* Clear out Child Proc Slot */
    ProcessInit(pCurrent->pid % MAXPROC, pCurrent->pid);

    /*** Terminate Process ***/
    quit(exit_code);

    return;

} /* kerTerminate */


/* ------------------------------------------------------------------------
  Name -            sysSemCreate [semCreate]
  Purpose -         Creates a user-level semaphore
  Parameters -      systemArgs *args
                        args->arg1: initial semaphore value
  Returns -         void, sets arg values
                    args->arg1:    sid
                    args->arg4:    -1:  no more semaphores are available
                                    0:  otherwise.
  Side Effects -    calls kerSemCreate
  ----------------------------------------------------------------------- */
static void sysSemCreate(sysargs *args) {
    /*** Function Initialization ***/
    int value;
    int semaphore;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Get Semaphore Value ***/
    value = (int) args->arg1;

    /*** Create Semaphore ***/
    semaphore = kerSemCreate(value);

    /*** Pack output ***/
    args->arg1 = (void *) semaphore;

    /*** Check if initial value is negative or no more semaphores available ***/
    semaphore == -1 ? (args->arg4 = (void *) -1): (args->arg4 = (void *) 0);

    /*** Switch into User Mode ***/
    setUserMode();

    return;

} /* sysSemCreate */


/* ------------------------------------------------------------------------
  Name -            kerSemCreate
  Purpose -         Creates a user-level semaphore
  Parameters -      init_value: initial semaphore value
  Returns -         sid:        semaphore id
  Side Effects -    creates semaphore and updates SemaphoreTable
  ----------------------------------------------------------------------- */
int kerSemCreate(int init_value) {
    /*** Function Initialization ***/
    int sid;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /** Error Check: Invalid init_value ***/
    if (init_value < 0) {
        DebugConsole3("%s: init_value was negative...\n", __func__);
        return -1;
    }

    /*** Error Check: Verify Semaphores are available ***/
    if (totalSem >= MAXSEMS) {
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

    /*** Update Global Values ***/
    totalSem++;

    /*** Add Semaphore to Table ***/
    AddToSemTable(sid, SEM_READY, init_value);

    return sid;

} /* kerSemCreate */


/* ------------------------------------------------------------------------
  Name -            sysSemV [SemV]
  Purpose -         Performs a ”V” operation on a semaphore; Increment
  Parameters -      systemArgs *args
                        args->arg1:     semaphore handle (sid)
  Returns -         void, sets arg values
                    args->arg4:    -1:  semaphore handle is invalid
                                    0:  otherwise
  Side Effects -    Calls kerSemV
  ----------------------------------------------------------------------- */
static void sysSemV(sysargs *args) {
    /*** Function Initialization ***/
    int handler;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Get Semaphore Handler ***/
    handler = (int) args->arg1;

    /*** Error Check: Invalid Handler ***/
    if (SemaphoreTable[handler % MAXSEMS].sid != handler) {

        /*** Pack Output ***/
        args->arg4 = (void *) -1;

        /*** Switch into User Mode ***/
        setUserMode();

        return;
    }

    /*** Pack Output ***/
    args->arg4 = (void *) kerSemV(handler);

    /*** Switch into User Mode ***/
    setUserMode();

    return;

} /* sysSemV */


/* ------------------------------------------------------------------------
  Name -            kerSemV [semV_real]
  Purpose -         Performs a ”V” operation on a semaphore; Increment
  Parameters -      semaphore:  sid
  Returns -          0: Success
                    -1: Illegal arguments used within MboxSend(),
                        Process was zapped,
                        mailbox was released while process was blocked
  Side Effects -    Unblocks blocked processes and increments the
                    semaphore value by one
  ----------------------------------------------------------------------- */
int kerSemV(int semaphore) {
    /*** Function Initialization ***/
    sem_struct_ptr sem;
    usr_proc_ptr proc;
    int pid;
    int result;
    int headPID;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    sem = &SemaphoreTable[semaphore % MAXSEMS]; // get semaphore

    /*** There are Blocked Processes ***/
    if (sem->blockedProcs.total > 0) {

        /*** Unblock Process from Blocked List ***/
        pid = sem->blockedProcs.pHeadProc->pProc->pid;
        headPID = RemoveProcessLL(pid, &sem->blockedProcs);
        proc = &UsrProcTable[headPID % MAXPROC];

        /*** Unblock ***/
        result = MboxSend(proc->mboxID, NULL, 0);

        /*** Check Send Result ***/
        if (result == -1) {
            DebugConsole3("%s : Illegal arguments used within MboxSend().\n",
                          __func__);
            return -1;
        }
        else if (result == -3) {
            DebugConsole3("%s : Process was zapped or mailbox was released while the process was blocked.\n",
                          __func__);
            return -1;
        }
    }
    /*** Sem Value == 0 ***/
    else {
        sem->value++; // increment semaphore value
    }

    return 0;

} /* kerSemV */

/* ------------------------------------------------------------------------
  Name -            sysSemP [SemP]
  Purpose -         Performs a ”P” operation on a semaphore; Decrement
  Parameters -      systemArgs *args
                        args->arg1:     semaphore handle (sid)
  Returns -         void, sets arg values
                    args->arg4:    -1:  semaphore handle is invalid
                                    0:  otherwise
  Side Effects -    Calls kerSemP
  ----------------------------------------------------------------------- */
static void sysSemP(sysargs *args) {
    /*** Function Initialization ***/
    int handler;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Get Semaphore Handler ***/
    handler = (int) args->arg1;

    /*** Invalid Semaphore ***/
    if (SemaphoreTable[handler % MAXSEMS].sid != handler) {
        /*** Pack Output ***/
        args->arg4 = (void *) -1;

        /*** Switch into User Mode ***/
        setUserMode();

        return;
    }

    /*** Pack Output ***/
    args->arg4 = (void *) kerSemP(handler);

    /*** Switch into User Mode ***/
    setUserMode();

    return;

} /* sysSemP */


/* ------------------------------------------------------------------------
  Name -            kerSemP [semP_real]
  Purpose -         Performs a ”P” operation on a semaphore; Decrement
  Parameters -      semaphore:  sid
  Returns -          0: Success
                    -1: Illegal arguments used within MboxSend(),
                        Sent message exceeds max size,
                        Process was zapped,
                        mailbox was released while process was blocked
  Side Effects -    Blocks processes and decrements the semaphore
                    value by one
  ----------------------------------------------------------------------- */
int kerSemP(int semaphore) {
    /*** Function Initialization ***/
    sem_struct_ptr sem;
    usr_proc_ptr  proc;
    int result;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    sem = &SemaphoreTable[semaphore % MAXSEMS]; // get semaphore

    /*** Sem Count > 0 ***/
    if (sem->value > 0) {
        sem->value--; // decrement semaphore value
    }
    /*** Sem Count <= 0 ***/
    else {
        /*** Add Process to Blocked List ***/
        proc = &UsrProcTable[getpid() % MAXPROC];
        AddProcessLL(proc, &sem->blockedProcs);

        /*** Block ***/
        result = MboxReceive(proc->mboxID, NULL, 0);

        /*** Check Receive Result ***/
        if (result == -1) {
            DebugConsole3("%s : Illegal arguments used within MboxReceive() or message sent exceeds max size.\n",
                          __func__);
            return -1;
        }
        else if (result == -3) {
            DebugConsole3("%s : Process was zapped or mailbox was released while the process was blocked.\n",
                          __func__);
            return -1;
        }

        /*** Check if the Semaphore was Freed while Blocked ***/
        if (sem->status == FREEING) {
            setUserMode();
            Terminate(1);
        }
    }

    return 0;

} /* kerSemP */


/* ------------------------------------------------------------------------
  Name -            sysSemFree   [SemFree]
  Purpose -         Frees a semaphore
  Parameters -      systemArgs *args
                        args->arg1:     semaphore handle (sid)
  Returns -         void, sets arg values
                    args->arg4:    -1:  semaphore handle is invalid
                                    1:  processes blocked on semaphore
                                    0:  otherwise
  Side Effects -    calls kerSemFree
  ----------------------------------------------------------------------- */
static void sysSemFree(sysargs *args) {
    /*** Function Initialization ***/
    int handler;

    /*** Get Semaphore Handler ***/
    handler = (int) args->arg1;

    if (SemaphoreTable[handler % MAXSEMS].sid != handler) {
        /*** Pack Output ***/
        args->arg4 = (void *) -1;
        return;
    }

    /*** Pack Output ***/
    args->arg4 = (void *) kerSemFree(handler);

    /*** Switch into User Mode ***/
    setUserMode();

    return;

} /* sysSemFree */


/* ------------------------------------------------------------------------
  Name -            kerSemFree
  Purpose -         Frees a semaphore
  Parameters -      semaphore:  sid
  Returns -          0: Success
                    -1: Illegal arguments used within MboxSend(),
                        Process was zapped,
                        mailbox was released while process was blocked
                     1: processes blocked on semaphore
  Side Effects -    handles semaphore block list
  ----------------------------------------------------------------------- */
int kerSemFree(int semaphore) {
    /*** Function Initialization ***/
    sem_struct_ptr sem;
    usr_proc_ptr proc;
    int pid;
    int result;

    sem = &SemaphoreTable[semaphore % MAXSEMS]; // get semaphore

    /*** No Blocked Processes ***/
    if (sem->blockedProcs.total == 0) {
        SemaphoreInit(semaphore % MAXSEMS,sem->sid);
        totalSem--;
    }
    /*** Blocked Processes Exist ***/
    else {
        sem->status = FREEING; // set status to freeing

        while (sem->blockedProcs.total > 0) {
            pid = sem->blockedProcs.pHeadProc->pProc->pid;
            RemoveProcessLL(pid, &sem->blockedProcs);
            proc = &UsrProcTable[pid % MAXPROC];

            /*** Unblock to Terminate ***/
            result = MboxSend(proc->mboxID, NULL, 0);

            /*** Check Receive Result ***/
            if (result == -1) {
                DebugConsole3("%s : Illegal arguments used within MboxSend().\n",
                              __func__);
                return -1;
            }
            else if (result == -3) {
                DebugConsole3("%s : Process was zapped or mailbox was released while the process was blocked.\n",
                              __func__);
                return -1;
            }
        }

        return 1;
    }

    return 0;

} /* kerSemFree */


/* ------------------------------------------------------------------------
  Name -            sysGetTimeOfDay
  Purpose -         Obtains value of the time-of-day clock
  Parameters -      systemArgs *args
  Returns -         void, sets arg values
                    args->arg1: time of day
  Side Effects -    sets args->arg1 to time of day
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
  Name -            sysCPUTime
  Purpose -         Obtains the CPU time of the process
  Parameters -      systemArgs *args
  Returns -         void, sets arg values
                    args->arg1: the process’ CPU time
  Side Effects -    sets args->arg1 to the process’ CPU time
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
  Name -            sysGetPID
  Purpose -         Obtains PID of the currently running process
  Parameters -      systemArgs *args
  Returns -         void, sets arg values
                    args->arg1: pid
  Side Effects -    sets args->arg1 to pid
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

/* ----------------------------- Imported Phase 1 Functions ----------------------------- */

/* ------------------------------------------------------------------------
   Name -           check_kernel_mode
   Purpose -        Checks the PSR for kernel mode and halts if in user mode
   Parameters -     functionName:   Function to Verify
   Returns -        none
   Side Effects -   Halts if not kernel mode
   ----------------------------------------------------------------------- */
void check_kernel_mode(const char *functionName) {
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
void DebugConsole3(char *format, ...) {
    if (DEBUG3 && debugFlag) {
        va_list argptr;
        va_start(argptr, format);
        vfprintf(stdout, format, argptr);
        fflush(stdout);
        va_end(argptr);
    }

} /* DebugConsole2 */

/* ----------------------------- Miscellaneous Functions ----------------------------- */

/* ------------------------------------------------------------------------
  Name -            nullsys3
  Purpose -         Error method to handle invalid syscalls
  Parameters -      systemArgs *args
  Returns -         none
  Side Effects -    none
  ----------------------------------------------------------------------- */
void nullsys3(sysargs *args) {
    console("nullsys(): Invalid syscall %d. Halting...\n", args->number);
    kerTerminate(1);

} /* nullsys3 */


/* ------------------------------------------------------------------------
  Name -            setUserMode
  Purpose -         Sets mode from kernel to user
  Parameters -      none
  Returns -         none
  Side Effects -    user mode is set
  ----------------------------------------------------------------------- */
void setUserMode() {
    psr_set(psr_get() & ~PSR_CURRENT_MODE);

} /* setUserMode */

/** ----------------------------- END of Functions ----------------------------- **/