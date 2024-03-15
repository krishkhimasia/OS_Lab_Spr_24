#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#ifdef VERBOSE
int verbose=1;
#else
int verbose=0;
#endif

#ifdef SLEEP
int slep=1;
#else
int slep=0;
#endif


int main(){
    srand((unsigned int)time(NULL));
    int n,t;
    printf("Enter n: ");
    scanf("%d",&n);
    printf("Enter t: ");
    scanf("%d",&t);
    int shmid, status;
    int *m,*mc;
    shmid = shmget(IPC_PRIVATE, 2*sizeof(int), 0777|IPC_CREAT);
    int pid[n+1];
    for(int i=1;i<=n;i++){
        if((pid[i]=fork())){
            //producer
            if(i==1) printf("Producer is alive\n");
            if(i==n){
                m = (int *) shmat(shmid,0,0);
                m[0]=0;
                int items[n+1], checksum[n+1];
                for(int j=1;j<=n;j++){
                    items[j]=0;
                    checksum[j]=0;
                }
                for(int j=1;j<=t;j++){
                    int c=1+rand()%n;
                    int item=100+rand()%900;
                    items[c]++;
                    checksum[c]+=item;
                    // give verbose output if VERBOSE macro set
                    if(verbose){
                        printf("Producer produces %d for Consumer %d\n", item, c);
                    }
                    m[0]=c;
                    //sleep for 5 microseconds if SLEEP macro set
                    if(slep){
                        usleep(5);
                    }
                    m[1]=item;
                    while(m[0]!=0) {}
                }
                m[0]=-1;
                for(int j=1;j<=n;j++){
                    waitpid(pid[j],&status,WUNTRACED);
                }
                printf("Producer has produced %d items\n", t);
                for(int j=1;j<=n;j++){
                    printf("%d items for Consumer %d: Checksum = %d\n", items[j], j, checksum[j]);
                }
                shmdt(m);

                shmctl(shmid, IPC_RMID, 0);
                exit(0);
            }
        }
        else{
            //consumer
            printf("\t\t\t\t\t\tConsumer %d is alive\n", i);
            mc = (int *) shmat(shmid,0,0);
            int items=0,checksum=0;
            while(1){
                if(mc[0]==-1) break;
                if(mc[0]==i){
                    items++;
                    checksum+=mc[1];
                    // give verbose output if VERBOSE macro set
                    if(verbose){
                            printf("\t\t\t\t\t\tConsumer %d reads %d\n", i, mc[1]);
                        }
                    mc[0]=0;
                }
            }
            printf("\t\t\t\t\t\tConsumer %d has read %d items: Checksum = %d\n", i, items, checksum);
            shmdt(mc);
            exit(0);
        }
    }

    
}