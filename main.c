#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>

#include "mythread.h"


void fun1 (int global_index)
{
  int a=0, b=0;
  for (a=0; a<10; ++a) {
//    printf ("Thread %d with priority %d\t from fun2 a = %d\tb = %d\n", mythread_gettid(), mythread_getpriority(), a, b);
    for (b=0; b<25000000; ++b);
  }

  for (a=0; a<10; ++a) {
//    printf ("Thread %d with priority %d\t from fun2 a = %d\tb = %d\n", mythread_gettid(), mythread_getpriority(), a, b);
    for (b=0; b<25000000; ++b);
  }
  mythread_exit();
  return;
}


void fun2 (int global_index)
{
  int a=0, b=0;
  for (a=0; a<10; ++a) {
  //  printf ("Thread %d with priority %d\t from fun2 a = %d\tb = %d\n", mythread_gettid(), mythread_getpriority(), a, b);
    for (b=0; b<18000000; ++b);
  }
  for (a=0; a<10; ++a) {
  //  printf ("Thread %d with priority %d\t from fun2 a = %d\tb = %d\n", mythread_gettid(), mythread_getpriority(), a, b);
    for (b=0; b<18000000; ++b);
  }
  mythread_exit();
  return;
}

void fun3 (int global_index)
{
  int a=0, b=0;
  for (a=0; a<10; ++a) {
    //printf ("Thread %d with priority %d\t from fun2 a = %d\tb = %d\n", mythread_gettid(), mythread_getpriority(), a, b);
    for (b=0; b<40000000; ++b);
  }
  for (a=0; a<10; ++a) {
    //printf ("Thread %d with priority %d\t from fun2 a = %d\tb = %d\n", mythread_gettid(), mythread_getpriority(), a, b);
    for (b=0; b<40000000; ++b);
  }
  mythread_exit();
  return;
}

// This is supposed to be an edge case or something, so... 0 threads.
int test0(){
    init_mythreadlib();
    mythread_exit();
    printf("This program should never come here\n");
    return 0;
}

// Default execution
int test1(){
    int i,j,k,l,m,a,b=0;

    mythread_setpriority(LOW_PRIORITY);

    if((i = mythread_create(fun1,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }
    if((j = mythread_create(fun2,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }
    if((k = mythread_create(fun3,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }
    if((l = mythread_create(fun1,HIGH_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }

    if((m = mythread_create(fun2,HIGH_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }

    printf("thread IDs are %d %d %d %d %d\n", i,j,k,l,m);

    mythread_create(fun1,LOW_PRIORITY);
    mythread_create(fun1,LOW_PRIORITY);
    mythread_create(fun1,LOW_PRIORITY);

    for (a=0; a<10; ++a) {
    //    printf ("Thread %d with priority %d\t from fun2 a = %d\tb = %d\n", mythread_gettid(), mythread_getpriority(), a, b);
        for (b=0; b<30000000; ++b);
      }
      mythread_create(fun1,HIGH_PRIORITY);

    mythread_exit();
    printf("This program should never come here\n");
    return 0;
}

// Test for Round Robin, to test slices.
int test2(){
    int i,j,k,l,m,n,o,a,b=0;

    mythread_setpriority(LOW_PRIORITY);

    if((i = mythread_create(fun3,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }
    if((j = mythread_create(fun2,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }

    for (a=0; a<10; ++a) {
    //    printf ("Thread %d with priority %d\t from fun2 a = %d\tb = %d\n", mythread_gettid(), mythread_getpriority(), a, b);
        for (b=0; b<30000000; ++b);
    }

    if((k = mythread_create(fun3,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }
    if((l = mythread_create(fun2,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }

    for (a=0; a<10; ++a) {
    //    printf ("Thread %d with priority %d\t from fun2 a = %d\tb = %d\n", mythread_gettid(), mythread_getpriority(), a, b);
        for (b=0; b<30000000; ++b);
    }

    if((m = mythread_create(fun3,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }

    if((n = mythread_create(fun1,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }

    if((o = mythread_create(fun2,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }

    mythread_exit();
    printf("This program should never come here\n");
    return 0;
}

// Default execution, but thread 0 has high priority
int test3(){
    int i,j,k,l,m,a,b=0;

    mythread_setpriority(LOW_PRIORITY);

    if((i = mythread_create(fun1,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }
    if((j = mythread_create(fun2,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }
    if((k = mythread_create(fun3,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }
    if((l = mythread_create(fun1,HIGH_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }

    if((m = mythread_create(fun2,HIGH_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }

    printf("thread IDs are %d %d %d %d %d\n", i,j,k,l,m);

    mythread_create(fun1,LOW_PRIORITY);
    mythread_create(fun1,LOW_PRIORITY);
    mythread_create(fun1,LOW_PRIORITY);

    for (a=0; a<10; ++a) {
    //    printf ("Thread %d with priority %d\t from fun2 a = %d\tb = %d\n", mythread_gettid(), mythread_getpriority(), a, b);
        for (b=0; b<30000000; ++b);
      }
      mythread_create(fun1,HIGH_PRIORITY);

    mythread_exit();
    printf("This program should never come here\n");
    return 0;
}

// Long processes in Round Robin, short processes interrupting with high priority
int test4(){
    int a,b,c,d,e,f,g,h,i,j=0;

    mythread_setpriority(LOW_PRIORITY);

    if((a = mythread_create(fun3,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }
    if((b = mythread_create(fun3,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }
    if((c = mythread_create(fun3,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }
    if((d = mythread_create(fun3,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }

    if((e = mythread_create(fun2,HIGH_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }

    if((f = mythread_create(fun3,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }
    if((g = mythread_create(fun3,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }
    if((h = mythread_create(fun3,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }
    if((i = mythread_create(fun3,LOW_PRIORITY)) == -1){
      printf("thread failed to initialize\n");
      exit(-1);
    }

    mythread_exit();
    printf("This program should never come here\n");
    return 0;
}

int test5(){
  int a, b, c, d;

  mythread_setpriority(LOW_PRIORITY);
  if((a = mythread_create(fun2,LOW_PRIORITY)) == -1){
    printf("thread failed to initialize\n");
    exit(-1);
  }

  if((b = mythread_create(fun3,HIGH_PRIORITY)) == -1){
    printf("thread failed to initialize\n");
    exit(-1);
  }

  if((c = mythread_create(fun3,HIGH_PRIORITY)) == -1){
    printf("thread failed to initialize\n");
    exit(-1);
  }

  if((c = mythread_create(fun1,HIGH_PRIORITY)) == -1){
    printf("thread failed to initialize\n");
    exit(-1);
  }

  mythread_exit();
  return 0;

}


int main(int argc, char *argv[])
{
    if(argc == 1){
        test1();
    }
    else if (argc == 2){
        switch (atoi(argv[1])) {
            case 0: test0(); break;
            case 1: test1(); break;
            case 2: test2(); break;
            case 3: test3(); break;
            case 4: test4(); break;
            case 5: test5(); break;
            default : printf("There is no such test case.\n");
        }
    }
    else {
        printf("The input format is not valid. Please enter the desired test or no arguments.\n");
    }
    return 0;
} /****** End main() ******/
