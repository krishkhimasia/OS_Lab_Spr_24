#include<foothread.h>
#include<stdio.h>
#include<stdlib.h>

int leadertid=-1;
int firstrun=0;
int numOfJoinableThreads=0;
int glob_semid;
struct sembuf glob_sembuf;
foothread_t table[FOOTHREAD_THREADS_MAX];
int jointype[FOOTHREAD_THREADS_MAX];
int tableindex=0;
int keygen=0;
foothread_mutex_t glob_mutex, table_mutex;

void foothread_create (foothread_t * thread, foothread_attr_t * attr, int (*start_routine)(void *), void * arg){
    if(firstrun==0){
        leadertid=gettid();
        firstrun=1;
        numOfJoinableThreads=0;
        table[0].tid=leadertid;
        table[0].pid=getpid();
        table[0].ppid=getppid();
        jointype[0]=FOOTHREAD_JOINABLE;
        tableindex=1;
        glob_semid=semget(IPC_PRIVATE, 1, IPC_CREAT|0666);
        semctl(glob_semid, 0, SETVAL, 1);
        foothread_mutex_init(&glob_mutex);
        foothread_mutex_init(&table_mutex);
    }
    foothread_mutex_lock(&table_mutex);
    if(thread==NULL){
        printf("foothread_create: thread is NULL\n");
        return;
    }
    if(attr==NULL){
        attr=(foothread_attr_t *)malloc(sizeof(foothread_attr_t));
        *attr=FOOTHREAD_ATTR_INITIALIZER;
    }
    if(attr->stacksize==0){
        attr->stacksize=FOOTHREAD_DEFAULT_STACK_SIZE;
    }
    if(attr->jointype!=FOOTHREAD_JOINABLE && attr->jointype!=FOOTHREAD_DETACHED){
        printf("foothread_create: jointype is not valid\n");
        return;
    }
    thread->pid=getpid();
    thread->ppid=getppid();
    thread->tid=clone(start_routine, (void *)malloc(attr->stacksize)+attr->stacksize, CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD, arg);
    if(thread->tid==-1){
        printf("foothread_create: thread creation failed\n");
        return;
    }
    if(attr->jointype==FOOTHREAD_JOINABLE)
        numOfJoinableThreads++;
    table[tableindex].tid=thread->tid;
    table[tableindex].pid=thread->pid;
    table[tableindex].ppid=thread->ppid;
    jointype[tableindex]=attr->jointype;
    tableindex++;
    foothread_mutex_unlock(&table_mutex);
}

void foothread_attr_setjointype ( foothread_attr_t * attr, int jointype){
    if(attr==NULL){
        printf("foothread_attr_setjointype: attr is NULL\n");
        return;
    }
    if(jointype==FOOTHREAD_JOINABLE || jointype==FOOTHREAD_DETACHED)
        attr->jointype=jointype;
    else
        printf("foothread_attr_setjointype: jointype is not valid\n");
}

void foothread_attr_setstacksize ( foothread_attr_t * attr, int stacksize){
    if(attr==NULL){
        printf("foothread_attr_setstacksize: attr is NULL\n");
        return;
    }
    attr->stacksize=stacksize;
}

void foothread_exit(){
    if(leadertid==-1){
        printf("foothread_exit: no threads to exit\n");
        return;
    }
    if(leadertid==gettid()){
        struct sembuf sembuf;
        sembuf.sem_num=0;
        sembuf.sem_op=-numOfJoinableThreads;
        sembuf.sem_flg=0;
        semop(glob_semid, &sembuf, 1);

        // destroying resources
        semctl(glob_semid, 0, IPC_RMID);
        foothread_mutex_destroy(&glob_mutex);
        foothread_mutex_destroy(&table_mutex);
    }
    else{
        int i=1;
        for(i=1;i<tableindex;i++){
            if(table[i].tid==gettid()){
                break;
            }
        }
        if(jointype[i]==FOOTHREAD_DETACHED){
            return;
        }
        struct sembuf sembuf;
        sembuf.sem_num=0;
        sembuf.sem_op=1;
        sembuf.sem_flg=0;
        semop(glob_semid, &sembuf, 1);
    }
}

void foothread_mutex_init ( foothread_mutex_t * mutex){
    if(mutex==NULL){
        printf("foothread_mutex_init: mutex is NULL\n");
        exit(1);
    }
    if(keygen==0){
        keygen='A';
    }
    mutex->key=ftok("/tmp", keygen++);
    mutex->semid=semget(mutex->key, 1, IPC_CREAT|0666);
    mutex->semval=1;
    mutex->semflg=0;
    mutex->lockedBytid=gettid();        // some initial value, will be overwritten when locked
    mutex->isLocked=0;
    mutex->init=1;
    semctl(mutex->semid, 0, SETVAL, mutex->semval);
}

