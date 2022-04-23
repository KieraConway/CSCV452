/* ------------------------------------------------------------------------
    phase4.c
    Applied Technology
    College of Applied Science and Technology
    The University of Arizona
    CSCV 452

    Kiera Conway
    Katelyn Griffith

    TODO:

        -  initialize SleepList in start
        -  test list functions


------------------------------------------------------------------------ */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase4.h>
#include <usyscall.h>
#include <provided_prototypes.h>
#include "driver.h"
#include "process.h"
#include "lists.h"
#include "phase4_helper.h"

/* Added for Clion Functionality */
#include "usloss/include/phase1.h"
#include "usloss/include/phase2.h"
#include "usloss/include/usyscall.h"
#include "usloss/include/provided_prototypes.h"

/** ------------------------------ Prototypes ------------------------------ **/

/*** Start Functions ***/
int start3(char *);
extern int start4(char *arg);

/*** Driver Functions ***/
static int ClockDriver(char *);
static int DiskDriver(char *);

/*** Kernel Mode Functions **/
static int sleep(int seconds);
static void diskRead(int unit,
                     int track,
                     int first,
                     int sectors,
                     void *buffer);
static void diskWrite(int unit,
                      int track,
                      int first,
                      int sectors,
                      void *buffer);
static void diskSize(int unit,
                     int *sector,
                     int *track,
                     int *disk);

/*** Functions Brought in From Phase 1 ***/
static void check_kernel_mode(const char *functionName);
static void DebugConsole4(char *format, ...);

/*** Others ***/
static void setUserMode();
static void setKernelMode();
static void sysCall(sysargs *args);
int SecToMSec(int seconds);

/** ------------------------------ Globals ------------------------------ **/

/*** Flags ***/
int debugFlag = 0;

static int runningSID; /*semaphore to synchronize drivers and start3*/

/*** Disk Globals ***/
static struct driver_proc DriverTable[MAXPROC];
static int diskPIDs[DISK_UNITS];                    // disk processes

/*** Process Globals ***/
p4proc_struct ProcTable[MAXPROC];    //Process Table
procQueue SleepingProcs;             //Sleeping Processes List

/** ----------------------------- Functions ----------------------------- **/

/* In the kernel, a process could be zapped by another process, indicating that it should quit as soon as
    possible. The device drivers in the support level must follow this convention; if they are zapped,
    they should clean up their state and call quit. */

/* ------------------------------------------------------------------------
  Name -            start3
  Purpose -         Phase 4 entry point.
  Parameters -      *arg:   default arg passed by fork1
  Returns -         0:      indicates normal quit
  Side Effects -    lots since it initializes the phase4 data structures.
  ----------------------------------------------------------------------- */
int start3(char *arg)
{
    /*** Function Initialization ***/
    char termBuff[10];
    char name[128];
    char buf[10];
    int	clockPID;
    int	status;
    int	pid;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Assign System Call Handlers ***/
    sys_vec[SYS_SLEEP]     = sysCall;
    sys_vec[SYS_DISKREAD]  = sysCall;
    sys_vec[SYS_DISKWRITE] = sysCall;
    sys_vec[SYS_DISKSIZE]  = sysCall;

    /*** Initialize Global Tables ***/
    memset(ProcTable, 0, sizeof(ProcTable));
    memset(SleepProcTable, 0, sizeof (SleepProcTable));

    /*** Create Process Mailboxes ***/
    for(int i=0; i< MAXPROC ; i++){
        ProcTable[i].mboxID = MboxCreate(1,4);
    }

    /*** Create Clock Device Driver ***/
    /* Fork Clock Driver */
    runningSID = semcreate_real(0);
    clockPID = fork1("Clock Driver",
                     ClockDriver,
                     NULL,
                     USLOSS_MIN_STACK,
                     2);

    /* Error Check: Valid Clock Driver Creation */
    if (clockPID < 0) {
        console("%s : Can't create clock driver. Halting ...\n",
                      __func__);
        halt(1);
    }

    /* Add to ProcTable */
    AddToProcTable(clockPID);

    /*
     * Wait for the clock driver to start. The idea is that ClockDriver
     * will V the semaphore "runningSID" once it is running.
     */

    semp_real(runningSID);

    /*
     * Create the disk device drivers here.  You may need to increase
     * the stack size depending on the complexity of your
     * driver, and perhaps do something with the pid returned.
     */

    for (int i = 0; i < DISK_UNITS; i++) {
        sprintf(buf, "%d", i);
        sprintf(name, "DiskDriver%d", i);

        diskPIDs[i] = fork1(name, DiskDriver, buf, USLOSS_MIN_STACK, 2);

        if (diskPIDs[i] < 0) {
            console("start3(): Can't create disk driver %d\n", i);
            halt(1);
        }
    }

    /*** Wait for all disk drivers to start ***/
    for (int i = 0; i < DISK_UNITS; i++) {
        semp_real(runningSID);
    }

    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * I'm assuming kernel-mode versions of the system calls
     * with lower-case names.
     */
    pid = spawn_real("start4", start4, NULL,  8 * USLOSS_MIN_STACK, 3);
    pid = wait_real(&status);

    /*
     * Zap the device drivers
     */

    zap(clockPID);  // clock driver
    join(&status); /* for the Clock Driver */

    return 0;

} /* start3 */


