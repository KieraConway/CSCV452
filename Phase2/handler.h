#ifndef PHASE_2_HANDLER_H
#define PHASE_2_HANDLER_H

#include <stdio.h>
#include <phase1.h>
#include <phase2.h>
#include "message.h"




#include "handler.h"

extern int debugFlag2;

/* an error method to handle invalid syscalls */
void nullsys(sysargs *args);
void clock_handler(int dev, void *unit);
void disk_handler(int dev, void *unit);
void term_handler(int dev, void *unit);


void syscall_handler(int dev, void *unit);

#endif //PHASE_2_HANDLER_H
