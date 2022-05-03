/* ------------------------------------------------------------------------
    phase4.c
    Applied Technology
    College of Applied Science and Technology
    The University of Arizona
    CSCV 452

    Kiera Conway
    Katelyn Griffith

------------------------------------------------------------------------ */
/* Standard Libraries */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <stdbool.h>

/* USLOSS and Previous Libraries */
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase4.h>
#include <usyscall.h>
#include <provided_prototypes.h>

/* Project Headers */
#include "process.h"

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
static int diskIO(int operation, 
                     void *buffer,
                     int sectors, 
                     int trackStart, 
                     int sectorStart, 
                     int unit);
// static int diskRead(void *buffer, 
//                      int sectors, 
//                      int trackStart, 
//                      int sectorStart, 
//                      int unit);
// static int diskWrite(void *buffer, 
//                      int sectors, 
//                      int trackStart, 
//                      int sectorStart, 
//                      int unit);
static int diskSize(int unit,
                    int *sectorSize,
                    int *numSectors,
                    int *numTracks);

/*** Table Functions ***/
/* Process Functions */
int GetIndex(int pid);
void AddToProcTable(int pid);
int RemoveFromProcTable(int pid);
proc_ptr CreatePointer(int pid);
// void AddToDriverTable(int pid);
// driver_ptr CreateDriverPointer(int pid);
// int RemoveFromDriverTable(int pid);

/*** List Functions **/
void InitializeList(procQueue *pProc);
bool ListIsFull(const procQueue *pProc);
bool ListIsEmpty(const procQueue *pProc);
int AddToList(proc_ptr pProc, procQueue * pq);
int RemoveFromList(int pid, procQueue * pq);

/*** Functions Brought in From Phase 1 ***/
static void check_kernel_mode(const char *functionName);
void DebugConsole4(char *format, ...);

/*** Others ***/
static void setUserMode();
static void setKernelMode();
static void sysCall(sysargs *args);
static int SecToMSec(int seconds);

/** ------------------------------ Globals ------------------------------ **/

/*** Flags ***/
int debugFlag = 0;
int exitDriver = 0;

/*** Global Counters ***/
int totalProcs = 0;
int totalDrivers = 0;
int totalSleepingProcs = 0;
int totalRequestProcs = 0;
int startMailbox = 0;
int currentTrack = 0;

static int runningSID; /*semaphore to synchronize drivers and start3*/

/*** Disk Globals ***/
static int diskPIDs[DISK_UNITS];          // disk processes for zapping
procQueue requestQueue[DISK_UNITS];       // waiting disk requests
int trackAmount[DISK_UNITS];
int diskSems[DISK_UNITS];

