/* ------------------------------------------------------------------------
   Katelyn Griffith -
   phase1.c

   CSCV 452

   NOTE for zapped processes -> use linked list
   ------------------------------------------------------------------------ */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <phase1.h>
#include "kernel.h"

/* ------------------------- Prototypes ----------------------------------- */
int sentinel (char *);
extern int start1 (char *);
void dispatcher(void);
void launch();
static void enableInterrupts();
static void check_deadlock();

// added functions for processing kg
static void check_kernel_mode(const char *functionName);
int GetNextPid();
void clockHandler(int dev, void *arg);
proc_ptr GetNextReadyProc();
void DebugConsole(char *format, ...);
int check_io();
void ListInsert(proc_ptr *child,proc_struct *table);

/* -------------------------- Globals ------------------------------------- */

/* Patrick's debugging global variable... */
int debugflag = 1;

/* the process table */
proc_struct ProcTable[MAXPROC];

/* Process lists  */
proc_ptr readyProcs[5]; // Linked list of priorities
// if empty, run sentinel

/* current process ID */
proc_ptr Current;

/* the next pid to be assigned */
unsigned int next_pid = SENTINELPID;

// added in kg 
unsigned int numProc = 0; // number of active processes
int init = 1;


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

   /* Initialize the Ready list, etc. */
   DebugConsole("startup(): initializing the Ready & Blocked lists\n");

   //ReadyList = NULL; List of ready processes

   /* Initialize the clock interrupt handler */
   int_vec[CLOCK_INT] = &clockHandler; // added in kg

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
int fork1(char *name, int (*func)(char *), char *arg, int stacksize, int priority)
{
   int proc_slot;
   int newPid; // added in kg

   DebugConsole("fork1(): creating process %s\n", name);

   /* test if in kernel mode; halt if in user mode */
   check_kernel_mode(__func__);

   /* Return if stack size is too small-> no empty slots in process table*/
   // Added flag checks in kg
   if (stacksize > MAXPROC)
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
   if (strcmp(name, '\0') == 0)
   {
      console("name was null...\n");
      return -1;
   }

   // Stacksize is less than USLOSS_MIN_STACK
   if (stacksize < USLOSS_MIN_STACK)
   {
      console("stacksize was less than minimum stack address...\n");
      return -2;
   }

   /* find an empty slot in the process table */
   newPid = GetNextPid(); // added in kg
   proc_slot = newPid % MAXPROC; // added in kg
   ProcTable[proc_slot].pid = newPid; // added in kg
   ProcTable[proc_slot].priority = priority; // added in kg
   ProcTable[proc_slot].stacksize = stacksize; // added in kg
   ProcTable[proc_slot].status = STATUS_READY; // added in kg
   ProcTable[proc_slot].stack = malloc(stacksize); // added in kg

   // Check if out of memory
   if (ProcTable[proc_slot].stack == NULL)
   {
      DebugConsole("Out of memory.\n");
      halt(1);
   }

   /* fill-in entry in process table */
   if ( strlen(name) >= (MAXNAME - 1) ) {
      DebugConsole("fork1(): Process name is too long.  Halting...\n");
      halt(1);
   }

   strcpy(ProcTable[proc_slot].name, name);
   ProcTable[proc_slot].start_func = func;

   if ( arg == NULL )
      ProcTable[proc_slot].start_arg[0] = '\0';
   else if ( strlen(arg) >= (MAXARG - 1) )
   {
      DebugConsole("fork1(): argument too long.  Halting...\n");
      halt(1);
   }
   else
      strcpy(ProcTable[proc_slot].start_arg, arg);

   /* set the parent and child values */
   ListInsert(&Current->child_proc_ptr, &ProcTable[proc_slot]); // Linked list insert child Params: (head ptr, first node in list)

   /* Initialize context for this process, but use launch function pointer for
    * the initial value of the process's program counter (PC)
    */
   context_init(&(ProcTable[proc_slot].state), psr_get(),
                ProcTable[proc_slot].stack, 
                ProcTable[proc_slot].stacksize, launch);

   /* for future phase(s) */
   p1_fork(ProcTable[proc_slot].pid);

   // added in kg
   if (!init)
   {
      // Call dispatcher
      dispatcher();
   }

   // end of added in

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
} /* quit */


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
   int procSlot = 0; // added in kg
   proc_ptr oldProcess; // added in kg
   proc_ptr next_Process;

   //p1_switch(Current->pid, next_process->pid); TODO

   next_Process = GetNextReadyProc(); // added in kg
   oldProcess = Current; // added in kg

   // Make sure Current is pointing to thge process we're switching to.
   // Needs to be done before context switch
   Current = next_Process; // added in kg

   context_switch((oldProcess == NULL) ? NULL : &oldProcess->state, &next_Process->state); // added in kg
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
   // call is_zapped()
   // won't return until zapped prcoess has called quit
   // print error msg and halt(1) if process tries to zap itself
   // or attempts to zap a non-existent prcoess

   // return values:
   // -1 - calling process itself was zapped while in zap
   // 0 - zapped process has called quit
}


