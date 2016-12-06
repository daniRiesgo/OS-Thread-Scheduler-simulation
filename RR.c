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

void add_to_queue(TCB *t);

TCB* get_from_queue();

/* Array of state thread control blocks: the process allows a maximum of N threads */
static TCB t_state[N];
/* Current running thread */
static int current = 0;
/* Variable indicating if the library is initialized (init == 1) or not (init == 0) */
static int init=0;
/* Ready Queue */
static struct queue *ready_queue;
/* reference to the executing thread */
static TCB* executing;

/* Method to securely add a thread to the ready queue */
void add_to_queue(TCB *t){
  disable_interrupt();
  enqueue(ready_queue, t);
  enable_interrupt();
}

/* Method to securely dequeue a thread from the ready queue */
TCB* get_from_queue() {
    disable_interrupt();
    TCB *next = dequeue(ready_queue);
    enable_interrupt();
    return next;
}

/* Initialize the thread library */
void init_mythreadlib() {
  int i;
  t_state[0].state = INIT;
  t_state[0].priority = LOW_PRIORITY;
  t_state[0].ticks = QUANTUM_TICKS;
  t_state[0].tid = 0;
  if(getcontext(&t_state[0].run_env) == -1){
    perror("getcontext in my_thread_create");
    exit(5);
  }
  for(i=1; i<N; i++){
    t_state[i].state = FREE;
  }
  // Initialize the queue
  ready_queue = queue_new();
  // Initialize our 'executing' reference with the lib thread
  executing = &t_state[0];
  // init the interruptions
  init_interrupt();
}


/* Create and intialize a new thread with body fun_addr and one integer argument */
int mythread_create (void (*fun_addr)(),int priority)
{
  int i;

  if (!init) { init_mythreadlib(); init=1;}
  for (i=0; i<N; i++)
    if (t_state[i].state == FREE) break;
  if (i == N) return(-1);
  if(getcontext(&t_state[i].run_env) == -1){
    perror("getcontext in my_thread_create");
    exit(-1);
  }
  t_state[i].state = INIT;
  t_state[i].priority = priority;
  t_state[i].function = fun_addr;
  // initially set ticks left to QUANTUM_TICKS
  t_state[i].ticks = QUANTUM_TICKS;
  t_state[i].tid = i;
  t_state[i].run_env.uc_stack.ss_sp = (void *)(malloc(STACKSIZE));
  if(t_state[i].run_env.uc_stack.ss_sp == NULL){
    printf("thread failed to get stack space\n");
    exit(-1);
  }
  t_state[i].run_env.uc_stack.ss_size = STACKSIZE;
  t_state[i].run_env.uc_stack.ss_flags = 0;
  makecontext(&t_state[i].run_env, fun_addr, 1);
  // add the thread to the ready queue
  add_to_queue(&t_state[i]);
  return i;
} /****** End my_thread_create() ******/


/* Free terminated thread and exits */
void mythread_exit() {
  int tid = mythread_gettid();

  printf("*** THREAD %d FINISHED\n", tid);
  t_state[tid].state = FREE;
  free(t_state[tid].run_env.uc_stack.ss_sp);

  TCB* next = scheduler();
  if (next != NULL) { // this shouldn't be NULL, but you know, it could happen
    // execute the next thread
    activator(next);
  } else {perror("Bad return from scheduler\n"); exit(-1);}
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
void timer_interrupt(int sig)
{
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
  // check the ready queue
  if (!queue_empty(ready_queue)) { // when the ready queue isn't empty
    // dequeue a ready thread
    TCB *next = get_from_queue(ready_queue);
    // pass it to the calling thread to be activated
    return next;
  }

  // if the queue is empty, it means that all threads finished execution.
  printf("FINISH\n");
  exit(1);
}

/* Activator */
void activator(TCB* next){
  // update variable current
  TCB *prev = executing;
  current = next->tid;
  if (prev->state == FREE) {
    // the previous thread has finished
    printf("*** THREAD %d TERMINATED : SET CONTEXT OF %d\n", executing->tid, next->tid);
    executing = next;
    setcontext(&(next->run_env));
  } else { // state = INIT: ICC
    // reset the quantum of the leaving thread
    executing->ticks = QUANTUM_TICKS;
    // save the leaving thread in the queue
    add_to_queue(executing);
    printf("*** SWAPCONTEXT FROM %d TO %d\n", executing->tid, next->tid);
    executing = next;
    swapcontext(&(prev->run_env), &(next->run_env));
  }
}