/*** Process Globals ***/
p4proc_struct ProcTable[MAXPROC];    //Process Table
procQueue SleepingProcs;             //Sleeping Processes List
int criticalMboxID;

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
    int msg = 0;

    /*** Check for Kernel Mode ***/
    check_kernel_mode(__func__);

    /*** Assign System Call Handlers ***/
    sys_vec[SYS_SLEEP]     = sysCall;
    sys_vec[SYS_DISKREAD]  = sysCall;
    sys_vec[SYS_DISKWRITE] = sysCall;
    sys_vec[SYS_DISKSIZE]  = sysCall;

    /*** Initialize Global Tables ***/
    memset(ProcTable, 0, sizeof(ProcTable));

    /*** Initialize Global Lists ***/
    InitializeList(&SleepingProcs);
    for(int i = 0; i++; i < DISK_UNITS){
        InitializeList(&requestQueue[i]);
    }

    /*** Create Process Mailboxes ***/
    for (int i=0; i< MAXPROC ; i++) {
        ProcTable[i].mboxID = MboxCreate(0,sizeof(int));
        ProcTable[i].blockSem = semcreate_real(0);
    }
    criticalMboxID = MboxCreate(1,sizeof(int));

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
    //AddToProcTable(clockPID); TODO do we even need to?

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

    proc_ptr pCur = CreatePointer(getpid());

    for (int i = 0; i < DISK_UNITS; i++) {
        sprintf(buf, "%d", i);
        sprintf(name, "DiskDriver%d", i);

        diskSems[i] = semcreate_real(0);
        diskPIDs[i] = fork1(name, DiskDriver, buf, USLOSS_MIN_STACK, 2);

        startMailbox = pCur->mboxID;
        MboxReceive(startMailbox, &msg, sizeof(int));

        if (diskPIDs[i] < 0) {
            console("start3(): Can't create disk driver %d\n", i);
            halt(1);
        }
    }

    // /*** Wait for all disk drivers to start ***/
    // for (int i = 0; i < DISK_UNITS; i++) {
    //     semp_real(runningSID);
    // }

    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * I'm assuming kernel-mode versions of the system calls
     * with lower-case names.
     */

    pid = spawn_real("start4", start4, NULL,  8 * USLOSS_MIN_STACK, 3);
    pid = wait_real(&status);

    DebugConsole4("%s: Function Returned from Start4\n", __func__);

    /*
     * Zap the device drivers
     */
    exitDriver = 1;

    /* Clock Driver */
    zap(clockPID);


    /* Disk Driver */
    for (int i = 0; i < DISK_UNITS; i++){
        semv_real(diskSems[i]);
        zap(diskPIDs[i]);
    }

    DebugConsole4("%s: Quitting\n", __func__);

    join(&status);
    quit(0);
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
    int curTime;
    int headPID;
    int awakerPID;
    int msg = 0;

    /*
     * Let the parent know we are running and enable interrupts.
     */

    semv_real(runningSID);

    /*** Enable Interrupts ***/
    setKernelMode();

    /*** Loop Until Zapped ***/
    while(!is_zapped()) {
        result = waitdevice(CLOCK_DEV, 0, &status);

        if (result != 0) {
            return 0;
        }

        /*
         * Compute the current time and wake up any processes
         * whose time has come.
         */

        curTime = sys_clock();

        /*** Awaken any Processes that need to be Awakened ***/
        while ((SleepingProcs.total > 0) && (SleepingProcs.pHeadProc->pProc->wakeTime < curTime)) {
            headPID = SleepingProcs.pHeadProc->pProc->pid;

            /*** Remove Head Sleep Process from the Sleeping List ***/
            awakerPID = RemoveFromList(headPID, &SleepingProcs);

            /*** Wake up Process ***/
            MboxSend(ProcTable[awakerPID % MAXPROC].mboxID, &msg, sizeof(int));
        }

    }

    /*** The Clock Driver was zapped -> quit ***/
    quit(0);

    return 0;

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
    int result;
    int status;
    int num_tracks;
    int sectorCount;
    int msg = CRIT_EMPTY;    //used for Mbox functions
    device_request my_request;
    proc_ptr current_req;
    proc_ptr pCur = CreatePointer(getpid());

    DebugConsole4("DiskDriver(%d): started\n", unit);

    /* Get the number of tracks for this disk */
    my_request.opr  = DISK_TRACKS;
    my_request.reg1 = &trackAmount[unit];

    /* Block Handling */
    result = device_output(DISK_DEV, unit, &my_request);
    result = waitdevice(DISK_DEV, unit, &status);

    /* Error Check: Invalid Output Result */
    if (result == DEV_INVALID){
        DebugConsole4("%s :Invalid device output result.\n", __func__);
    }

    /*
    * Let the parent know we are running and enable interrupts.
    */
    MboxSend(startMailbox, &msg, sizeof(int));    //unblock Start3

    //semv_real(runningSID);

    /* Initialize Arm Position to 0 */
    result = device_output(DISK_DEV, unit, &my_request);
    result = waitdevice(DISK_DEV, unit, &status);

    /* Error Check: Invalid Output Result */
    if (result == DEV_INVALID){
        DebugConsole4("%s :Invalid device output result.\n", __func__);
    }

    while (!is_zapped()) {

        /* Await Request */
        semp_real(diskSems[unit]);
        //TODO: mailbox if semp not working //MboxReceive(proc->mboxID,  &msg, sizeof(int));

        /* Error Check: Check for Zapping Upon Return */
        if (is_zapped() || exitDriver == 1){
            return 0;
        }

        /* Enter Mutex for Critical Section */
        MboxCondReceive(criticalMboxID, &msg, sizeof(int));
        
        if (msg == CRIT_EMPTY){
            msg = CRIT_FULL;
            MboxSend(criticalMboxID, &msg, sizeof(int));

        }
        else{
            MboxReceive(criticalMboxID, &msg, sizeof(int));
            msg = CRIT_FULL;
            MboxSend(criticalMboxID, &msg, sizeof(int));
        }

        /* Check Queue */
        if (requestQueue[unit].total == 0){
            DebugConsole4("%s : Disk queue is empty.\n", __func__);
        }
        else{

            /*** Grab Next Request ***/
            /* Take Next Request from Queue */
            current_req = requestQueue[unit].pHeadProc->pProc;

            /* Remove Request from Queue */
            RemoveFromList(current_req->pid, &requestQueue[unit]);

            /* Exit Mutex for Critical Section */
            msg = CRIT_EMPTY;
            MboxRelease(criticalMboxID);
            criticalMboxID = MboxCreate(1,sizeof(int));
            MboxSend(criticalMboxID, &msg, sizeof(int));

            /** Inialize Starting Values **/
            current_req->currentTrack = current_req->trackStart;
            current_req->currentSector = current_req->sectorStart;
            current_req->diskOffset = current_req->diskBuf;
            sectorCount = 0;


            do {
                /*** Verify Track ***/
                if(currentTrack != current_req->trackStart){

                    /* Seek to Current Track */
                    my_request.opr =  DISK_SEEK;
                    my_request.reg1 = current_req->currentTrack;

                    result = device_output(DISK_DEV, unit, &my_request);
                    result = waitdevice(DISK_DEV, unit, &status);

                    /* Error Check: Invalid Output Result */
                    if (result == DEV_INVALID){
                        DebugConsole4("%s :Invalid device output result.\n", __func__);
                    }

                    /* Update Global */    
                    currentTrack = current_req->trackStart;
                }

                my_request.opr = current_req->operation;
                my_request.reg1 = (void*) current_req->currentSector;
                my_request.reg2 = current_req->diskOffset;

                result = device_output(DISK_DEV, unit, &my_request);
                result = waitdevice(DISK_DEV, unit, &status);

                if (result != DEV_OK){
                    console("DiskDriver %d: did not get DEV_OK on DISK_TRACKS call\n", unit);
                    console("DiskDriver %d: is the file disk%d present???\n", unit, unit);
                    halt(1);
                }

                if (++(current_req->currentSector) == DISK_TRACK_SIZE){
                    current_req->currentSector = 0;
                    current_req->currentTrack = (current_req->currentTrack + 1) %DISK_TRACK_SIZE;
                    currentTrack = current_req->currentTrack;

                    // my_request.opr =  DISK_SEEK;
                    // my_request.reg1 = current_req->current_track;

                    // result = device_output(DISK_DEV, unit, &my_request);
                    // result = waitdevice(DISK_DEV, unit, &status);

                }
                //else sector++
                current_req->diskOffset += 512;
                
                sectorCount++;

            } while (sectorCount < current_req->numSectors);

            /* Wake - Inform Process that Request is Complete */
            MboxSend(current_req->mboxID, &msg, sizeof(int));
        }

    }


    //DebugConsole4("DiskDriver(%d): tracks = %d\n", unit, num_tracks[unit]);

    //semv_real(runningSID);

    /*** The Disk Driver was zapped => quit ***/
    quit(0);

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
    int mSec;
    proc_ptr pCur;
    int curTime;
    int msg = 0;

    /*** Add Process to ProcTable ***/
    AddToProcTable(getpid());

    /*** Error Check: Check for valid seconds ***/
    if (seconds > 0) {

        /* Change to Microseconds */
        mSec = SecToMSec(seconds);

        /* Get Current Time */
        curTime = sys_clock();

        /* Set Wake Time for Process */
        ProcTable[getpid() % MAXPROC].wakeTime = mSec + curTime;

        /* Add to Sleeping List */
        pCur = CreatePointer(getpid());
        AddToList(pCur, &SleepingProcs); //TODO: test functionality

        /* Block Process */
        MboxReceive(pCur->mboxID, &msg, sizeof(int));

        result = 0;
    }

    /*** Error Check: If Process was zapped ***/
    if (is_zapped()) {
        memset(&ProcTable[getpid() % MAXPROC], 0, sizeof(p4proc_struct));
        quit(1);
    }

    return result;

} /* sleep */