/* ------------------------------------------------------------------------
  Name -            ClockDriver
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static int ClockDriver(char *arg) {

    /* The clock device driver is a process that is responsible for implementing a general delay facility.
    This allows a process to sleep for a specified period of time, after which the clock driver process will
    make it runnable again. The clock driver process keeps track of time using waitdevice. Since the
    kernel controls low-level scheduling decisions, all the clock driver can ensure is that a sleeping
    process is not made runnable until the specified time has expired */

    int result;
    int status;

    /*
     * Let the parent know we are running and enable interrupts.
     */
    semv_real(runningSID);

    /*** Set Kernel Mode ***/
    setKernelMode();

    while(!is_zapped()) {
        result = waitdevice(CLOCK_DEV, 0, &status);

        if (result != 0) {
            return 0;
        }
        /*
         * Compute the current time and wake up any processes
         * whose time has come.
         */



    }

} /* ClockDriver */


/* ------------------------------------------------------------------------
  Name -            DiskDriver
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static int DiskDriver(char *arg) {

    /* The disk driver is responsible for reading and writing sectors to the disk. Your implementation
    needs to queue several requests from user processes simultaneously, rather than simply handling
    one request at a time. (Note that you may choose to implement any disk scheduling algorithm in
    your implementation to improve disk utilization. Or just simply queue the requests). */

    int unit = atoi(arg);
    device_request my_request;
    int result;
    int status;

    driver_proc_ptr current_req;

    DebugConsole4("DiskDriver(%d): started\n", unit);

    /* Get the number of tracks for this disk */
    my_request.opr  = DISK_TRACKS;
    //todo: add in-> my_request.reg1 = &num_tracks[unit];

    /*
    * Let the parent know we are running and enable interrupts.
    */
    semv_real(runningSID);

    /*** Set Kernel Mode ***/
    setKernelMode();

    while(!is_zapped()) {
        result = waitdevice(CLOCK_DEV, 0, &status);

        if (result != 0) {
            return 0;
        }
        /*
         * Compute the current time and wake up any processes
         * whose time has come.
         */
    }

    result = device_output(DISK_DEV, unit, &my_request);

    if (result != DEV_OK) {
        console("DiskDriver %d: did not get DEV_OK on DISK_TRACKS call\n", unit);
        console("DiskDriver %d: is the file disk%d present???\n", unit, unit);
        halt(1);
    }

    waitdevice(DISK_DEV, unit, &status);

    //DebugConsole4("DiskDriver(%d): tracks = %d\n", unit, num_tracks[unit]);

    //semv_real(runningSID);

    return 0;

} /* DiskDriver */


/* ------------------------------------------------------------------------
  Name -            sleep [kerSleep]
  Purpose -         Delays the calling process for int seconds
  Parameters -      seconds:    number of seconds to delay the process
  Returns -         -1: illegal values passed
                     0: Success
  Side Effects -
  ----------------------------------------------------------------------- */
