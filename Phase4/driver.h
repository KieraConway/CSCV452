#ifndef DRIVER_H     //Header Guards
#define DRIVER_H

#include "phase4_helper.h"

/** ------------------------ Typedefs and Structs ------------------------ **/
typedef struct driver_proc * driver_proc_ptr;

struct driver_proc {
    driver_proc_ptr next_ptr;

    int   wake_time;    /* for sleep syscall */
    int   been_zapped;

    /* Used for disk requests */
    int   operation;    /* DISK_READ, DISK_WRITE, DISK_SEEK, DISK_TRACKS */
    int   track_start;
    int   sector_start;
    int   num_sectors;
    void *disk_buf;

    //more fields to add

};

struct psr_bits {
    unsigned int cur_mode:1;
    unsigned int cur_int_enable:1;
    unsigned int prev_mode:1;
    unsigned int prev_int_enable:1;
    unsigned int unused:28;
};

union psr_values {
    struct psr_bits bits;
    unsigned int integer_part;
};

#endif


