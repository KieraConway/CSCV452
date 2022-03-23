#include "handler.h"
#include "message.h"

extern int debugFlag2;

/* Bring in function references */
extern void DebugConsole2(char *format, ...);
extern void check_kernel_mode(const char *functionName);
extern void disableInterrupts();
extern void enableInterrupts();
extern int io_mbox[7];

#define DISKLOC 1   // Starting location of disk slots on MailboxTable
#define TERMLOC 3   // Starting location of term slots on MailboxTable

static int interrupt_count = 1;


/* an error method to handle invalid syscalls */
void nullsys(sysargs *args)
{
    console("nullsys(): Invalid syscall %d. Halting...\n", args->number);
    halt(1);
} /* nullsys */


void clock_handler2(int dev, void *unit)
{
    int msg = 0;
    int *msgptr = &msg;
    int result;
    int lunit = (int)unit;

    DebugConsole2("%s: handler called\n", __func__);

    /* Check for kernel mode */
    check_kernel_mode(__func__);

    /* Disable interrupts */
    disableInterrupts();

    /* Invalid argument checks */
    if (dev != CLOCK_DEV) {
        DebugConsole2("%s: handler called. Halting.\n", __func__);
        halt(1);
    }

    if (lunit != 0) {
        DebugConsole2("%s: Unit value invalid. Halting.\n", __func__);
        halt(1);
    }

    /* Time Slice 80 ms */
    if (interrupt_count == 4) {
        time_slice();
    }

    /* Send a message every 5 interrupts */
    if (interrupt_count == 5) {
        int status;
        result = MboxCondSend(io_mbox[0], msgptr, sizeof(int));

        // todo checks go here for result

        interrupt_count = 0;
    }

    interrupt_count++;

    /* Enable interrupts */
    enableInterrupts();
} /* clock_handler */


void disk_handler(int dev, void *unit)
{
    int status;
    int result;
    int lunit = (int)unit;

    DebugConsole2("%s: handler called\n", __func__);

    /* Check for kernel mode */
    check_kernel_mode(__func__);

    /* Disable interrupts */
    disableInterrupts();

    /* Invalid argument checks */
    if (dev != DISK_DEV) {
        DebugConsole2("%s: Device value invalid. Halting.\n", __func__);
        halt(1);
    }

    if ((lunit > 1) || (lunit < 0)) {
        DebugConsole2("%s: Unit value invalid. Halting.\n", __func__);
        halt(1);
    }

    device_input(DISK_DEV, lunit, &status);

    // assuming lunit is 1 or 2
    result = MboxCondSend(io_mbox[1 + lunit],
                          &status,
                          sizeof(int));

    // todo checks go here for result

    /* Enable interrupts */
    enableInterrupts();
} /* disk_handler */


void term_handler(int dev, void *unit)
{
    int status;
    int result;
    int lunit = (int)unit;

    console("%s: started, dev = %d, unit = %d\n", __func__, dev, lunit);

    DebugConsole2("%s: handler called\n", __func__);

    /* Check for kernel mode */
    check_kernel_mode(__func__);

    /* Disable interrupts */
    disableInterrupts();

    /* Invalid argument checks */
    if (dev != TERM_DEV) {
        DebugConsole2("%s: Device value invalid. Halting.\n", __func__);
        halt(1);
    }

    if ((lunit > 3) || (lunit < 0)) {
        DebugConsole2("%s: Unit value invalid. Halting.\n", __func__);
        halt(1);
    }

    device_input(TERM_DEV, unit, &status);

    result = MboxCondSend(io_mbox[3 + lunit],
                          status,
                          sizeof(int));

    // todo checks go here for result

    /* Enable interrupts */
    enableInterrupts();
} /* term_handler */


void syscall_handler(int dev, void *unit)
{
    sysargs *sys_ptr;
    sys_ptr = (sysargs*)unit;

    console("%s: started, dev = %d, sys number = %d\n",
            __func__,
            dev,
            sys_ptr->number);

    /* Check for kernel mode */
    check_kernel_mode(__func__);

    /* Disable interrupts */
    disableInterrupts();

    /* Sanity check: if the interrupt is not SYSCALL_INT, halt(1) */
    if (dev != SYSCALL_INT) {
        DebugConsole2("%s: Device value invalid. Halting.\n", __func__);
        halt(1);
    }

    /* check what system: if the call is not in the range between 0 and MAXSYSCALLS, , halt(1) */
    if ((sys_ptr->number < 0) || (sys_ptr->number >= MAXSYSCALLS)) {
        console("syscall_handler: sys number %d is wrong.  Halting...\n", sys_ptr->number);
        halt(1);
    }

    /* Now it is time to call the appropriate system call handler */
    sys_vec[sys_ptr->number](sys_ptr);

    /* Enable interrupts */
    enableInterrupts();
} /* syscall_handler */