static int sleep(int seconds) {
    /*** Function Initialization ***/
    int result = -1;

    /*** Add Process to ProcTable ***/
    AddToProcTable(getpid());

    /*** Error Check: Check for valid seconds ***/
    if (seconds > 0) {

        /* Change to Milliseconds */
        int mSec = SecToMSec(seconds);

        /* Add to Sleeping List */
        proc_ptr pCur = CreatePointer(getpid());
        AddProcessLL(pCur, &SleepingProcs); //todo: test functionality

        /* Block Process */
        MboxReceive(pCur->mboxID, NULL, 0);

        result = 0;
    }

    /*** Error Check: If Process was zapped ***/
    //todo

    return result;

} /* sleep */


/* ------------------------------------------------------------------------
  Name -            kerDiskRead
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void diskRead(int unit,
                     int track,
                     int first,
                     int sectors,
                     void *buffer) {
    return 0;

} /* diskRead */


/* ------------------------------------------------------------------------
  Name -            kerDiskWrite
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void diskWrite(int unit,
                      int track,
                      int first,
                      int sectors,
                      void *buffer) {
    return 0;

} /* diskWrite */


/* ------------------------------------------------------------------------
  Name -            kerDiskSize
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void diskSize(int unit,
                     int *sector,
                     int *track,
                     int *disk) {
    return 0;

} /* diskSize */


/* ----------------------------- Imported Phase 1 Functions ----------------------------- */

/* ------------------------------------------------------------------------
   Name -           check_kernel_mode
   Purpose -        Checks the PSR for kernel mode and halts if in user mode
   Parameters -     functionName:   Function to Verify
   Returns -        none
   Side Effects -   Halts if not kernel mode
   ----------------------------------------------------------------------- */
static void check_kernel_mode(const char *functionName) {
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
   Name -           DebugConsole4
   Purpose -        Prints the message to the console if in debug mode
   Parameters -     *format:    Format String to Display
                    ...:        Optional Corresponding Arguments
   Returns -        none
   Side Effects -   none
   ----------------------------------------------------------------------- */
static void DebugConsole4(char *format, ...) {
    if (DEBUG4 && debugFlag) {
        va_list argptr;
        va_start(argptr, format);
        vfprintf(stdout, format, argptr);
        fflush(stdout);
        va_end(argptr);
    }

} /* DebugConsole4 */

/* ----------------------------- Miscellaneous Functions ----------------------------- */

/* ------------------------------------------------------------------------
  Name -            setUserMode
  Purpose -         Sets mode from kernel to user
  Parameters -      none
  Returns -         none
  Side Effects -    user mode is set
  ----------------------------------------------------------------------- */
static void setUserMode() {
    psr_set(psr_get() & ~PSR_CURRENT_MODE);

} /* setUserMode */


/* ------------------------------------------------------------------------
  Name -            setKernelMode
  Purpose -         Sets mode from user to kernel
  Parameters -      none
  Returns -         none
  Side Effects -    kernel mode is set
  ----------------------------------------------------------------------- */
static void setKernelMode() {
    psr_set(psr_get() | PSR_CURRENT_INT);

} /* setKernelMode */


/* ------------------------------------------------------------------------
  Name -            sysCall
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static void sysCall(sysargs *args) {
    char *pFunctionName;
    int *values;

    if (args == NULL) {
        console("%s: Invalid syscall %d, no arguments.\n", __func__, args->number);
        console("%s: Process %d, terminating...\n", __func__, getpid());
        terminate_real(1);
    }

    switch (args->number) {
        case SYS_SLEEP:
            args->arg4 = (void *) sleep((__intptr_t) args->arg1);
            break;
        case SYS_DISKREAD:
            break;
        case SYS_DISKWRITE:
            break;
        case SYS_DISKSIZE:
            break;
        default:
            console("Unknown syscall number");
            halt(1);
    }

    if (is_zapped()) {
        console("%s: process %d was zapped while in system call.\n", pFunctionName, getpid());
        terminate_real(0);
    }

    /*** Set User Mode ***/
    setUserMode();

} /* sysCall */

/* ------------------------------------------------------------------------
  Name -            SecToMsec
  Purpose -         Converts seconds to milliseconds
  Parameters -      seconds
  Returns -         milliseconds
  Side Effects -    none
  ----------------------------------------------------------------------- */
int SecToMSec(int seconds) {
    return (seconds * 1000);
}