/* ------------------------------------------------------------------------
  Name -            diskIO
  Purpose -         Handles disk read/write
  Parameters -      buffer:         memory address to transfer to
                    operation:      indicates disk operation
                    sectors:        number of sectors to read/write
                    trackStart:     starting disk track number
                    sectorStart:    starting disk sector number
                    unit:           unit number from which to write
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static int diskIO(int operation, void *buffer, int sectors, int trackStart, int sectorStart, int unit){
    /*** Function Initialization ***/
    int msg = CRIT_EMPTY;

    /*** Error Check: Valid Parameters ***/
    /* Verify Valid Track Start */
    if (trackStart < 0 || trackStart > trackAmount[unit]){
        DebugConsole4("%s : Illegal trackStart parameter.\n", __func__);
        return -1;
    }
    /* Verify Valid Sector Start */
    if (sectorStart < 0 || sectorStart  > DISK_TRACK_SIZE){
        DebugConsole4("%s : Illegal sectorStart parameter.\n", __func__);
        return -1;
    }
    /* Verify Unit */
    if (unit < 0 || unit > (DISK_UNITS-1)){
        DebugConsole4("%s : Illegal unit parameter.\n", __func__);
        return -1;
    }

    /*** Add Process to Global ProcTable ***/
    AddToProcTable(getpid());
    proc_ptr pProc = CreatePointer(getpid());

    /* Build the Request */
    pProc->been_zapped = 0;
    pProc->operation = operation;
    pProc->mboxID = pProc->mboxID;
    pProc->diskBuf = buffer;
    pProc->numSectors = sectors;
    pProc->trackStart = trackStart;
    pProc->sectorStart = sectorStart;
    pProc->unit = unit;


    /* Enter Mutex for Critical Section */
    MboxCondReceive(criticalMboxID, &msg, sizeof(int));
    
    if (msg == CRIT_EMPTY){
        msg = CRIT_FULL;
        MboxSend(criticalMboxID, &msg, sizeof(int));

    }
    else{
        MboxReceive(criticalMboxID, &msg, sizeof(int));
        msg = CRIT_FULL;
        MboxSend(criticalMboxID, &msg, sizeof(int));
    }

    /* Add Request to Queue */
    AddToList(pProc, &requestQueue[unit]);  

    /* Exit Mutex for Critical Section */
    msg = CRIT_EMPTY;
    MboxRelease(criticalMboxID);
    criticalMboxID = MboxCreate(1,sizeof(int));
    MboxSend(criticalMboxID, &msg, sizeof(int));

    /*** Handle Blocking ***/
    semv_real(diskSems[unit]);      //wake up disk

    //todo: wait for disk operation to complete
    //TODO: maybe? send line 492 in IO
    MboxReceive(pProc->mboxID, &msg, sizeof(int));

    return 0;

} /* diskRead */


