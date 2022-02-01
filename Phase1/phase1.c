/* ------------------------------------------------------------------------
   phase1.c

   CSCV 452
<<<<<<< Updated upstream

=======
   
	Katelyn Griffith
	Kiera Conway
>>>>>>> Stashed changes
   ------------------------------------------------------------------------ */
   /* * * * * * * * * * * * * * * * * * * * *	
   * Author Notes
   *
   *   for zapped processes -> use linked list
   * * * * * * * * * * * * * * * * * * * * */
   
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <phase1.h>
#include "kernel.h"

/* ------------------------- Prototypes ----------------------------------- */
<<<<<<< Updated upstream
int sentinel (char *);        //KC TODO void to char
=======
int sentinel (char *);			//TODO: void-> char
>>>>>>> Stashed changes
extern int start1 (char *);
void dispatcher(void);
void launch();
static void enableInterrupts();
static void check_deadlock();

<<<<<<< Updated upstream
=======
// added functions for processing kg
static void check_kernel_mode(const char *functionName); //Check Kernel Mode
void ClockIntHandler(int dev, void *arg);                //Clock handler
void DebugConsole(char *format, ...);                    //Debug console
void ListInsert(proc_ptr *child,proc_struct *table);
int GetNextPid();
int check_io();
proc_ptr GetNextReadyProc();
>>>>>>> Stashed changes

/* -------------------------- Globals ------------------------------------- */

/* Patrick's debugging global variable... */
int debugflag = 1;

/* the process table */
proc_struct ProcTable[MAXPROC];

/* Process lists  */

/*TODO decide process list type
proc_ptr readyPriority1;
proc_ptr readyPriority2;
proc_ptr readyPriority3;
proc_ptr readyPriority4;
proc_ptr readyPriority5;
*/

/* current process ID */
proc_ptr Current;

/* the next pid to be assigned */
unsigned int next_pid = SENTINELPID;

<<<<<<< Updated upstream
=======
/* number of active processes */ 
unsigned int numProc = 0; 	// kg

/* Error Check: context_switch() Initializer */
int init = 1;
>>>>>>> Stashed changes

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
   char ReadyList;   //KC TODO, Temp line fix for line 59

   /* initialize the process table */

   /* Initialize the Ready list, etc. */
   if (DEBUG && debugflag)
      console("startup(): initializing the Ready & Blocked lists\n");
   ReadyList = NULL;

   /* Initialize the clock interrupt handler */

   /* startup a sentinel process */
   if (DEBUG && debugflag)
       console("startup(): calling fork1() for sentinel\n");
   result = fork1("sentinel", sentinel, NULL, USLOSS_MIN_STACK,
                   SENTINELPRIORITY);
   if (result < 0) {
      if (DEBUG && debugflag)
         console("startup(): fork1 of sentinel returned error, halting...\n");
      halt(1);
   }
  
   /* start the test process */
   if (DEBUG && debugflag)
      console("startup(): calling fork1() for start1\n");
   result = fork1("start1", start1, NULL, 2 * USLOSS_MIN_STACK, 1);
   if (result < 0) {
      console("startup(): fork1 for start1 returned an error, halting...\n");
      halt(1);
   }

   console("startup(): Should not see this message! ");
   console("Returned from fork1 call that created start1\n");

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
   if (DEBUG && debugflag)
      console("in finish...\n");
} /* finish */

/* ------------------------------------------------------------------------
   Name - fork1
   Purpose - Gets a new process from the process table and initializes
             information of the process.  Updates information in the
             parent process to reflect this child process creation.
   Parameters - the process procedure address, the size of the stack and
                the priority to be assigned to the child process.

            //KC: added complete descriptions below//
            name:       descriptive name for process, no longer than 
                        MAXNAME
            f:          function child process is executing from
            arg:        f argument
            stacksize:  process stacksize in bytes
            priority:   priority to be assigned to child process

   Returns - the process id of the created child or -1 if no child could
             be created or if priority is not between max and min priority.
   Side Effects - ReadyList is changed, ProcTable is changed, Current
                  process information changed
   ------------------------------------------------------------------------ */
