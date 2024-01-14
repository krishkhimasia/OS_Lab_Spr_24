#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>

int main(int argc, char *argv[])
{
    if(argc==1){
        printf("Run with a node name.\n");
        exit(0);
    }
    int lvl;
    if(argc==2){
        lvl=0;
    }
    else{
        lvl=atoi(argv[2]);
    }
    
    int status,n;
    FILE *fptr=fopen("treeinfo.txt","r");
    char cmp[1010];

    while(fscanf(fptr,"%s",cmp)!=EOF)
    {
        if(!strcmp(argv[1],cmp)){
            if(lvl)
            {
                for(int i=0;i<lvl;i++) printf("\t");
            }
            printf("%s (%d)\n",argv[1],getpid());
            fscanf(fptr,"%d",&n);
            while(n--)
            {
                char child[100];
                fscanf(fptr,"%s",child);
                int cpid;
                if((cpid=fork())){
                    waitpid(cpid,&status,0);
                }
                else{
                    char level[10];
                    sprintf(level,"%d",lvl+1);
                    execlp("./proctree","./proctree",child,level,NULL);
                }
            }
            exit(0);
        }
        else{
            fgets(cmp,1000,fptr);
        }
    }
    printf("City %s not found.\n",argv[1]);
    exit(0);
}