/*
 * These are the definitions for phase 2 of the project
 */

#ifndef _PHASE2_H
#define _PHASE2_H

/*
 * Maximum line length. Used by terminal read and write.
 */
#define MAXLINE         80
#define MAXMBOX         2000 /* 2000 */      //maximum number of mailboxes.//TODO CHANGE BACK
#define MAXSLOTS        2500 /* 2500 */     //number of usable mailbox slots
#define MAXMESSAGE      150  /* largest possible message in a single slot (bytes)*/

/* returns id of mailbox, or -1 if no more mailboxes, -2 if invalid args */
extern int MboxCreate(int slots, int msg_size);

/* returns 0 if successful, -1 if invalid arg */
extern int MboxRelease(int mbox_id);

/* returns 0 if successful, -1 if invalid args */
extern int MboxSend(int mbox_id, void *msg_ptr, int msg_size);

/* returns size of received msg if successful, -1 if invalid args */
extern int MboxReceive(int mbox_id, void *msg_ptr, int max_msg_size);

/* returns 0 if successful, 1 if mailbox full, -1 if illegal args */
extern int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size);

/* returns 0 if successful, 1 if no msg available, -1 if illegal args */
extern int MboxCondReceive(int mbox_id, void *msg_ptr, int msg_max_size);

/* type = interrupt device type, unit = # of device (when more than one),
 * status = where interrupt handler puts device's status register.
 */
extern int waitdevice(int type, int unit, int *status);

/*  The sysargs structure */
typedef struct sysargs
{
        int number;
        void *arg1;
        void *arg2;
        void *arg3;
        void *arg4;
        void *arg5;
} sysargs;

//extern void             (*sys_vec[MAXSYSCALLS])(sysargs *args);
extern void             (*sys_vec[])(sysargs *args);

#endif
