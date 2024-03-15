#include <stdio.h>
#include <stdlib.h>
#include <string.h>   /* Needed for strcpy() */
#include <time.h>     /* Needed for time() to seed the RNG */
#include <unistd.h>   /* Needed for sleep() and usleep() */
#include <pthread.h>  /* Needed for all pthread library calls */
#include <event.h>

struct thread_arg{
    int num;
    int duration;
}thread_arg;

pthread_mutex_t mtx_assistant;
pthread_mutex_t mtx_doctor;
pthread_mutex_t mtx_reporter;
pthread_mutex_t mtx_salesrep;
pthread_mutex_t mtx_patient;

pthread_cond_t doctor;
pthread_cond_t patient;
pthread_cond_t reporter;
pthread_cond_t salesrep;

pthread_barrier_t doctor_barrier, patient_barrier, reporter_barrier, salesrep_barrier;

int end_of_service=0;
int waiting_reporters=0;
int waiting_patients=0;
int waiting_salesrep=0;
int asleep=0;
int turn_patient=0;
int turn_reporter=0;
int turn_salesrep=0;

int end_of_service;
int docHasArrived;

int currTime;
int currEventDuration;

void printTime(char *time, int minutesFrom9AM) {
    int totalMinutes = 9 * 60 + minutesFrom9AM; // Convert 9 AM to minutes and add the input minutes
    int hours = totalMinutes / 60;
    int minutes = totalMinutes % 60;
    char meridiem[3];

    if (hours < 12) {
        strcpy(meridiem, "am");
        if (hours == 0) {
            hours = 12;
        }
    } else {
        strcpy(meridiem, "pm");
        if (hours > 12) {
            hours -= 12;
        }
    }
    sprintf(time, "%02d:%02d%s", hours, minutes, meridiem);
}

void *tdoctor(void *arg){
    while(1){
        char t[8];
        pthread_mutex_lock(&mtx_doctor);
        while(asleep)
            pthread_cond_wait(&doctor, &mtx_doctor);
        asleep=1;
        printTime(t, currTime);
        if(end_of_service){
            printf("[%s] Doctor leaves\n", t);
            pthread_mutex_unlock(&mtx_doctor);
            pthread_barrier_wait(&doctor_barrier);
            pthread_exit(NULL);
        }
        printf("[%s] Doctor has next visitor\n", t);
        pthread_mutex_unlock(&mtx_doctor);
        pthread_barrier_wait(&doctor_barrier);
    }
}

void *treporter(void *arg){
    char t1[8], t2[8];
    struct thread_arg *arg1 = (struct thread_arg *)arg;
    int num=arg1->num;
    int duration=arg1->duration;
    pthread_mutex_lock(&mtx_reporter);
    while(turn_reporter!=num)
        pthread_cond_wait(&reporter, &mtx_reporter);
    printTime(t1,currTime);
    printTime(t2,currTime+duration);
    printf("[%s - %s] Reporter %d is in doctor's chamber\n", t1, t2, num);
    currEventDuration=duration;
    free(arg1);
    pthread_mutex_unlock(&mtx_reporter);
    pthread_barrier_wait(&reporter_barrier);

    pthread_exit(NULL);
}

void *tpatient(void *arg){
    char t1[8], t2[8];
    struct thread_arg *arg1 = (struct thread_arg *)arg;
    int num=arg1->num;
    int duration=arg1->duration;
    pthread_mutex_lock(&mtx_patient);
    while(turn_patient!=num)
        pthread_cond_wait(&patient, &mtx_patient);
    printTime(t1,currTime);
    printTime(t2,currTime+duration);
    printf("[%s - %s] Patient %d is in doctor's chamber\n", t1, t2, num);
    currEventDuration=duration;
    free(arg1);
    pthread_mutex_unlock(&mtx_patient);
    pthread_barrier_wait(&patient_barrier);

    pthread_exit(NULL);
}

