#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define P(s) semop(s, &pop, 1)  /* pop is the structure we pass for doing
				   the P(s) operation */
#define V(s) semop(s, &vop, 1)  /* vop is the structure we pass for doing
				   the V(s) operation */

int main(int argc, char *argv[]){
    if(argc<3){
        printf("Usage: %s <n> <id>\n",argv[0]);
        exit(1);
    }
    int n, i;
    n = atoi(argv[1]);
    i = atoi(argv[2]);

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
    struct sembuf pop, vop;
    struct sembuf *popw, *vopw;         // array of sembufs, one for each workers
    popw = (struct sembuf *)malloc(n*sizeof(struct sembuf));
    vopw = (struct sembuf *)malloc(n*sizeof(struct sembuf));
    for(int j=0;j<n;j++){
        popw[j].sem_num = vopw[j].sem_num = j;
        popw[j].sem_flg = vopw[j].sem_flg = 0;
        popw[j].sem_op = -1 ; vopw[j].sem_op = 1;
    }
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
    T = (int *)shmat(shmidT, 0, 0);
    idx = (int *)shmat(shmididx, 0, 0);

    // waiting for all modules i depends on
    for(int j=0;j<n;j++){
        if(A[n*j+i]){
            semop(semidsync, &popw[j], 1);
        }
    }

    // updating T and idx
    P(semidmtx);
    T[*idx]=i;
    *idx = *idx + 1;
    V(semidmtx);

    // notifying all modules dependent on i
    for(int j=0;j<n;j++){
        if(A[n*i+j]){
            semop(semidsync, &vopw[i], 1);
        }
    }

    // notifying the boss
    V(semidntfy);

    shmdt(A);
    shmdt(T);
    shmdt(idx);
    
    exit(0);
}