int getpid(void)
{
   // based on processes, check for running status and return the pid
}


int is_zapped(void)
{
   // return 0 if not zapped
   // return 1 if zapped
}


void dump_processes(void)
{
   // prints process ifnro to console
   // for each PCB, output:
   // PID, parent' PID, priority, process status, # children, CPU time consumed, and name
}


/* check to determine if deadlock has occurred... */
static void check_deadlock()
{
   if (check_io() == 1)
      return;

   /* Has everyone terminated? */
   // check the number of process
   // if there is only one active prcoess
   // halt(0);
   //otherwise
   //halt(1);
} /* check_deadlock */


/*
 * Disables the interrupts.
 */
void disableInterrupts()
{
  /* turn the interrupts OFF iff we are in kernel mode */
  if((PSR_CURRENT_MODE & psr_get()) == 0) {
    //not in kernel mode
    console("Kernel Error: Not in kernel mode, may not disable interrupts\n");
    halt(1);
  } else
    /* We ARE in kernel mode */
    psr_set( psr_get() & ~PSR_CURRENT_INT );
} /* disableInterrupts */


// Added functions kg
/* ------------------------------------------------------------------------
   Name - check_kernel_mode
   Purpose - Checks the PSR for kernel mode and halts if in user mode
   Parameters - none
   Returns - nothing
   Side Effects - Will halt if not kernel mode
   ----------------------------------------------------------------------- */
static void check_kernel_mode(const char *functionName)
{
   union psr_values psrValue; /* holds caller's psr values */

   DebugConsole("check_kernel_node(): verifying kernel mode for %s\n", functionName);

   /* test if in kernel mode; halt if in user mode */
   psrValue.integer_part = psr_get();

   if (psrValue.bits.cur_mode == 0)
   {
      console("Kernel mode expected, but function called in user mode.\n");
      halt(1);
   }

   console("Function is in Kernel mode (:\n");
}


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

      newPid = next_pid++;
   }

   return newPid;
}


static void enableInterrupts()
{
   check_kernel_mode(__func__);

   int psr = psr_get();
}


proc_ptr GetNextReadyProc()
{
   int highestPrior = 6;
   proc_ptr pNextProc = NULL;

   // check priority array
   // here


   for (int i = 0; i < MAXPROC; i++)
   {
      // Get highest priority process running in process table
      if ((ProcTable[i].status == STATUS_READY) && (ProcTable[i].priority < highestPrior))
      {
         pNextProc = &ProcTable[i];
         break;
      }
   }

   return pNextProc; // return pointer to next ready proc
}


// This is apparently all we need -> will be important for phase 2
int check_io()
{
   return 0;
}


// Linked list - either singly or doubly
void ListInsert(proc_ptr *child,proc_struct *table)
{

}


void clockHandler(int dev, void *arg)
{
   int i = 0;
   // time-slice = 80 milliseconds
   // detect 80 ms -> switch to next highest priority processor
   //if (Current->runtime > 80ms) dispatcher();

}


// Displaying debugging prompts
void DebugConsole(char *format, ...)
{
   if (DEBUG && debugflag)
   {
      /*va_list argptr;
      va_start(argptr, format);
      console(format, argptr);
      va_end(argptr);*/

      printf("%s\n", format);
   }
}

