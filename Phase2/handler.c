
#include "handler.h"

extern int debugFlag2;

/* an error method to handle invalid syscalls */
void nullsys(sysargs *args)
{
    console("nullsys(): Invalid syscall. Halting...\n");
    halt(1);
} /* nullsys */


void clock_handler(int dev, void *unit)
{

   if (DEBUG2 && debugFlag2)
      console("clock_handler(): handler called\n");


} /* clock_handler */


void disk_handler(int dev, void *unit)
{

   if (DEBUG2 && debugFlag2)
      console("disk_handler(): handler called\n");


} /* disk_handler */


void term_handler(int dev, void *unit)
{

   if (DEBUG2 && debugFlag2)
      console("term_handler(): handler called\n");


} /* term_handler */


void syscall_handler(int dev, void *unit)
{

   if (DEBUG2 && debugFlag2)
      console("syscall_handler(): handler called\n");


} /* syscall_handler */