int fork1(char *name, int (*f)(char *), char *arg, int stacksize, int priority)
{
   int proc_slot;

   /*KC: DEBUG = 0 in kernel.h */
   /*KC: debugflag = 1 local */

   if (DEBUG && debugflag)                               
      console("fork1(): creating process %s\n", name);

   /* test if in kernel mode; halt if in user mode */


   /* Return if stack size is too small */

<<<<<<< Updated upstream
   /* find an empty slot in the process table */

   /* fill-in entry in process table */
   if ( strlen(name) >= (MAXNAME - 1) ) {
      console("fork1(): Process name is too long.  Halting...\n");
      halt(1);
   }
   strcpy(ProcTable[proc_slot].name, name);
   ProcTable[proc_slot].start_func = f;
   if ( arg == NULL )
      ProcTable[proc_slot].start_arg[0] = '\0';
   else if ( strlen(arg) >= (MAXARG - 1) ) {
      console("fork1(): argument too long.  Halting...\n");
      halt(1);
   }
   else
      strcpy(ProcTable[proc_slot].start_arg, arg);
=======
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
>>>>>>> Stashed changes

   /* Initialize context for this process, but use launch function pointer for
    * the initial value of the process's program counter (PC)
    */
   context_init(&(ProcTable[proc_slot].state), psr_get(),
                ProcTable[proc_slot].stack, 
                ProcTable[proc_slot].stacksize, launch);

   /* for future phase(s) */
   p1_fork(ProcTable[proc_slot].pid);

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

<<<<<<< Updated upstream
   if (DEBUG && debugflag)
      console("launch(): started\n");

   /* Enable interrupts */
   //enableInterrupts();   //KC: TODO  
=======
	DebugConsole("launch(): started\n");

	/* Enable interrupts */
	enableInterrupts();
>>>>>>> Stashed changes

	/* Call the function passed to fork1, and capture its return value */
	result = Current->start_func(Current->start_arg);

<<<<<<< Updated upstream
   if (DEBUG && debugflag)
      console("Process %d returned to launch\n", Current->pid);
=======
	DebugConsole("Process %d returned to launch\n", Current->pid);
>>>>>>> Stashed changes

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
     * check priority lists/array
     */

	for (int i = 0; i < MAXPROC; i++)
	{
		// Get highest priority process running in process table
		if ((ProcTable[i].status == STATUS_READY) && (ProcTable[i].priority < highestPrior))
		{
			//TODO: if (ProcTable[i].priority is < pNextProc) , remove break
			pNextProc = &ProcTable[i];
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
<<<<<<< Updated upstream
   proc_ptr next_process;

   p1_switch(Current->pid, next_process->pid);
=======
	proc_ptr oldProcess; // added in kg
	proc_ptr next_Process;

	//p1_switch(Current->pid, next_process->pid); TODO

	next_Process = GetNextReadyProc(); //assign newProcess; added in kg
	oldProcess = Current; 			// added in kg

	/* Make sure Current is pointing to the process we are switching to
	must be done Before context_switch() */
	Current = next_Process; // added in kg

	context_switch((oldProcess == NULL) ? NULL : &oldProcess->state, &next_Process->state); // added in kg
>>>>>>> Stashed changes
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
   if (DEBUG && debugflag)
      console("sentinel(): called\n");
   while (1)
   {
      check_deadlock();
      waitint();
   }
} /* sentinel */

<<<<<<< Updated upstream
=======
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


int getpid(void)
{
	// TODO 
	// based on processes, check for running status and return the pid
}


int is_zapped(void)
{
	// TODO 
	// return 0 if not zapped
	// return 1 if zapped
}


void dump_processes(void)
{
	// TODO 
	// prints process ifnro to console
	// for each PCB, output:
	// PID, parent' PID, priority, process status, # children, CPU time consumed, and name
}

>>>>>>> Stashed changes

/* check to determine if deadlock has occurred... */
static void check_deadlock()
{
<<<<<<< Updated upstream
=======
   if (check_io() == 1)
      return;

   // TODO 
   /* Has everyone terminated? */
   // check the number of process
   // if there is only one active prcoess
   // halt(0);
   //otherwise
   //halt(1);
   
>>>>>>> Stashed changes
} /* check_deadlock */

/*
 * Enable the interrupts.
 */
static void enableInterrupts()
{
	check_kernel_mode(__func__);
	/*Confirmed Kernel Mode*/
	int psr = psr_get();

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
<<<<<<< Updated upstream
=======

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
   psrValue.integer_part = psr_get();

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
		/*va_list argptr;
		va_start(argptr, format);
		console(format, argptr);
		va_end(argptr);*/
		//TODO
		console("%s\n", format);
	}
}

/* ------------------------------------------------------------------------
Name - ClockIntHandler
Purpose -
Parameters - 
Returns - nothing
Side Effects -
----------------------------------------------------------------------- */
void clockHandler(int dev, void *arg)
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
Name - ListInsert
Purpose - Add newProcess to list of child process
Parameters -
Returns -
Side Effects -
----------------------------------------------------------------------- */

// Linked list - either singly or doubly
void ListInsert(proc_ptr *child,proc_struct *table)
{

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


>>>>>>> Stashed changes
