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
int spawn_real(char *name,
               int (*func)(char *),
               char *arg,
               int stack_size,
               int priority);
int wait_real(int *status);
extern int start3();

/*** Functions Brought in From Phase 1 ***/
void check_kernel_mode(const char *functionName);
void DebugConsole3(char *format, ...);

/*** Others ***/
void nullsys3(sysargs *args);

/*** Needed for sys_vec ***/
static void sysSpawn();
void sysWait();
static int spawn_launch(char *arg);
void sysTerminate();
void sysSemCreate();
void sysSemV();
void sysSemP();
void sysSemFree();
void sysGetPID();
void sysGetTimeOfDay();
void sysCPUTime();

/** ------------------------------ Globals ------------------------------ **/

/* Flags */
extern int debugFlag;

/* Process Globals */
extern usr_proc_struct UsrProcTable[MAXPROC];  //Process Table
extern int totalProc;           //total Processes
extern unsigned int nextPID;    //next process id

/** ----------------------------- Functions ----------------------------- **/

/* ------------------------------------------------------------------------
  Name -           start2
  Purpose -
  Parameters -     *arg:   default arg passed by fork1, not used here.
  Returns -        0:      indicates normal quit
  Side Effects -   lots since it initializes the phase3 data structures.
  ----------------------------------------------------------------------- */
int start2(char *arg) {
    int		pid;
    int		status;

    /*** Function Initialization ***/
    check_kernel_mode(__func__); // Check for kernel mode

    /*** Initialize Global Tables ***/
    memset(UsrProcTable, 0, sizeof(UsrProcTable));
    // semaphore table

    /*** Initialize sys_vec ***/
    for (int i = 0; i < MAXSYSCALLS; i++) {
        sys_vec[i] = nullsys3;
    }

    // Constants from usyscall.h from usloss include file
    sys_vec[SYS_SPAWN] = sysSpawn;
    sys_vec[SYS_WAIT] = sysWait;
    sys_vec[SYS_TERMINATE] = sysTerminate;
    sys_vec[SYS_SEMCREATE] = sysSemCreate;
    sys_vec[SYS_SEMV] = sysSemV;
    sys_vec[SYS_SEMP] = sysSemP;
    sys_vec[SYS_SEMFREE] = sysSemFree;
    sys_vec[SYS_GETPID] = sysGetPID;
    sys_vec[SYS_GETTIMEOFDAY] = sysGetTimeOfDay;
    sys_vec[SYS_CPUTIME] = sysCPUTime;

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

    pid = spawn_real("start3", start3, NULL, 4*USLOSS_MIN_STACK, 3);
    pid = wait_real(&status);

    return 0; // we should not get here

} /* start2 */


/* ------------------------------------------------------------------------
  Name -           Spawn
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void sysSpawn(sysargs *args) {
    int (* func) (char *);
    char arg[MAXARG];
    unsigned int stackSize;
    int priority;
    char name[MAXNAME];

    /*** Check if Current was Zapped ***/
    if (is_zapped()) {
        // terminate process
    }

    /*** Get Functions Arguments ***/
    func      = args->arg1;
    strcpy(arg, args->arg2);
    stackSize = args->arg3;
    priority  = args->arg4;
    strcpy(name, args->arg5);

    /*** Value Checks ***/
    // Out-of-range priorities
    if ((priority > LOWEST_PRIORITY) || (priority < HIGHEST_PRIORITY)) {
        console("Process priority is out of range. Must be 1 - 5...\n");
        // terminate process
    }

    // func is NULL
    if (func == NULL) {
        console("func was null...\n");
       //terminate process
    }

    // name is NULL
    if (name == NULL) {
        console("name was null...\n");
        //terminate process
    }

    // Stacksize is less than USLOSS_MIN_STACK
    if (stackSize < USLOSS_MIN_STACK) {
        console("stackSize was less than minimum stack address...\n");
        //terminate process
    }

    /*** Fork User Process ***/
    int kidpid = spawn_real(name, func, arg, stackSize, priority);
    args->arg1 = (void *) kidpid; // packing to return back to the caller
    args->arg4 = (void *) 0;

    /*** Check if User Process was Zapped ***/
    if (is_zapped()) {
        // terminate process
    }

    /*** Switch into User Mode ***/
    psr_set(psr_get() & ~PSR_CURRENT_MODE);

    return;

} /* sysSpawn */

