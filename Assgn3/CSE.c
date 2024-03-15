#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define SIZEOFBUF 1000

int main(int argc, char *argv[]){
    if(argc<=1){
        printf("+++ CSE in supervisor mode: Started\n");
        // int in,out;
        int fd[2];
        pipe(fd);
        int swapfd[2];
        pipe(swapfd);
        // in=dup(0);      // new fd for stdin
        // out=dup(1);     // new fd for stdout
        printf("+++ CSE in supervisor mode: pfd = [%d %d]\n",fd[0],fd[1]);
        int pid1,pid2;
        if(pid1=fork()){
            printf("+++ CSE in supervisor mode: Forking first child in command-input mode\n");
            if(pid2=fork()){
                printf("+++ CSE in supervisor mode: Forking second child in execute mode\n");
                waitpid(pid2,NULL,WUNTRACED);
                printf("+++ CSE in supervisor mode: Second child terminated\n");
            }
            else{   // E
                char ins[2],outs[2],swin[2],swout[2];
                sprintf(ins,"%d",fd[0]);
                sprintf(outs,"%d",fd[1]);
                sprintf(swin,"%d",swapfd[0]);
                sprintf(swout,"%d",swapfd[1]);
                execlp("xterm","xterm","-T","Second Child","-e","./CSE","e",ins,outs,swin,swout,NULL);
                
            }
            waitpid(pid1,NULL,WUNTRACED);
            printf("+++ CSE in supervisor mode: First child terminated\n");
        }
        else{   // C
            char ins[2],outs[2],swin[2],swout[2];
            sprintf(ins,"%d",fd[0]);
            sprintf(outs,"%d",fd[1]);
            sprintf(swin,"%d",swapfd[0]);
            sprintf(swout,"%d",swapfd[1]);
            execlp("xterm","xterm","-T","First Child","-e","./CSE","c",ins,outs,swin,swout,NULL);
        }
    }
    else if(!strcmp(argv[1],"e")){
        char line[SIZEOFBUF];
        int ifd=atoi(argv[2]),ofd=atoi(argv[3]),iswfd=atoi(argv[4]),oswfd=atoi(argv[5]);
        close(ofd);
        close(iswfd);
        int infd=dup(0);
        int outfd=dup(1);
        int mode=1;
        while(1){
            if(mode==1){
                close(0);
                dup(ifd);
                close(1);
                dup(outfd);
                printf("Waiting for command> ");
                fflush(stdout);
                read(ifd,line,SIZEOFBUF);
                if(!strcmp(line,"exit")){
                    exit(0);
                }
                if(!strcmp(line,"swaprole")){
                    printf("swaprole\n");
                    mode=0;
                    continue;
                }
                printf("%s\n",line);
                fflush(stdout);
                char *args[100];

                // creating array of arguments for execvp
                char *token = strtok(line, " ");

                // number of words
                int count = 0;

                while (token != NULL) {
                    args[count] = token;
                    token = strtok(NULL, " ");
                    count++;
                }
                args[count] = NULL;
                
                int pid;
                if(pid=fork()){
                    waitpid(pid,NULL,WUNTRACED);
                }
                else
                {
                    close(0);
                    dup(infd);
                    close(1);
                    dup(outfd);
                    if(execvp(args[0],args)<0)
                    {
                        perror("execvp failed");
                        exit(1);
                    }
                }
            }
            else{
                close(0);
                dup(infd);
                close(1);
                dup(oswfd);
                fprintf(stderr,"Enter command> ", 16);

                for(int i=0;i<SIZEOFBUF;i++){
                    char t;
                    scanf("%c",&t);
                    if(t=='\n') {
                        line[i]='\0';
                        break;
                    }
                    line[i]=t;
                }
                write(oswfd,line,SIZEOFBUF);
                fflush(stdout);
                if(!strcmp(line,"exit")){
                    // close()
                    exit(0);
                }
                if(!strcmp(line,"swaprole")){
                    mode=1;
                    continue;
                }
            }
        }
    }
    else{
        char line[SIZEOFBUF];
        int ifd=atoi(argv[2]),ofd=atoi(argv[3]),iswfd=atoi(argv[4]),oswfd=atoi(argv[5]);
        close(ifd);
        close(oswfd);
        int infd=dup(0);
        int outfd=dup(1);
        int mode=0;
        while(1){
            if(mode==0)
            {
                close(0);
                dup(infd);
                close(1);
                dup(ofd);
                fprintf(stderr,"Enter command> ", 16);

                for(int i=0;i<SIZEOFBUF;i++){
                    char t;
                    scanf("%c",&t);
                    if(t=='\n') {
                        line[i]='\0';
                        break;
                    }
                    line[i]=t;
                }
                write(ofd,line,SIZEOFBUF);
                fflush(stdout);
                if(!strcmp(line,"exit")){
                    // close()
                    exit(0);
                }
                if(!strcmp(line,"swaprole")){
                    mode=1;
                    continue;
                }
            }
            else
            {
                close(0);
                dup(iswfd);
                close(1);
                dup(outfd);
                printf("Waiting for command> ");
                fflush(stdout);
                read(iswfd,line,SIZEOFBUF);
                if(!strcmp(line,"exit")){
                    exit(0);
                }
                if(!strcmp(line,"swaprole")){
                    printf("swaprole\n");
                    mode=0;
                    continue;
                }
                printf("%s\n",line);
                fflush(stdout);
                char *args[100];

                // creating array of arguments for execvp
                char *token = strtok(line, " ");

                // number of words
                int count = 0;

                while (token != NULL) {
                    args[count] = token;
                    token = strtok(NULL, " ");
                    count++;
                }
                args[count] = NULL;
                
                int pid;
                if(pid=fork()){
                    waitpid(pid,NULL,WUNTRACED);
                }
                else
                {
                    close(0);
                    dup(infd);
                    close(1);
                    dup(outfd);
                    if(execvp(args[0],args)<0){
                        perror("execvp failed");
                        exit(1);
                    }
                }
            }
        }
    }
    exit(0);
}