/* ------------------------------------------------------------------------
  Name -            diskSize
  Purpose -
  Parameters -
  Returns -
  Side Effects -
  ----------------------------------------------------------------------- */
static int diskSize(int unit, int *sectorSize, int *numSectors, int *numTracks) {
    
    /* Error Check: Unit is Valid */
    if (unit < 0 || unit > DISK_UNITS ){
        return -1;
    }

    *sectorSize = DISK_SECTOR_SIZE;
    *numSectors = DISK_TRACK_SIZE;
    *numTracks = trackAmount[unit];


    //TODO: sleep start3 until disksize returns
    
    /* Return Results */
    return 0; //0 means success, -1 means error occurs

} /* diskSize */

/* ************************************************************************************ */
/*                                  Process Functions                                   */
/* ************************************************************************************ */

/* ------------------------------------------------------------------------
   Name -           GetIndex
   Purpose -        Gets Process index from id
   Parameters -     pid:    Unique Process ID
   Returns -        Process Index
   Side Effects -   none
   ----------------------------------------------------------------------- */
int GetIndex(int pid){
    return pid % MAXPROC;

} /* GetIndex */


/* ------------------------------------------------------------------------
  Name -            AddToProcTable
  Purpose -         Adds current process to ProcTable()
  Parameters -      pid:    Unique Process ID
  Returns -         none
  Side Effects -    Process added to ProcTable
  ----------------------------------------------------------------------- */
