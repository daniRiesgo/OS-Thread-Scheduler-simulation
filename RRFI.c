#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include "mythread.h"
#include "interrupt.h"
#include "queue.h"

TCB* scheduler();

void activator();

void timer_interrupt(int sig);

void add_to_queue(TCB *t, struct queue *q);

TCB* get_from_queue(struct queue *q);

void mythread_sethungry(int value);

int mythread_gethungry();

void queue_iterate_exchange();

TCB* get_next(struct queue *q);

/* Array of state thread control blocks: the process allows a maximum of N threads */
static TCB t_state[N];
/* Current running thread */
static int current = 0;
/* Variable indicating if the library is initialized (init == 1) or not (init == 0) */
static int init=0;
/* Queue storing the threads with high priority */
static struct queue *high_priority_queue;
/* Queue storing the threads with low priority */
static struct queue *low_priority_queue;
/* Pointer to the process in execution*/
static TCB *executing;
/* Variable that indicates if a process suffering from starvation is in execution. 0 false, 1 true*/
static int starving_executing = 0;

TCB* get_next(struct queue *q){
  TCB *next = get_from_queue(q);
  return next;
}


/* iterate over the elements of low_priority_queue and put them into the
high priority queue if hungy equals 0.
*/
void queue_iterate_exchange() {
  struct my_struct* p = NULL;
  if(low_priority_queue) {
    if (queue_empty(low_priority_queue)) {
      return;
    } else {
      for(p = low_priority_queue->head; p; p = p->next) {
        TCB *data = p->data;
        data->hungry--;
        if (data->hungry == 0) {
          //remove data from low_priority_queue
          disable_interrupt();
          queue_find_remove(low_priority_queue, data);
          enable_interrupt();
          //insert data into high_priority_queue
          add_to_queue(data, high_priority_queue);
          data->hungry = STARVATION;
          printf("*** THREAD %d MOVED TO HIGH PRIORITY QUEUE\n", data->tid);
        }
      }
    }
  }
}

/** Function that enqueues a thread in a queue */
void add_to_queue(TCB *t, struct queue *q) {
      disable_interrupt();
      enqueue(q, t);
      enable_interrupt();
}

/** Function that dequeues a thread from the given queue */
TCB* get_from_queue(struct queue *q) {
    disable_interrupt();
    TCB *next = dequeue(q);
    enable_interrupt();
    return next;
}

/* Initialize the thread library */
void init_mythreadlib() {
  // launch the library thread
  int i;
  t_state[0].state = INIT;
  t_state[0].priority = LOW_PRIORITY;
  t_state[0].ticks = QUANTUM_TICKS;
  t_state[0].tid = 0;
  t_state[0].hungry = STARVATION;
  if(getcontext(&t_state[0].run_env) == -1){
    perror("getcontext in my_thread_create");
    exit(5);
  }
    // clean the array
  for(i=1; i<N; i++){
    t_state[i].state = FREE;
  }
  // initialize the queues
  high_priority_queue = queue_new();
  low_priority_queue = queue_new();
  executing = &t_state[0];
  init_interrupt();
}


/* Create and intialize a new thread with body fun_addr and one integer argument */
int mythread_create (void (*fun_addr)(), int priority)
{
  int i;

  if (!init) { init_mythreadlib(); init=1;}
  for (i=0; i<N; i++)
    if (t_state[i].state == FREE) break;
  if (i == N) return(-1);
  if(getcontext(&t_state[i].run_env) == -1) {
    perror("getcontext in my_thread_create");
    exit(-1);
  }
  t_state[i].state = INIT;
  t_state[i].priority = priority;
  t_state[i].function = fun_addr;
  t_state[i].ticks = QUANTUM_TICKS;
  t_state[i].tid = i;
  t_state[i].run_env.uc_stack.ss_sp = (void *)(malloc(STACKSIZE));
  t_state[i].hungry = STARVATION;
  if(t_state[i].run_env.uc_stack.ss_sp == NULL){
    printf("thread failed to get stack space\n");
    exit(-1);
  }
  t_state[i].run_env.uc_stack.ss_size = STACKSIZE;
  t_state[i].run_env.uc_stack.ss_flags = 0;
  makecontext(&t_state[i].run_env, fun_addr, 1);

  // add the thread to the relevant queue
  if (priority == LOW_PRIORITY) {
    add_to_queue(&t_state[i], low_priority_queue);
  } else {
    add_to_queue(&t_state[i], high_priority_queue);
  }
  //if the new thread has a higher priority, the context is swapped between the current thread and the new thread
  if(priority > executing->priority && !starving_executing) {
    // dequeue a high priority thread (the one just inserted)
    TCB *next = get_from_queue(high_priority_queue);
    // update the currently executing variable
    current = next->tid;
    // backup the executing thread reference for the change of context
    TCB *prev = executing;
    // update the executing thread reference
    executing = next;
    // make the change of context
    add_to_queue(prev, low_priority_queue);
    printf("*** THREAD %d EJECTED: SET CONTEXT OF %d\n", prev->tid, next->tid);
    swapcontext(&(prev->run_env), &(next->run_env));
  }
  return i;
} /****** End my_thread_create() ******/


