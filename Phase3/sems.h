/* Header File for Semaphores */

#ifndef PHASE3_SEMS_H
#define PHASE3_SEMS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tables.h"
#include "lists.h"
#include "phase3_helper.h"

#define SEM_READY 0
#define SEM_USED  1
#define LOCKED    1
#define UNLOCKED  0
#define FREEING   2

/** ------------------------ Typedefs and Structs ------------------------ **/
typedef struct semaphore_struct semaphore_struct;
typedef struct semaphore_struct * sem_struct_ptr;

/** ------------------------ External Prototypes ------------------------ **/
/* --------------------- External tables.c Prototypes --------------------- */
/** Semaphore Table **/
/* Initializes Sem Table Entry */
void SemaphoreInit(int index, short sid);
/* Adds Semaphore to Table */
void AddToSemTable(int sid, int newStatus, int newValue);
/* Gets Semaphore index from sid | Returns Corresponding index */
int GetSemIndex(int sid);

#endif