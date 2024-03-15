#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define P(s) semop(s, &pop, 1)  /* pop is the structure we pass for doing
				   the P(s) operation */
#define V(s) semop(s, &vop, 1)  /* vop is the structure we pass for doing
				   the V(s) operation */

int main(){

    int n;
    FILE *fp=fopen("graph.txt","r");
    fscanf(fp,"%d",&n);
    int *A;     // adjacency matrix
    int *T;       // topological listing
    int *idx;

    int shmidA, shmidT, shmididx;
    key_t keyA, keyT, keyidx;

    keyA = ftok("graph.txt", 65);
    keyT = ftok("graph.txt", 66);
    keyidx = ftok("graph.txt", 67);

    shmidA = shmget(keyA, n*n*sizeof(int), 0777|IPC_CREAT);
    shmidT = shmget(keyT, n*sizeof(int), 0777|IPC_CREAT);
    shmididx = shmget(keyidx, sizeof(int), 0777|IPC_CREAT);

    int semidmtx, semidsync, semidntfy;
    struct sembuf pop, vop ;
    key_t keymtx, keysync, keyntfy;

    keymtx = ftok("graph.txt", 68);
    keysync = ftok("graph.txt", 69);
    keyntfy = ftok("graph.txt", 70);

    semidmtx = semget(keymtx, 1, 0777|IPC_CREAT);
    semidsync = semget(keysync, n, 0777|IPC_CREAT);
    semidntfy = semget(keyntfy, 1, 0777|IPC_CREAT);

    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1;

    //initialising A, T, idx
    A = (int *)shmat(shmidA, 0, 0);
    memset(A,0,n*n*sizeof(int));
    for(int i=0;i<n;i++){
        for(int j=0;j<n;j++){
            fscanf(fp,"%d",&A[n*i+j]);
        }
    }

    T = (int *)shmat(shmidT, 0, 0);
    memset(T,0,n*sizeof(int));

    idx = (int *)shmat(shmididx, 0, 0);
    memset(idx,0,sizeof(int));
    *idx=0;

    // initialising semaphores
    semctl(semidmtx, 0, SETVAL, 1);
    
    for(int j=0;j<n;j++){
        semctl(semidsync, j, SETVAL, 0);
    }

    semctl(semidntfy, 0, SETVAL, 0);

    printf("+++ Boss: Setup done...\n");

    // waiting on all workers to finish
    for(int i=0;i<n;i++){
        P(semidntfy);
    }

    // print T, check contents and figure out if valid topoligical listing
    printf("+++ Topological sorting of the vertices\n");
    for(int i=0;i<n;i++){
        printf("%d ",T[i]);
    }

    int f=1;
    for(int i=0;i<n;i++){
        for(int j=i+1;j<n;j++){
            if(A[n*T[j]+T[i]]){
                f=0;
                break;
            }
        }
    }
    if(f){
        printf("\n+++ Boss: Well done, my team...\n");
    }
    else{
        printf("\n+++ Boss: All you had to do, was sort the damn T topologically, CJ\n");
    }

    // remove semaphores and shared memory
    shmdt(A);
    shmdt(T);
    shmdt(idx);
    shmctl(shmidA, IPC_RMID, 0);
    shmctl(shmidT, IPC_RMID, 0);
    shmctl(shmididx, IPC_RMID, 0);
    semctl(semidmtx, 0, IPC_RMID, 0);
    semctl(semidsync, 0, IPC_RMID, 0);
    semctl(semidntfy, 0, IPC_RMID, 0);
    
    exit(0);
}