void *tsalesrep(void *arg){
    char t1[8], t2[8];
    struct thread_arg *arg1 = (struct thread_arg *)arg;
    int num=arg1->num;
    int duration=arg1->duration;
    pthread_mutex_lock(&mtx_salesrep);
    while(turn_salesrep!=num)
        pthread_cond_wait(&salesrep, &mtx_salesrep);
    printTime(t1,currTime);
    printTime(t2,currTime+duration);
    printf("[%s - %s] Salesrep %d is in doctor's chamber\n", t1, t2, num);
    currEventDuration=duration;
    free(arg1);
    pthread_mutex_unlock(&mtx_salesrep);
    pthread_barrier_wait(&salesrep_barrier);

    pthread_exit(NULL);
}

void create_mutex ()
{
    pthread_mutex_init(&mtx_doctor, NULL);
    pthread_mutex_init(&mtx_reporter, NULL);
    pthread_mutex_init(&mtx_salesrep, NULL);
    pthread_mutex_init(&mtx_patient, NULL);

    pthread_mutex_trylock(&mtx_doctor);
    pthread_mutex_unlock(&mtx_doctor);

    pthread_mutex_trylock(&mtx_reporter);
    pthread_mutex_unlock(&mtx_reporter);

    pthread_mutex_trylock(&mtx_salesrep);
    pthread_mutex_unlock(&mtx_salesrep);

    pthread_mutex_trylock(&mtx_patient);
    pthread_mutex_unlock(&mtx_patient);

    pthread_cond_init(&doctor, NULL);
    pthread_cond_init(&patient, NULL);
    pthread_cond_init(&reporter, NULL);
    pthread_cond_init(&salesrep, NULL);

    pthread_barrier_init(&doctor_barrier, NULL, 2);
    pthread_barrier_init(&patient_barrier, NULL, 2);
    pthread_barrier_init(&reporter_barrier, NULL, 2);
    pthread_barrier_init(&salesrep_barrier, NULL, 2);
}

void wind_up ()
{
    pthread_mutex_destroy(&mtx_doctor);
    pthread_mutex_destroy(&mtx_reporter);
    pthread_mutex_destroy(&mtx_salesrep);
    pthread_mutex_destroy(&mtx_patient);

    pthread_cond_destroy(&doctor);
    pthread_cond_destroy(&patient);
    pthread_cond_destroy(&reporter);
    pthread_cond_destroy(&salesrep);

    pthread_barrier_destroy(&doctor_barrier);
    pthread_barrier_destroy(&patient_barrier);
    pthread_barrier_destroy(&reporter_barrier);
    pthread_barrier_destroy(&salesrep_barrier);
}

int num_of_patients;
int num_of_salesrep;
int num_of_reporters;

