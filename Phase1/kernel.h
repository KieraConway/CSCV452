#define DEBUG 1

typedef struct proc_struct proc_struct;

typedef struct proc_struct * proc_ptr;

// ADDED FROM WEEK 3 VIDEO
#define STATUS_EMPTY    0
#define STATUS_RUNNING  1
#define STATUS_READY    2
#define STATUS_QUIT     3
#define STATUS_BLOCKED  4
#define STATUS_ZAPPED   5
#define STATUS_ZAPPER   6
#define STATUS_LAST     7

struct proc_struct {
   proc_ptr       next_proc_ptr;
   proc_ptr       child_proc_ptr;
   proc_ptr       next_sibling_ptr;
   char           name[MAXNAME];     /* process's name */
   char           start_arg[MAXARG]; /* args passed to process */
   context        state;             /* current context for process */
   short          pid;               /* process id */
   int            priority;
   int (* start_func) (char *);   /* function where process begins -- launch */
   char          *stack;
   unsigned int   stacksize;
   int            status;         /* READY, BLOCKED, QUIT, etc. */
   /* other fields as needed... */
   proc_ptr       parent_proc_ptr;  // for refering back to parent process inside of ListInsert() within phase1.c
   proc_ptr       prev_proc_ptr;    // used doubly linked list
   proc_ptr       prev_sibling_ptr;
   proc_ptr       next_zapping_ptr;
   proc_ptr       next_prev_ptr;
   //proc_list      children;
   //proic_list     zappers;
   int            startTime;
   int            switchTime; // last time switched
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

/* Some useful constants.  Add more as needed... */
#define NO_CURRENT_PROCESS NULL
#define MINPRIORITY 5
#define MAXPRIORITY 1
#define SENTINELPID 1
#define SENTINELPRIORITY LOWEST_PRIORITY