/* ------------------------------------------------------------------------
   Name -           spawn_real
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

int spawn_real(char *name, int (*func)(char *), char *arg, int stack_size, int priority) {
    int kidpid;
    int my_location;    /* parent's location in process table */
    int kid_location;   /* child's location in process table */
    int result;
    //u_proc_ptr kidptr, prevptr; // todo replace with our process table struct

    my_location = getpid() % MAXPROC;

    /*** Create Process ***/
    kidpid = fork1(name, spawn_launch, NULL, stack_size, priority);

    /* Error Check: Valid PID */
    if(kidpid < 0){
        DebugConsole3("%s : Fork Failure\n", __func__);
        return -1;
    }


    /*** Change to User-mode ***/
    //todo

    /*** Add to Process Table ***/

    // If the spawned function returns, it should have the same effect as calling Terminate.




    //Then synchronize with the child using a mailbox
//    result = MboxSend(ProcTable[kid_location].start_mbox,
//                      &my_location, sizeof(int));
    //more to add

    return kidpid;

} /* spawn_real */


static int spawn_launch(char *arg) {
    int parent_location = 0;
    int my_location;
    int result;
    int (* start_func) (char *);
    // more stuffs?

    my_location = getpid() % MAXPROC;

    /*** Sanity Check ***/
    // am I sane? nope.

    UsrProcTable[my_location].status = ITEM_IN_USE;

    //sync with parent
    //mboxreceive prob?

    // get start functions and argument
    if (!is_zapped()) {
        psr_set(psr_get() & ~PSR_CURRENT_MODE); // this apparently switches to user mode
        result = (start_func)(arg);
        Terminate(result);
    }
    else {
        terminate_real(0);
    }

    console("ayyo you shouldn't be seeing this! BAD, BAD CODE");

    return 0;

} /* spawn_launch */


/* ------------------------------------------------------------------------
  Name -           Wait
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void sysWait() {
    return;

} /* sysWait */


/* ------------------------------------------------------------------------
Name -           wait_real
Purpose -
Parameters -
Returns -
Side Effects -
----------------------------------------------------------------------- */
int wait_real(int *status) {
    return 0;
} /* wait_real */


/* ------------------------------------------------------------------------
  Name -           Terminate
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void sysTerminate() {
    return;

} /* sysTerminate */


/* ------------------------------------------------------------------------
  Name -           terminate_real
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void terminate_real(int exit_code) {
    return;

} /* terminate_real */


/* ------------------------------------------------------------------------
  Name -           SemCreate
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void sysSemCreate() {
    return;

} /* sysSemCreate */


/* ------------------------------------------------------------------------
  Name -           semcreate_real
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
int semcreate_real(int init_value) {
    return 0;

} /* semcreate_real */


/* ------------------------------------------------------------------------
  Name -           SemV
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void sysSemV() {
    return;

} /* sysSemV */


/* ------------------------------------------------------------------------
  Name -           semp_real
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
int semv_real(int semaphore) {
    return 0;

} /* semp_real */


/* ------------------------------------------------------------------------
  Name -           SemP
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void sysSemP() {
    return;

} /* sysSemP */


/* ------------------------------------------------------------------------
  Name -           semv_real
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
int semp_real(int semaphore) {
    return 0;

} /* semv_real */


/* ------------------------------------------------------------------------
  Name -           SemFree
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void sysSemFree() {
    return;

} /* sysSemFree */


/* ------------------------------------------------------------------------
  Name -           semfree_real
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
int semfree_real(int semaphore) {
    return 0;

} /* semfree_real */


/* ------------------------------------------------------------------------
  Name -           GetTimeOfDay
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void sysGetTimeOfDay() {
    return;

} /* sysGetTimeOfDay */


/* ------------------------------------------------------------------------
  Name -           gettimeofday_real
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
int  gettimeofday_real(int *time) {
    return 0;

} /* gettimeofday_real */


/* ------------------------------------------------------------------------
  Name -           CPUTime
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void sysCPUTime() {
    return;

} /* sysCPUTime */


/* ------------------------------------------------------------------------
  Name -           cputime_real
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
int cputime_real(int *time) {
    return 0;

} /* cputime_real */


/* ------------------------------------------------------------------------
  Name -           GetPID
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void sysGetPID() {
    return;

} /* sysGetPID */


/* ------------------------------------------------------------------------
  Name -           getPID_real
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
int getPID_real(int *pid) {
    return 0;
} /* getPID_real */


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

    DebugConsole3("check_kernel_node(): verifying kernel mode for %s\n", functionName);

    /* test if in kernel mode; halt if in user mode */
    psrValue.integer_part = psr_get();

    if (psrValue.bits.cur_mode == 0)
    {
        console("Kernel mode expected, but function called in user mode.\n");
        halt(1);
    }

    DebugConsole3("Function is in Kernel mode (:\n");

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


/* ------------------------------------------------------------------------
  Name -           nullsys3
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
void nullsys3(sysargs *args) {
    console("nullsys(): Invalid syscall %d. Halting...\n", args->number);
    terminate_real(1);

} /* nullsys3 */