// File:	worker_t.h

// List all group member's name:
//Venkata Sri Ram Srujan Nidadavolu(vsn9), Srujan Kashyap Dendukuri(sd1278)
// username of iLab:
// iLab Server:h206-1.cs.rutgers.edu

#ifndef WORKER_T_H
#define WORKER_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_WORKERS macro */
#define USE_WORKERS 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include<time.h>

#define READY 0
#define SCHEDULED 1
#define BLOCKED 2
#define FINISHED 4

typedef unsigned int uint;

typedef uint worker_t;


typedef struct TCB {
	/* add important states in a thread control block */
	// thread Id
	// thread status
	// thread context
	// thread stack
	// thread priority
	// And more ...

	// YOUR CODE HERE
	worker_t *id;
	int status;
	void *stack;
	ucontext_t context;
	int priority;
	struct TCB *next;
	int lock_required;
	int lock_acquired;
	uint mutex_id;
	void *vlaue_ptr;
	int waitStatus;

} tcb; 

/* mutex struct definition */
typedef struct worker_mutex_t {
	/* add something here */

	// YOUR CODE HERE
	uint mutexId;
	int flag;
} worker_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE
typedef struct queue_tcb{
	tcb *head;
	tcb *tail;
} queue_tcb;

typedef struct list_tcb{
	tcb *head;
} list_tcb;

/* Function Declarations: */
/*data structure*/
//queue related functions
queue_tcb * initqueue_tcb(void);
void enqueue_tcb(queue_tcb *, tcb *);
tcb * dequeue_tcb(queue_tcb *);
//linklist related functions
list_tcb * initList_tcb(void);
void addToFinishlist_tcb(list_tcb *,tcb *);
tcb * checkFinishedlist(list_tcb *, uint );

int testandset(int * ,int );
double calcTimerValue(void);

static void schedule(void);
static void sched_rr(void);
static void sched_mlfq(void);
void timer_handler(int signum);
/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, void *
    (*function)(void *), void * arg);

/* give CPU pocession to other user level worker threads voluntarily */
int worker_yield();

/* terminate a thread */
void worker_exit(void *value_ptr);

/* wait for thread termination */
int worker_join(worker_t thread, void **value_ptr);

/* initial the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex);

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex);

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex);

#ifdef USE_WORKERS
#define pthread_t worker_t
#define pthread_mutex_t worker_mutex_t
#define pthread_create worker_create
#define pthread_exit worker_exit
#define pthread_join worker_join
#define pthread_mutex_init worker_mutex_init
#define pthread_mutex_lock worker_mutex_lock
#define pthread_mutex_unlock worker_mutex_unlock
#define pthread_mutex_destroy worker_mutex_destroy
#endif

#endif
