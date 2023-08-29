// File:	worker.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "worker.h"
// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE

//#define MLFQ

#define QUANTUM_MICROSEC 010000
#define QUANTUM_SEC 0
#define MLFQ_LEVELS 8
#define THREAD_STACK_SIZE 1024*64


struct itimerval timer;
struct itimerval timer_disarm;
struct itimerval currentTimer;
struct sigaction sig_action;
double timerValue;

tcb *create_tcb;
tcb *scheduler_tcb;
tcb *current_tcb;
ucontext_t main_context;

uint threadIdCounter = 0;
uint mutexIdCounter = 0;
int notInitialized = 1;
#ifndef MLFQ
queue_tcb *queue_1;
#else
queue_tcb *queues_tcb[MLFQ_LEVELS];
int queueRecall;
#endif
list_tcb *finish_threads;
/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, 
                      void * (*function)(void *), void * arg) {

       // - create Thread Control Block (TCB)
       // - create and initialize the context of this worker thread
       // - allocate space of stack for this thread to run
       // after everything is set, push this thread into run queue and 
       // - make it ready for the execution.

       // YOUR CODE HERE
	   if(notInitialized){
		   //Intitializing the scheduler
		   scheduler_tcb = (tcb *) malloc(sizeof(tcb));
		   scheduler_tcb->id  = malloc(sizeof(unsigned int));
		   *(scheduler_tcb->id)= threadIdCounter;
		   scheduler_tcb->status = 5;
		   scheduler_tcb->stack = malloc(THREAD_STACK_SIZE);
		   getcontext(&(scheduler_tcb->context));
		   scheduler_tcb->context.uc_stack.ss_sp = scheduler_tcb->stack;
		   scheduler_tcb->context.uc_stack.ss_size = THREAD_STACK_SIZE;
		   scheduler_tcb->priority = 0;
		   makecontext(&scheduler_tcb->context,schedule,0);

		   //Intitializing the mainThread to currentThread
		   current_tcb = (tcb *) malloc(sizeof(tcb));
		   current_tcb->id = malloc(THREAD_STACK_SIZE);
		   *(current_tcb->id)= getpid();
		   current_tcb->status = READY;
		   current_tcb->priority = 0;
		   current_tcb->waitStatus = 0;

		   //Intiatializing the createThead
		   
		   create_tcb = (tcb *) malloc(sizeof(tcb));
		   create_tcb->id = thread;
		   *(create_tcb->id)= ++threadIdCounter;
		   create_tcb->status = READY;
		   create_tcb->stack = malloc(THREAD_STACK_SIZE);
		   getcontext(&(create_tcb->context));
		   create_tcb->context.uc_stack.ss_sp = create_tcb->stack;
		   create_tcb->context.uc_stack.ss_size = THREAD_STACK_SIZE;
		   create_tcb->priority = 0;
		   create_tcb->waitStatus = 0;
		   makecontext(&create_tcb->context,(void (*)(void))function,1,arg);
		   
		   swapcontext(&(current_tcb->context),&scheduler_tcb->context);
		   
			return 0;
	   }
			create_tcb = (tcb *) malloc(sizeof(tcb));
		   create_tcb->id = thread;
		   *(create_tcb->id)= ++threadIdCounter;
		   create_tcb->status = READY;
		   create_tcb->stack = malloc(THREAD_STACK_SIZE);
		   getcontext(&(create_tcb->context));
		   create_tcb->context.uc_stack.ss_sp = create_tcb->stack;
		   create_tcb->context.uc_stack.ss_size = THREAD_STACK_SIZE;
		   create_tcb->priority = 0;
		   current_tcb->waitStatus = 0;
		   makecontext(&create_tcb->context,(void (*)(void))function,1,arg);

			#ifndef MLFQ
		enqueue_tcb(queue_1,create_tcb);
		#else
		enqueue_tcb(queues_tcb[0],create_tcb);
		#endif
		   

    return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield() {
	
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// YOUR CODE HERE
	raise(SIGPROF);
	
	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - de-allocate any dynamic memory created when starting this thread

	// YOUR CODE HERE
	current_tcb->vlaue_ptr = value_ptr;
	current_tcb->status = FINISHED;
	raise(SIGPROF);

};


/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE
	tcb *temp;
	while (1)
	{
		current_tcb->waitStatus=1;
		if((temp = checkFinishedlist(finish_threads,thread))!=NULL){
			if(value_ptr!=NULL)
			value_ptr = &(temp->vlaue_ptr);
			break;
		}
		worker_yield();
	}
		current_tcb->waitStatus = 0;


	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//- initialize data structures for this mutex

	// YOUR CODE HERE
	mutex->flag = 0;
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {

        // - use the built-in test-and-set atomic function to test the mutex
        // - if the mutex is acquired successfully, enter the critical section
        // - if acquiring mutex fails, push create thread into block list and
        // context switch to the scheduler thread

        // YOUR CODE HERE
		
		while(testandset(&(mutex->flag),1)==1){
			current_tcb->waitStatus=1;
			worker_yield();
		}
		current_tcb->waitStatus=0;
        return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
	// - release mutex and make it available again. 
	// - put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	mutex->flag  = 0;
	return 0;
};


/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {
	// - de-allocate dynamic memory created in worker_mutex_init
	//worker_mutex_unlock(mutex);
	//free((void *)mutex);
	return 0;
}

/* scheduler */
static void schedule() {
	// - every time a timer interrupt occurs, your worker thread library 
	// should be contexted switched from a thread context to this 
	// schedule() function

	// - invoke scheduling algorithms according to the policy (RR or MLFQ)

	// if (sched == RR)
	//		sched_rr();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

// - schedule policy

	if(notInitialized){
		notInitialized = 0;
		#ifndef MLFQ
		queue_1 = initqueue_tcb();
		enqueue_tcb(queue_1,current_tcb);
		enqueue_tcb(queue_1,create_tcb);
		#else
		for(int i = 0; i<MLFQ_LEVELS;i++){
			queues_tcb[i] = initqueue_tcb();
		}
		enqueue_tcb(queues_tcb[0],current_tcb);
		enqueue_tcb(queues_tcb[0],create_tcb);
		#endif
		
		
		finish_threads = initList_tcb();
		timer.it_value.tv_sec = QUANTUM_SEC;
		timer.it_value.tv_usec = QUANTUM_MICROSEC;
		timer_disarm.it_value.tv_sec = 0;
		timer_disarm.it_value.tv_usec = 0;
		memset(&sig_action,0,sizeof(sig_action));
		sig_action.sa_handler = timer_handler;
		sigaction(SIGPROF,&sig_action,NULL);
		#ifndef MLFQ
		current_tcb= dequeue_tcb(queue_1);
		#else
		current_tcb= dequeue_tcb(queues_tcb[0]);
		//queueRecall = 1;
		#endif
		swapcontext(&scheduler_tcb->context,&current_tcb->context);
	}
while(1){
#ifndef MLFQ
	// Choose RR
	
sched_rr();

#else 
	// Choose MLFQ
	sched_mlfq();
#endif
}
}

#ifndef MLFQ
/* Round-robin (RR) scheduling algorithm */
static void sched_rr() {
	// - your own implementation of RR
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	
	while(1){
		enqueue_tcb(queue_1,current_tcb);
	current_tcb = dequeue_tcb(queue_1);
	if(current_tcb == NULL){
		break;
	}
	if(current_tcb->status == FINISHED){
		if(current_tcb->stack!=NULL)
		free(current_tcb->stack);
		addToFinishlist_tcb(finish_threads,current_tcb);
		current_tcb = dequeue_tcb(queue_1);
	}
	if(current_tcb->status == BLOCKED){
		continue;
	}
	if(current_tcb->status == READY){
		setitimer(ITIMER_PROF,&timer,NULL);
		swapcontext(&scheduler_tcb->context,&current_tcb->context);
	}
	}

}
#else 
static int sched_rr_forMlfq(queue_tcb * queue_1){

	while(1){
		if(queueRecall==0){

		if(current_tcb!=NULL){
		enqueue_tcb(queues_tcb[current_tcb->priority],current_tcb);
		}
		
	current_tcb = dequeue_tcb(queue_1);
	if(current_tcb == NULL){
		return 1;
	}
	if(current_tcb->status == FINISHED){
		if(current_tcb->stack!=NULL)
		free(current_tcb->stack);
		addToFinishlist_tcb(finish_threads,current_tcb);
		current_tcb = dequeue_tcb(queue_1);
		if (current_tcb==NULL)
		{
			return 1;
		}
		while(checkFinishedlist(finish_threads,*(current_tcb->id))!=NULL){
			current_tcb = dequeue_tcb(queue_1);
			if (current_tcb==NULL)
		{
			return 1;
		}
		}
	}
	if(current_tcb->status == BLOCKED){
		continue;
	}
	if(current_tcb->status == READY){
		queueRecall = 1;
		setitimer(ITIMER_PROF,&timer,NULL);
		swapcontext(&scheduler_tcb->context,&current_tcb->context);
	}
		}else{
			return 0;
		}
	}
	return 0;
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	queueRecall = 0;
	int i = 0;
	while(1){
		queueRecall = 0;
	if(sched_rr_forMlfq(queues_tcb[i])==1){
		if(i<MLFQ_LEVELS-1){
			i++;
		}else {
			break;
		}
	}else{
		break;
	}
		}
}
#endif
// Feel free to add any other functions you need

// YOUR CODE HERE
queue_tcb * initqueue_tcb(void){
	queue_tcb *temp = (queue_tcb *) malloc(sizeof(struct queue_tcb));
	tcb *tem = (tcb *) malloc(sizeof(struct TCB));
	tem->next = NULL;
	temp->head =  tem;
	temp->tail =  tem;
	return temp;

}

void enqueue_tcb(queue_tcb *queue_gen, tcb *gen){
	gen->next = NULL;
	queue_gen->tail->next = gen;
	if(queue_gen->head->next ==NULL){
		queue_gen->head->next = gen;
	}
	queue_gen->tail = gen;
}

tcb * dequeue_tcb(queue_tcb * queue_gen){
	tcb *temp = queue_gen->head->next; 
	if(temp!=NULL){
		queue_gen->head->next = temp->next;
		temp->next = NULL;
	}
	return temp;
}

list_tcb * initList_tcb(){
	list_tcb * temp = malloc(sizeof(struct list_tcb));
	tcb *tem = malloc(sizeof(struct TCB));
	tem->next = NULL;
	temp->head = tem;
	return temp;
}

void addToFinishlist_tcb(list_tcb *list,tcb *fins){
	tcb *temp = list->head;
	while(temp->next!=NULL){
		temp = temp->next;
	}
	temp->next = fins;
}
tcb * checkFinishedlist(list_tcb *list, uint thread_id){
	tcb *temp = list->head;
	while(temp->next!=NULL){
		if(*(temp->next->id)==thread_id){
			return temp->next;
		}
		temp = temp->next;
	}
	return temp->next;
}



void timer_handler(int signum){
	getitimer(ITIMER_PROF,&currentTimer);// To Check whether the process yeilds before the quantum
	timerValue = calcTimerValue();//calculate the time;
	#ifdef MLFQ
	if(timerValue == 0.0||current_tcb->waitStatus){
		if(current_tcb->priority<MLFQ_LEVELS-1){
			current_tcb->priority++;
		}
	 }
	#endif
	setitimer(ITIMER_PROF,&timer_disarm,NULL);
	swapcontext(&current_tcb->context,&scheduler_tcb->context);
}

int testandset(int *old, int value){
	int temp = *(old);
	*old = value;
	return temp;
}

double calcTimerValue(){
	return ((currentTimer.it_value.tv_sec)+((currentTimer.it_value.tv_usec)*1e-6));
}




