#ifndef FOO_THREAD_H
#define FOO_THREAD_H

#define _GNU_SOURCE
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<sched.h>

#define FOOTHREAD_THREADS_MAX 100
#define FOOTHREAD_DEFAULT_STACK_SIZE 2097152
#define FOOTHREAD_JOINABLE 0
#define FOOTHREAD_DETACHED 1
#define FOOTHREAD_ATTR_INITIALIZER (foothread_attr_t){FOOTHREAD_JOINABLE, FOOTHREAD_DEFAULT_STACK_SIZE}

// thread creation
typedef struct foothread_attr_t{
    int jointype;
    int stacksize;
}foothread_attr_t;


typedef struct foothread_t{
    pid_t pid;
    pid_t ppid;
    int tid;
}foothread_t;


void foothread_create (foothread_t * ,foothread_attr_t * ,int (*)(void *) ,void * );
void foothread_attr_setjointype ( foothread_attr_t * , int );
void foothread_attr_setstacksize ( foothread_attr_t * , int );

// thread termination
void foothread_exit();

// mutexes
typedef struct foothread_mutex_t{
    int init;
    int semid;
    struct sembuf sembuf;
    key_t key;
    int semval;
    int semflg;
    int lockedBytid;
    int isLocked;
}foothread_mutex_t;

void foothread_mutex_init ( foothread_mutex_t * );
void foothread_mutex_lock ( foothread_mutex_t * );
void foothread_mutex_unlock ( foothread_mutex_t * );
void foothread_mutex_destroy ( foothread_mutex_t * );

// barriers
typedef struct foothread_barrier_t{
    int init;
    int count;
    int n;
    int tid;
    int semid;
    struct sembuf sembuf;
    key_t key;
    int semflg;
    
}foothread_barrier_t;

void foothread_barrier_init ( foothread_barrier_t * , int );
void foothread_barrier_wait ( foothread_barrier_t * );
void foothread_barrier_destroy ( foothread_barrier_t * );

#endif