int main ()
{
    eventQ EQ=initEQ("arrival.txt");
    EQ = addevent(EQ,(event){'D',0,0});
    char time[8];

    create_mutex();

    num_of_patients=0;
    num_of_reporters=0;
    num_of_salesrep=0;
    waiting_patients=0;
    waiting_reporters=0;
    waiting_salesrep=0;
    turn_patient=0;
    turn_reporter=0;
    turn_salesrep=0;
    docHasArrived=0;

    // create doctor thread
    end_of_service=0;
    asleep=1;
    currTime=0;
    pthread_attr_t attr;
    pthread_t doctor_thread;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if(pthread_create(&doctor_thread,&attr,tdoctor,NULL)){
        printf("Error creating doctor thread\n");
        exit(1);
    }
    pthread_attr_destroy(&attr);

    //create other threads
    while(!emptyQ(EQ)){
        event e=nextevent(EQ);
        EQ=delevent(EQ);
        if(end_of_service){
            if(e.type=='P'){
                num_of_patients++;
                printTime(time,e.time);
                printf("          [%s] Patient %d arrives\n", time, num_of_patients);
                printf("          [%s] Patient %d leaves (session over)\n", time, num_of_patients);
            }
            else if(e.type=='R'){
                num_of_reporters++;
                printTime(time,e.time);
                printf("          [%s] Reporter %d arrives\n", time, num_of_reporters);
                printf("          [%s] Reporter %d leaves (session over)\n", time, num_of_reporters); 
            }
            else if(e.type=='S'){
                num_of_salesrep++;
                printTime(time,e.time);
                printf("          [%s] Salesrep %d arrives\n", time, num_of_salesrep);
                printf("          [%s] Salesrep %d leaves (session over)\n", time, num_of_salesrep);
            }
            continue;
        }
        if(e.type=='P'){
            printTime(time,e.time);
            num_of_patients++;
            if(num_of_patients>25){
                printf("          [%s] Patient %d arrives\n", time, num_of_patients);
                printf("          [%s] Patient %d leaves (quota full)\n", time, num_of_patients);
                continue;
            }
            waiting_patients++;
            printf("          [%s] Patient %d arrives\n", time, num_of_patients);
            struct thread_arg *thpatient = (struct thread_arg *)malloc(sizeof(thread_arg));
            thpatient->num=num_of_patients;
            thpatient->duration=e.duration;
            pthread_t patient_thread;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
            if(pthread_create(&patient_thread,&attr,tpatient,(void *)thpatient)){
                printf("Error creating patient thread\n");
                exit(1);
            }
            pthread_attr_destroy(&attr);

            if(!docHasArrived) continue;

            // if patient comes and no one is waiting, doctor is asleep
            if(waiting_patients==1 && !(waiting_reporters || waiting_salesrep)){
                pthread_mutex_lock(&mtx_doctor);
                asleep=0;
                currTime=e.time;
                pthread_cond_signal(&doctor);
                pthread_mutex_unlock(&mtx_doctor);
                pthread_barrier_wait(&doctor_barrier);
                pthread_mutex_lock(&mtx_patient);
                turn_patient++;
                pthread_cond_broadcast(&patient);
                pthread_mutex_unlock(&mtx_patient);
                pthread_barrier_wait(&patient_barrier);
                EQ=addevent(EQ,(event){'p',currTime+currEventDuration,0});
                currTime=e.time+currEventDuration;
            }
        }
        else if(e.type=='R'){
            num_of_reporters++;
            waiting_reporters++;
            printTime(time,e.time);
            printf("          [%s] Reporter %d arrives\n", time, num_of_reporters);
            struct thread_arg *threporter = (struct thread_arg *)malloc(sizeof(thread_arg));
            threporter->num=num_of_reporters;
            threporter->duration=e.duration;
            pthread_t reporter_thread;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
            if(pthread_create(&reporter_thread,&attr,treporter,(void *)threporter)){
                printf("Error creating reporter thread\n");
                exit(1);
            }
            pthread_attr_destroy(&attr);

            if(!docHasArrived) continue;

            // if reporter comes and no one is waiting, doctor is asleep
            if(waiting_reporters==1 && !(waiting_patients || waiting_salesrep)){
                pthread_mutex_lock(&mtx_doctor);
                asleep=0;
                currTime=e.time;
                pthread_cond_signal(&doctor);
                pthread_mutex_unlock(&mtx_doctor);
                pthread_barrier_wait(&doctor_barrier);
                pthread_mutex_lock(&mtx_reporter);
                turn_reporter++;
                pthread_cond_broadcast(&reporter);
                pthread_mutex_unlock(&mtx_reporter);
                pthread_barrier_wait(&reporter_barrier);
                EQ=addevent(EQ,(event){'r',currTime+currEventDuration,0});
                currTime=e.time+currEventDuration;
            }
        }
        else if(e.type=='S'){
            num_of_salesrep++;
            printTime(time,e.time);
            if(num_of_salesrep>3){
                printf("          [%s] Salesrep %d arrives\n", time, num_of_salesrep);
                printf("          [%s] Salesrep %d leaves (quota full)\n", time, num_of_salesrep);
                continue;
            }
            waiting_salesrep++;
            printf("          [%s] Salesrep %d arrives\n", time, num_of_salesrep);
            struct thread_arg *thsalesrep = (struct thread_arg *)malloc(sizeof(thread_arg));
            thsalesrep->num=num_of_salesrep;
            thsalesrep->duration=e.duration;
            // printf("4)Salesrep %d\n", salesrep_num);
            pthread_t salesrep_thread;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
            if(pthread_create(&salesrep_thread,&attr,tsalesrep,(void *)thsalesrep)){
                printf("Error creating salesrep thread\n");
                exit(1);
            }
            pthread_attr_destroy(&attr);

            if(!docHasArrived) continue;

            // if salesrep comes and no one is waiting, doctor is asleep
            if(waiting_salesrep==1 && !(waiting_patients || waiting_reporters)){
                pthread_mutex_lock(&mtx_doctor);
                asleep=0;
                currTime=e.time;
                pthread_cond_signal(&doctor);
                pthread_mutex_unlock(&mtx_doctor);
                pthread_barrier_wait(&doctor_barrier);
                pthread_mutex_lock(&mtx_salesrep);
                turn_salesrep++;
                pthread_cond_broadcast(&salesrep);
                pthread_mutex_unlock(&mtx_salesrep);
                pthread_barrier_wait(&salesrep_barrier);
                EQ=addevent(EQ,(event){'s',currTime+currEventDuration,0});
                currTime=e.time+currEventDuration;
            }
        }
        else if(e.type=='D' || e.type=='p' || e.type=='r' || e.type=='s'){
            if(e.type=='D'){
                docHasArrived=1;
            }
            else if(e.type=='p'){
                waiting_patients--;
            }
            else if(e.type=='r'){
                waiting_reporters--;
            }
            else if(e.type=='s'){
                waiting_salesrep--;
            }

            // if quota over and no reporter waiting, doctor leaves
            if(waiting_patients==0 && waiting_reporters==0 && waiting_salesrep==0 && num_of_patients>=25 && num_of_salesrep>=3){
                pthread_mutex_lock(&mtx_doctor);
                asleep=0;
                end_of_service=1;
                currTime=e.time;
                pthread_cond_signal(&doctor);
                pthread_mutex_unlock(&mtx_doctor);
                pthread_barrier_wait(&doctor_barrier);
                continue;
            }
            
            if((waiting_patients || waiting_reporters || waiting_salesrep)){
                pthread_mutex_lock(&mtx_doctor);
                asleep=0;
                currTime=e.time;
                pthread_cond_signal(&doctor);
                pthread_mutex_unlock(&mtx_doctor);
                pthread_barrier_wait(&doctor_barrier);
            }

            if(waiting_reporters){
                pthread_mutex_lock(&mtx_reporter);
                turn_reporter++;
                pthread_cond_broadcast(&reporter);
                pthread_mutex_unlock(&mtx_reporter);
                pthread_barrier_wait(&reporter_barrier);
                EQ=addevent(EQ,(event){'r',currTime+currEventDuration,0});
            }
            else if(waiting_patients){
                pthread_mutex_lock(&mtx_patient);
                turn_patient++;
                pthread_cond_broadcast(&patient);
                pthread_mutex_unlock(&mtx_patient);
                pthread_barrier_wait(&patient_barrier);
                EQ=addevent(EQ,(event){'p',currTime+currEventDuration,0});
            }
            else if(waiting_salesrep){
                pthread_mutex_lock(&mtx_salesrep);
                turn_salesrep++;
                pthread_cond_broadcast(&salesrep);
                pthread_mutex_unlock(&mtx_salesrep);
                pthread_barrier_wait(&salesrep_barrier);
                EQ=addevent(EQ,(event){'s',currTime+currEventDuration,0});
            }
            currTime=e.time+currEventDuration;
        }
    }
    wind_up();
    return 0;
}

/* End of program */