/* Free terminated thread and exits */
void mythread_exit() {
  int tid = mythread_gettid();

  t_state[tid].state = FREE;
  free(t_state[tid].run_env.uc_stack.ss_sp);
  printf("*** THREAD %d FINISHED\n", tid);

  TCB* next = scheduler();
  // execute the next thread
  activator(next);
}

/* Sets the hungry value of the calling thread */
void mythread_sethungry(int value) {
  int tid = mythread_gettid();
  TCB* this_thread = &t_state[tid];
  this_thread->hungry = value;
}

/* Returns the hungry value of the calling thread */
int mythread_gethungry() {
  int tid = mythread_gettid();
  TCB* this_thread = &t_state[tid];
  return this_thread->hungry;
}

/* Sets the priority of the calling thread */
void mythread_setpriority(int priority) {
  int tid = mythread_gettid();
  t_state[tid].priority = priority;
}

/* Returns the priority of the calling thread */
int mythread_getpriority() {
  int tid = mythread_gettid();
  return t_state[tid].priority;
}

/* Get the current thread id.  */
int mythread_gettid(){
  if (!init) { init_mythreadlib(); init=1;}
  return current;
}

/* Timer interrupt  */
void timer_interrupt(int sig) {
  queue_iterate_exchange();
  // do nothing when a high priority process is executing
  if (executing->priority == HIGH_PRIORITY){ return; }
  // decrease the value of ticks otherwise
  executing->ticks--;
  // when the thread ended its quantum, call another thread to be executed
  if (executing->ticks <= 0) {
    // get the next thread to execute
    TCB *next = scheduler();
    // pass the thread to the activator to execute it
    activator(next);
  }
}

/* Scheduler: returns the next thread to be executed */
TCB* scheduler(){

  if (!queue_empty(high_priority_queue)) {
    TCB *next = get_next(high_priority_queue);

    // if the priority of thread obtained from high_priority_queue is LOW_PRIORITY, it's in starvation
    if (next->priority == LOW_PRIORITY) {
      starving_executing = 1;
    } else {
      starving_executing = 0;
    }
    return next;
  }

  // if the high priority queue is empty, check the low priority queue
  if (!queue_empty(low_priority_queue)) { //queue not empty
    starving_executing = 0;
    return get_next(low_priority_queue);
  }

  printf("FINISH\n");
  exit(1);
}

/* Activator */
void activator(TCB* next){
  // update variable current and change reference to executing
  TCB *prev = executing;
  current = next->tid;

  if (prev->state == FREE) {
    // the previous thread has finished
    printf("*** THREAD %d TERMINATED : SET CONTEXT OF %d\n", prev->tid, next->tid);
    executing = next;
    setcontext(&(next->run_env));
  } else { // state = INIT
    // reset the ticks counter of the executin thread
    prev->ticks = QUANTUM_TICKS;
    // save the previous thread in the queue
    if (prev->priority == LOW_PRIORITY) {
      add_to_queue(prev, low_priority_queue);
    } else {
      add_to_queue(prev, high_priority_queue);
    }
    printf("*** SWAPCONTEXT FROM %d TO %d\n", prev->tid, next->tid);
    executing = next;
    swapcontext(&(prev->run_env), &(next->run_env));
  }
}
