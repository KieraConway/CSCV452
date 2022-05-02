#ifndef DRIVER_H     //Header Guards
#define DRIVER_H

/** ------------------------ Typedefs and Structs ------------------------ **/
typedef struct driver_proc driver_proc;
typedef struct driver_proc * driver_ptr;
typedef struct diskQueue diskQueue;
typedef struct diskList * disk_ptr;

/* Structures for Disk */
typedef struct diskList{
    driver_ptr    pDisk;          //points to disk
    struct diskList *pNextDisk; //points to next disk
    struct diskList *pPrevDisk; //points to previous disk
} DiskList;

typedef struct diskQueue {
    DiskList *pHeadDisk;        //points to disk list head
    DiskList *pTailDisk;        //points to disk list tail
    int total;                  //counts total disks in queue
} DiskQueue;

struct driver_proc {
    int     wake_time;    /* for sleep syscall */
    int     pid;
    int     been_zapped;
    int     mboxID;
    int     blockSem;
    /* Used for disk requests */
    int     operation;    /* DISK_READ, DISK_WRITE, DISK_SEEK, DISK_TRACKS */
    int     unit;
    int     track_start;
    int     current_track;      //current track location
    int     sector_start;       
    int     current_sector;     //current sector location
    int     num_sectors;
    void    *disk_buf;
    void    *disk_offset;       //current buffer location (512i)


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