void AddToProcTable(int pid){
    /*** Function Initialization ***/
    int index = GetIndex(pid);
    proc_ptr pProc = &ProcTable[index];

    /*** Verify Entry ***/
    if (pid != pProc->pid){
        /*** Update ProcTable Entry ***/
        pProc->index = index;       //Process Index
        pProc->pid = pid;           //Process ID
        pProc->status = READY;      //Process Status

        totalProcs++;
    }

    return;

} /* AddToProcTable */


/* ------------------------------------------------------------------------
  Name -            RemoveFromProcTable
  Purpose -         Removes current process from ProcTable()
  Parameters -      pid:    Unique Process ID
  Returns -         none
  Side Effects -    Process removed from ProcTable
 ----------------------------------------------------------------------- */
int RemoveFromProcTable(int pid){
    /*** Function Initialization ***/
    int index  = GetIndex(pid);
    proc_ptr pProc = &ProcTable[index];

    /*** Remove Values ***/
    pProc->pid = -1;            //process ID
    pProc->index = -1;          //index Location
    pProc->been_zapped = -1;    //zapping flag
    pProc->mboxID = -1;         //mailbox ID
    pProc->blockSem = -1;       //semaphore ID
    pProc->status = -1;         //status
    pProc->wakeTime = -1;       //time to wake

    pProc->operation = -1;      //indicates disk operation
    pProc->unit = -1;
    pProc->trackStart = -1;     //track starting location
    pProc->currentTrack = -1;   //current track location
    pProc->sectorStart = -1;    //sector starting location   
    pProc->currentSector = -1;  //current sector location
    pProc->numSectors = -1;     //number of sectors
    pProc->diskBuf = -1;        //starting buffer location
    pProc->diskOffset = -1;     //current buffer location (512i)

    pProc->diskRequest.opr = -1;
    pProc->diskRequest.reg1 = -1;
    pProc->diskRequest.reg2 = -1;

    /*** Update Counter ***/
    totalProcs--;

} /* RemoveFromProcTable */

/* ------------------------------------------------------------------------
    Name -          CreatePointer
    Purpose -       Creates a pointer to specified process
    Parameters -    pid:    Process ID
    Returns -       Pointer to found Process
    Side Effects -  None
   ----------------------------------------------------------------------- */
proc_ptr CreatePointer(int pid){
    /*** Create Current Pointer ***/
    int index = pid % MAXPROC;
    return &ProcTable[index];

} /* CreatePointer */

// /* ------------------------------------------------------------------------
//   Name -            AddToDriverTable
//   Purpose -         Adds current process to DriverTable()
//   Parameters -      pid:    Unique Process ID
//   Returns -         none
//   Side Effects -    Process added to DriverTable
//   ----------------------------------------------------------------------- */
// void AddToDriverTable(int pid){
//     /*** Function Initialization ***/
//     int index = pid % MAXPROC;
//     driver_ptr pDriver = &DriverTable[index];

//     /*** Update DriverTable Entry ***/
//     pDriver->wake_time = -1;
//     pDriver->been_zapped = -1;
//     pDriver->operation = -1;
//     pDriver->trackStart = -1;
//     pDriver->sector_start = -1;
//     pDriver->current_sector = -1;
//     pDriver->num_sectors = -1;
//     pDriver->disk_buf = -1;
//     pDriver->disk_offset = -1;

//     totalDrivers++;
// } /* AddToDriverTable */

// /* ------------------------------------------------------------------------
//  * TODO: function
//   Name -            RemoveFromProcTable
//   Purpose -
//   Parameters -

//   Returns -
//   Side Effects -
//  ----------------------------------------------------------------------- */
// int RemoveFromDriverTable(int pid){
//     /*** Function Initialization ***/
//     int index = pid % MAXPROC;
//     driver_ptr pDriver = &DriverTable[index];

//     pDriver->wake_time = -1;
//     pDriver->been_zapped = -1;
//     pDriver->operation = -1;
//     pDriver->track_start = -1;
//     pDriver->sector_start = -1;
//     pDriver->current_sector = -1;
//     pDriver->num_sectors = -1;
//     pDriver->disk_buf = NULL;
//     pDriver->disk_offset = NULL;

//     totalDrivers--;

// } /* RemoveFromDriverTable */

