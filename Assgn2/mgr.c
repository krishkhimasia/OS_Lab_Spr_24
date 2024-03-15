#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

typedef struct Rec
{
    int pid, pgid, status;
    char arg;
} rec;

int currJob;
int running;
int sz,sus;
rec PT[11];

void sigHandler ( int sig )
{
    // if child has finished, nothing, else change status of child
    if(running)
    {
        if (sig == SIGINT) {
            kill(currJob,SIGINT);
            running=0;
            for(int i=0;i<sz;i++)
            {
                if(PT[i].pid==currJob){
                    PT[i].status=2;
                }
            }
        } else if (sig == SIGTSTP) {
            sus++;
            kill(currJob,SIGTSTP);
            running=0;
            for(int i=0;i<sz;i++)
            {
                if(PT[i].pid==currJob){
                    PT[i].status=3;
                }
            }
        }
        printf("\n");
    }
    else{
        if(sig==SIGTSTP || sig==SIGINT)
        printf("\nmgr> ");
    }
}

int main()
{
    srand((unsigned int)time(NULL));
    currJob=0;
    char ch = 'h';
    char sta[5][11] = {"SELF", "FINISHED", "TERMINATED", "SUSPENDED", "KILLED"};
    sz = 1;
    sus=0;
    signal(SIGINT,sigHandler);
    signal(SIGTSTP,sigHandler);
    int status;
    PT[0].status = 0;
    PT[0].pid = getpid();
    PT[0].pgid = getpgid(PT[0].pid);
    while (1)
    {
        printf("mgr> ");
        scanf("%c", &ch);
        if (ch == 'c')
        {
            if(sus){
                printf("Suspended Jobs: ");
                for(int i=1;i<sz;i++)
                {
                    if(PT[i].status==3){
                        printf("%d, ",i);
                    }
                }
                printf("(Pick one): ");
                int choice;
                scanf("%d",&choice);
                currJob=PT[choice].pid;
                PT[choice].status=1;
                running=1;
                sus--;
                kill(PT[choice].pid,SIGCONT);
                waitpid(PT[choice].pid,&status,WUNTRACED);
                running=0;
            }
        }
        else if (ch == 'h')
        {
            printf("\tCommand\t:\tAction\t\n");
            printf("\t   c   \t:\tContinue a suspended job\n");
            printf("\t   h   \t:\tPrint this help message\n");
            printf("\t   k   \t:\tKill a suspended job\n");
            printf("\t   p   \t:\tPrint the process table\n");
            printf("\t   q   \t:\tQuit\n");
            printf("\t   r   \t:\tRun a new job\n");
        }
        else if (ch == 'k')
        {
            if(sus){
                printf("Suspended Jobs: ");
                for(int i=1;i<sz;i++)
                {
                    if(PT[i].status==3){
                        printf("%d, ",i);
                    }
                }
                printf("(Pick one): ");
                int choice;
                scanf("%d",&choice);
                PT[choice].status=4;
                kill(PT[choice].pid,SIGKILL);
                sus--;
            }
        }
        else if (ch == 'p')
        {
            printf("NO\tPID\tPGID\tSTATUS\t\t\tNAME\n");
            for (int i = 0; i < sz; i++)
            {
                printf("%d\t%d\t%d\t%s\t\t", i, PT[i].pid, PT[i].pgid, sta[PT[i].status]);
                if(PT[i].status==4 || PT[i].status==0) printf("\t");
                if (PT[i].status){
                    printf("job %c\n", PT[i].arg);}
                else
                    printf("mgr\n");
            }
        }
        else if (ch == 'q')
        {
            exit(0);
        }
        else if (ch == 'r')
        {
            if(sz>=11){
                printf("Process Table is full. Quitting...\n");
                exit(0);
            }
            int cpid;
            char argu='A'+rand()%26;
            if(cpid=fork()){
                PT[sz].pid=cpid;
                PT[sz].pgid=cpid;
                PT[sz].status=1;
                PT[sz].arg=argu;
                sz++;
                signal(SIGINT,sigHandler);
                signal(SIGTSTP,sigHandler);
                running=1;
                currJob=cpid;
                waitpid(cpid,&status,WUNTRACED);
                running=0;
            }
            else{
                char random[2]={argu,'\0'};
                setpgid(getpid(),0);
                execlp("./job","./job",random,NULL);
            }
        }
        else
        {
            printf("Enter valid input!\n");
        }
        char buff;
        scanf("%c",&buff);
    }
}