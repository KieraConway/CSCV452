/* Header File for Semaphores */

#ifndef PHASE3_SEMS_H
#define PHASE3_SEMS_H

typedef struct sem sem;

struct sem {
    int sID;
    int mutex;
};

#endif