// /* ------------------------------------------------------------------------
//     Name -          CreateDriverPointer
//     Purpose -       Creates a pointer to specified driver
//     Parameters -    did:    Driver ID
//     Returns -       Pointer to found driver
//     Side Effects -  None
//    ----------------------------------------------------------------------- */
// driver_ptr CreateDriverPointer(int pid){
//     /*** Create Current Pointer ***/
//     int index = pid % MAXPROC;
//     return &DriverTable[index];

// } /* CreateDriverPointer */


/* * * * * * * * * * * * * * * * END OF Process Functions * * * * * * * * * * * * * * * */


/* * ********************************************************************************** */
/*                                    List Functions                                    */
/* ************************************************************************************ */

/* ------------------------------------------------------------------------
    Name -          InitializeList
    Purpose -       Initialize List pointed to by parameter
    Parameters -    *pProc: Pointer to Process List
    Returns -       None
    Side Effects -  None
   ----------------------------------------------------------------------- */
void InitializeList(procQueue *pProc) {

    /*** Initialize Proc Queue ***/
    pProc->pHeadProc = NULL;    //Head of Queue
    pProc->pTailProc = NULL;    //Tail of Queue
    pProc->total = 0;           //Total Count


}/* InitializeList */


/* ------------------------------------------------------------------------
    Name -          ListIsFull
    Purpose -       Checks if process list is Full
    Parameters -    *pProc: Pointer to Process List
    Returns -        0: not full
                     1: full
    Side Effects -  none
   ----------------------------------------------------------------------- */
bool ListIsFull(const procQueue *pProc) {

    return pProc->total == MAXPROC;

}/* ListIsFull */


/* ------------------------------------------------------------------------
    Name -          ListIsEmpty
    Purpose -       Checks if process list is empty
    Parameters -    *pProc: Pointer to Process List
    Returns -        0: not Empty
                     1: Empty
    Side Effects -  none
   ----------------------------------------------------------------------- */
bool ListIsEmpty(const procQueue *pProc) {

    return pProc->pHeadProc == NULL;

}/* ListIsEmpty */


/* ------------------------------------------------------------------------
    Name -          AddToList
    Purpose -       Add Process to Specified Process Queue
    Parameters -    pProc:      Pointer to Process
                    pq:         Pointer to Process Queue
    Returns -        0: Success
                    -1: Proc Queue is Full or System ProcTable is Full
    Side Effects -  None
   ----------------------------------------------------------------------- */
int AddToList(proc_ptr pProc,  procQueue * pq) {

    /*** Function Initialization ***/
    bool procAdded = false; //Process Added flag
    ProcList *pList;       //New Node
    ProcList *pSave;       //Save Pointer

    /*** Error Check: Verify Space Available ***/
    if (ListIsFull(pq)) {
        DebugConsole4("%s: Queue is full.\n", __func__);
        return -1;
    }

    /*** Prepare pList and pProc to add to Linked List ***/
    /* Allocate Space for New Node */
    pList = (ProcList *) malloc(sizeof(ProcList));

    /* Verify Allocation Success */
    if (pList == NULL) {
        DebugConsole4("%s: Failed to allocate memory for node in linked list. Halting...\n",
                        __func__);
        halt(1);    //memory allocation failed
    }

    /*** Add pList to ProcQueue and Update Links ***/
    /* Point pList to pProc */
    pList->pProc = pProc;

    /* Add New Node to List */
    pList->pNextProc = NULL;                //assign pNext to NULL

    if (ListIsEmpty(pq)) {                  //if list is empty...
        pq->pHeadProc = pList;              //add pNew to front of list
        pq->pTailProc = pList;              //reassign pTail to new node
        procAdded = true;
    }
    else {                                  //if list not empty...

        /** Iterate Through List **/
        pSave = pq->pHeadProc;          //Set pSave to head of list
        while (pSave != NULL) {   //verify not end of list

            /** Compare Wake Times **/
            /* if pProc wakes later than pSave, iterate */
            if (pProc->wakeTime >= pSave->pProc->wakeTime) {
                pSave = pSave->pNextProc;       //increment to next in list
            }
                /* if pSave wakes later than pProc, insert pProc */
            else {
                /* Adding Proc as New Head */
                if (pSave == pq->pHeadProc) {
                    pq->pHeadProc = pList;
                    pSave->pPrevProc = pList;
                    pList->pNextProc = pSave;
                }
                /* Add Proc Between Two Procs */
                else {
                    pSave->pPrevProc->pNextProc = pList;
                    pList->pPrevProc = pSave->pPrevProc;
                    pList->pNextProc = pSave;
                    pSave->pPrevProc = pList;
                }

                procAdded = true;   //update flag
                break;
            }

            /* Adding Proc as New Tail if pProc has biggest time */
            if (pSave == pq->pTailProc) {
                pq->pTailProc = pList;
                pSave->pNextProc = pList;
                pList->pPrevProc = pSave;

                procAdded = true;
                break;
            }
        }

        if (procAdded == false) {
            pq->pTailProc->pNextProc = pList;   //add pNew to end of list
            pList->pPrevProc = pq->pTailProc;   //assign pNew prev to current tail
            pq->pTailProc = pList;              //reassign pTail to new node
        }
    }

    /*** Update Counters ***/
    /* List Total */
    pq->total++;            //update pq count
    
    /* Global Total */
    if (pq == &SleepingProcs){
        totalSleepingProcs++;
    }
    else if (pq == &requestQueue[0] || pq == &requestQueue[1]){
        totalRequestProcs++;
    }
    else {
        DebugConsole4("%s: Invalid List", __func__);
    }

    /*** Function Termination ***/
    if (procAdded){
        return 0;   //Process Addition Successful
    }
    return 1;       //Process Addition Unsuccessful

}/* AddToList */


