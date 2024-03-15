#include<stdio.h>
#include<stdlib.h>
#include<foothread.h>

int *sum, *numOfChildren, *P;
foothread_barrier_t *barrier;

void node(void *arg){
    int i=*(int *)arg;
    if(numOfChildren[i]){
        foothread_barrier_wait(&barrier[i]);
        printf("Internal node   %d gets the partial sum %d from its children\n",i,sum[i]);
    }
    if(P[i]!=-1){
        sum[P[i]]+=sum[i];
        foothread_barrier_wait(&barrier[P[i]]);
    }
    foothread_exit();
}

int main(){
    FILE *fp=fopen("tree.txt","r");
    int n;
    int root=-1;
    fscanf(fp,"%d",&n);
    numOfChildren=(int *)malloc(n*sizeof(int));
    P=(int *)malloc(n*sizeof(int));
    sum=(int *)malloc(n*sizeof(int));
    barrier=(foothread_barrier_t *)malloc(n*sizeof(foothread_barrier_t));
    for(int i=0;i<n;i++){
        P[i]=-1;
        numOfChildren[i]=0;
    }
    int parent,child;
    while(fscanf(fp,"%d %d",&child,&parent)!=EOF){
        if(child==parent){
            root=child;
            continue;
        }
        P[child]=parent;
        numOfChildren[parent]++;
    }

    for(int i=0;i<n;i++){
        foothread_barrier_init(&barrier[i],numOfChildren[i]+1);
    }

    for(int i=0;i<n;i++){
        if(numOfChildren[i]==0){
            printf("Leaf node   %d :: Enter a positive integer: ",i);
            scanf("%d",&sum[i]);   
        }
    }

    for(int i=0;i<n;i++){
        foothread_t thread;
        int *info=(int *)malloc(sizeof(int));
        *info=i;
        foothread_create(&thread,NULL,(void *)node,(void *)info);
    }
    foothread_exit();
    printf("Sum at root (node %d) = %d\n",root,sum[root]);

    // cleanup
    free(sum);
    free(numOfChildren);
    free(P);
    for(int i=0;i<n;i++){
        foothread_barrier_destroy(&barrier[i]);
    }
    free(barrier);
    fclose(fp);

    return 0;
}