void foothread_mutex_lock ( foothread_mutex_t * mutex){
    if(mutex==NULL){
        printf("foothread_mutex_lock: mutex is NULL\n");
        exit(1);
    }
    if(mutex->init!=1){
        printf("foothread_mutex_lock: mutex is not initialized\n");
        exit(1);
    }
    mutex->sembuf.sem_num=0;
    mutex->sembuf.sem_op=-1;
    mutex->sembuf.sem_flg=0;
    semop(mutex->semid, &mutex->sembuf, 1);
    mutex->isLocked=1;
    mutex->lockedBytid=gettid();
}

void foothread_mutex_unlock ( foothread_mutex_t * mutex){
    if(mutex==NULL){
        printf("foothread_mutex_unlock: mutex is NULL\n");
        exit(1);
    }
    if(mutex->lockedBytid!=gettid()){
        printf("foothread_mutex_unlock: thread did not lock the mutex\n");
        exit(1);
    }
    if(mutex->init!=1){
        printf("foothread_mutex_lock: mutex is not initialized\n");
        exit(1);
    }
    if(mutex->isLocked==0){
        printf("foothread_mutex_unlock: mutex is not locked\n");
        exit(1);
    }
    mutex->sembuf.sem_num=0;
    mutex->sembuf.sem_op=1;
    mutex->sembuf.sem_flg=0;
    mutex->isLocked=0;
    semop(mutex->semid, &mutex->sembuf, 1);
}

void foothread_mutex_destroy ( foothread_mutex_t * mutex){
    if(mutex==NULL){
        printf("foothread_mutex_destroy: mutex is NULL\n");
        exit(1);
    }
    if(mutex->init!=1){
        printf("foothread_mutex_lock: mutex is not initialized\n");
        exit(1);
    }
    semctl(mutex->semid, 0, IPC_RMID);
}

void foothread_barrier_init ( foothread_barrier_t * barrier, int count){
    if(barrier==NULL){
        printf("foothread_barrier_init: barrier is NULL\n");
        exit(1);
    }
    if(count<1){
        printf("foothread_barrier_init: count is less than 1\n");
        exit(1);
    }
    if(!glob_mutex.init) foothread_mutex_init(&glob_mutex);
    foothread_mutex_lock(&glob_mutex);
    barrier->init=1;
    barrier->count=count;
    barrier->n=count;
    barrier->tid=gettid();
    if(keygen==0){
        keygen='A';
    }
    barrier->key=ftok("/tmp", keygen++);
    barrier->semid=semget(IPC_PRIVATE, 1, IPC_CREAT|0666);
    barrier->semflg=0;
    semctl(barrier->semid, 0, SETVAL, 0);
    foothread_mutex_unlock(&glob_mutex);
}

void foothread_barrier_wait ( foothread_barrier_t * barrier){
    if(barrier==NULL){
        printf("foothread_barrier_wait: barrier is NULL\n");
        exit(1);
    }
    if(barrier->init!=1){
        printf("foothread_barrier_wait: barrier is not initialized\n");
        exit(1);
    }
    struct sembuf sembuf;
    sembuf.sem_num=0;
    sembuf.sem_flg=0;
    foothread_mutex_lock(&glob_mutex);
    if(barrier->n==1){
        foothread_mutex_unlock(&glob_mutex);
        return;
    }
    if(barrier->count==1){
        sembuf.sem_op=barrier->n-1;
        foothread_mutex_unlock(&glob_mutex);
        semop(barrier->semid, &sembuf, 1);
        foothread_mutex_lock(&glob_mutex);
        barrier->count=barrier->n;
        foothread_mutex_unlock(&glob_mutex);
    }
    else{
        sembuf.sem_op=-1;
        barrier->count--;
        foothread_mutex_unlock(&glob_mutex);
        semop(barrier->semid, &sembuf, 1);
    }
}

void foothread_barrier_destroy ( foothread_barrier_t * barrier){
    if(barrier==NULL){
        printf("foothread_barrier_destroy: barrier is NULL\n");
        exit(1);
    }
    if(barrier->init!=1){
        printf("foothread_barrier_wait: barrier is not initialized\n");
        exit(1);
    }
    semctl(barrier->semid, 0, IPC_RMID);
}