/* ------------------------------------------------------------------------
    Name -          RemoveFromList
    Purpose -       Removes Process from Specified Process Queue
    Parameters -    pid:    Process ID
                    pq:     Pointer to Process Queue
    Returns -       Pid of Removed Process, halt (1) on error
    Side Effects -  None
   ----------------------------------------------------------------------- */
int RemoveFromList(int pid, procQueue * pq) {

    /*** Function Initialization ***/
    bool pidFlag = false;   //verify pid found in list
    bool remFlag = false;   //verify process removed successfully

    ProcList *pSave;       //Pointer for Linked List

    /*** Error Check: List is Empty ***/
    if (ListIsEmpty(pq)) {
        DebugConsole4("%s: Queue is empty.\n", __func__);
        halt(1);
    }

    /*** Find PID and Remove from List ***/
    pSave = pq->pHeadProc;              //Set pSave to head of list

    /** PID Found **/
    pidFlag = true;             //Trigger Flag - PID found

    /** If pid to be removed is at head **/
    if (pSave == pq->pHeadProc) {
        pq->pHeadProc = pSave->pNextProc;       //Set next pid as head
        pSave->pNextProc = NULL;                //Set old head next pointer to NULL

        /* if proc is NOT only node on list */
        if (pq->pHeadProc != NULL) {
            pq->pHeadProc->pPrevProc = NULL;    //Set new head prev pointer to NULL
        }

            /* if proc IS only node on list */
        else {
            pSave->pPrevProc = NULL;            //Set old prev pointer to NULL
            pq->pTailProc = NULL;               //Set new tail to NULL
        }

        remFlag = true;                         //Trigger Flag - Process Removed
    }

    /*** Update Counters ***/
    /* List Total */
    pq->total--;            //update pq count
    
    /* Global Total */
    if (pq == &SleepingProcs){
        totalSleepingProcs--;
    }
    else if (pq == &requestQueue[0] || pq == &requestQueue[1]){
        totalRequestProcs--;
    }
    else {
        DebugConsole4("%s: Invalid List", __func__);
    }

    /*** Error Check: Pid Found and Removed ***/
    if (pidFlag && remFlag) {
        pSave = NULL;       //free() causing issues in USLOSS
    }
    else {
        if (pidFlag == false)
        {
            DebugConsole4("%s: Unable to locate pid [%d]. Halting...\n",
                            __func__, pid);
            halt(1);
        }
        else if (remFlag == false)
        {
            DebugConsole4("%s: Removal of pid [%d] unsuccessful. Halting...\n",
                            __func__, remFlag);
            halt(1);
        }
    }
    

    /*** Function Termination ***/
    return pid;     //process removal success

}/* RemoveFromList */


/* * * * * * * * * * * * * * * * END OF List Functions * * * * * * * * * * * * * * * */


