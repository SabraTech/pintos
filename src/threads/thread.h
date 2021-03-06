#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/synch.h"
#include "threads/fixed-point.h"

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/*
   Represents the thread as a child process, stores everything
   the parent needs to know, persists even after the child process
   exits, only the parent can destroy it.
 */
struct child_thread_elem
  {
    int exit_status;                 /* returned by wait (). */
    int loading_status;              /* to inform parent if the load was successful. */
    struct semaphore wait_sema;      /* used by the parent to block on a child when
                                        wait () is called on it. */
    tid_t tid;
    struct thread *t;                /* pointer to the thread, can be NULL if the process exits. */
    struct list_elem elem;           /* used by parent to put its children in a list. */
  };

 /*
     Represents an open file and associates it with a file descriptor.
 */
struct file_table_entry
 {
   int fd;                       /* file descriptor. */
   struct file *f;               /* pointer to open file. */
   struct list_elem elem;        /* used by the process to store its open files. */
 };

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

    /* Used by synch.c */
    struct list_elem cond_waiter_elem; /* List element to put thread in list of
                                          threads waiting on condition. */
                                          /* Used by timer_interrupt (), so interrupts should be
                                             disabled before accessing it from a kernel thread */

    struct list_elem sleeping_elem;     /* List element for sleeping_threads_list*/


#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
    /* Used by timer.c */
    int64_t wakeup_time;                /* tick at which a sleeping thread will wake up */

    /* Used by timer.c */
    struct semaphore blocked;           /* Used for blocking the thread when it is sleeping,
                                           should be initialized 0 before using, shouldn't be
                                           used for any other purposes*/

    struct semaphore blocked_on_cond;    /* Used for blocking thread when it is
                                           waiting on a condition variable. */

    int base_priority;

    struct list donar_list;               /* list of threads that donated
                                             priority to this thread. */

    struct lock *requested_lock;          /* Lock that the thread tried to access
                                             but failed due to priority inversion. */

    struct list_elem donar_elem;         /* List element for donar list. */

    struct thread *donee;                /* thread whose donar list contains me,
                                            used to propagate priority down to
                                            all donees  */

    int nice;                            /* Value that determines how "nice" the
                                            thread should be to other
                                            threads, between -20 to 20 and
                                            initialize with value of zero. */
    fixedpoint recent_cpu;               /* recent_cpu to measure how much CPU
                                          time each process has received "recently." */

    /* used by syscalls */
    struct list children_list;            /* If this thread is a user process thread
                                             this list contains all child process
                                             created using exec syscall */
    struct child_thread_elem *child_elem; /* A pointer will be allocated by exec
                                             syscall to be able to be accessed by
                                             parent if this thread is destroyed */
    struct list file_table;

  };

bool priority_comp (const struct list_elem *a, const struct list_elem *b, void *aux);

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

bool is_idle_thread (struct thread *t);
struct child_thread_elem *thread_get_child (tid_t tid);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

void thread_mlfqs_calc_priority (struct thread *t);
void thread_mlfqs_update (void);

void donate_priority (struct thread* donee);
#endif /* threads/thread.h */