/* * ********************************************************************************** */
/*                              Imported Phase 1 Functions                              */
/* ************************************************************************************ */

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
void DebugConsole4(char *format, ...) {
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
    int status = -1;

    if (args == NULL) {
        console("%s: Invalid syscall %d, no arguments.\n", __func__, args->number);
        console("%s: Process %d, terminating...\n", __func__, getpid());
        terminate_real(1);
    }

    switch (args->number) {
        case SYS_SLEEP:
            /*** INPUT ***
             *      arg1: delay amount (seconds)
             */ 
             /*** OUTPUT *** 
             *      arg4:  -1 - illegal values
             *              0 - otherwise
            */
            args->arg4 = (void *) sleep((__intptr_t) args->arg1);
            break;
        case SYS_DISKREAD:
            /*** INPUT ***
             *      arg1: memory address to transfer to
             *      arg2: number of sectors to read
             *      arg3: track start
             *      arg4: sector start
             *      arg5: unit
             */ 
             /*** OUTPUT *** 
             *      arg1:   0 - success
             *             >0 - disk status register 
             *      arg4:  -1 - illegal values
             *              0 - otherwise
            */
            args->arg4 = (void *) diskIO(DISK_READ,
                                        args->arg1, 
                                        (__intptr_t) args->arg2, 
                                        (__intptr_t) args->arg3, 
                                        (__intptr_t) args->arg4, 
                                        (__intptr_t) args->arg5);

            if(args->arg4 == -1){
                args->arg1 = DEV_INVALID;       //invalid parameters
            }
            else {
                args->arg1 = (void *) DEV_OK;   //success
            }

            //TODO: delete 
            // args->arg4 = (void *) diskRead((__intptr_t) &args->arg1,
            //                     (__intptr_t) args->arg2,
            //                     (__intptr_t) args->arg3,
            //                     (__intptr_t) args->arg4,
            //                     (__intptr_t) args->arg5);
            break;
        case SYS_DISKWRITE:
            /*** INPUT ***
             *      arg1: memory address to transfer to
             *      arg2: number of sectors to write
             *      arg3: track start
             *      arg4: sector start
             *      arg5: unit
             */ 
             /*** OUTPUT *** 
             *      arg1:   0 - success
             *             >0 - disk status register 
             *      arg4:  -1 - illegal values
             *              0 - otherwise
            */
            
            args->arg4 = (void *) diskIO(DISK_WRITE,
                                        args->arg1, 
                                        (__intptr_t) args->arg2, 
                                        (__intptr_t) args->arg3, 
                                        (__intptr_t) args->arg4, 
                                        (__intptr_t) args->arg5);
            
            if(args->arg4 == -1){
                args->arg1 = DEV_INVALID;       //invalid parameters
            }
            else {
                args->arg1 = (void *) DEV_OK;   //success
            }

            //TODO: delete
            // args->arg4 = (void *) diskWrite((__intptr_t) &args->arg1,
            //                     (__intptr_t) args->arg2,
            //                     (__intptr_t) args->arg3,
            //                     (__intptr_t) args->arg4,
            //                     (__intptr_t) args->arg5);
            
            break;
        case SYS_DISKSIZE:
            /*** INPUT ***
             *      arg1: disk unit number
             */ 
             /*** OUTPUT *** 
             *      arg1: sector size (bytes)
             *      arg2: number of sectors in track
             *      arg3: number of tracks on disk
             *      arg4:  -1 - illegal values
             *              0 - otherwise
            */

            args->arg4 = (void *) diskSize((__intptr_t) args->arg1,
                                (__intptr_t) &args->arg1,
                                (__intptr_t) &args->arg2,
                                (__intptr_t) &args->arg3);
            break;
        default:
            console("Unknown syscall number");
            halt(1);
    }

    if (is_zapped()) {
        //console("%s: process %d was zapped while in system call.\n", pFunctionName, getpid());
        terminate_real(0);
    }

    /*** Set User Mode ***/
    setUserMode();

} /* sysCall */


/* ------------------------------------------------------------------------
  Name -            SecToMsec
  Purpose -         Converts seconds to microseconds
  Parameters -      seconds
  Returns -         microseconds
  Side Effects -    none
  ----------------------------------------------------------------------- */
static int SecToMSec(int seconds) {
    return (seconds * 1000000);
} /* SecToMSec */