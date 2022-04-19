#include "queue.c"
#include <stdio.h> // printf
#include <pthread.h> // pthread_*
#include <time.h>
#include <sys/time.h>
#include <string.h>

#define MAX_EVENT_NUM 1000
int simulationTime = 120;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 40; // frequency of emergency
float p = 0.2;               // probability of a ground job (launch & assembly)
pthread_t tid[1024] = {0};	
int thread_count = 0;
int eventNum = 0;
int t = 2;
Queue *landQ, *launchQ, *assemblyQ;
void* LandingJob(void *arg); 
void* LaunchJob(void *arg);
void* EmergencyJob(void *arg); 
void* AssemblyJob(void *arg); 
void* ControlTower(Job *nextJob);

//conditions that indicates the availability of pads
pthread_cond_t pad_A, pad_B;
pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct craftEvent{
int id, reqTime, endTime, trndTime;
char status, pad;
}craftEvent;

// pthread sleeper function
int pthread_sleep (int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if(pthread_mutex_init(&mutex,NULL))
    {
        return -1;
    }
    if(pthread_cond_init(&conditionvar,NULL))
    {
        return -1;
    }
    struct timeval tp;
    //When to expire is an absolute time, so get the current time and add it to our delay time
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;
    
    pthread_mutex_lock (&mutex);
    int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock (&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);
    
    //Upon successful completion, a value of zero shall be returned
    return res;
}

int main(int argc,char **argv){
    // -p (float) => sets p
    // -t (int) => simulation time in seconds
    // -s (int) => change the random seed
    struct timeval tv;
    time_t curTime;
    struct tm *info;
    int i = 0;
    int time = 5;
    craftEvent events[MAX_EVENT_NUM];
    for(int i=1; i<argc; i++){
        if(!strcmp(argv[i], "-p")) {p = atof(argv[++i]);}
        else if(!strcmp(argv[i], "-t")) {simulationTime = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-s"))  {seed = atoi(argv[++i]);}
    }
    
    srand(seed); // feed the seed
    pthread_mutex_init(&mymutex, NULL);
    pthread_cond_init(&pad_A,NULL);
    pthread_cond_init(&pad_B,NULL);
    gettimeofday(&tv, NULL);
    curTime = tv.tv_sec;

    info = localtime(&curTime);
    printf("Current time of day: %s",asctime (info));
    landQ = ConstructQueue(MAX_EVENT_NUM);
    launchQ = ConstructQueue(MAX_EVENT_NUM);
    assemblyQ = ConstructQueue(MAX_EVENT_NUM);
    Job *firstJob = (Job*) malloc(sizeof (Job));
    firstJob->ID = thread_count;
    firstJob->type = 0; //launch
    
    
    // creating a seperate thread for control tower
    pthread_create(&tid[thread_count++], NULL, (void *)&ControlTower, firstJob);
    //simulation
    while(i<time) {

    printf("doing stuff in duration %d\n", i);
        //stuff();
        pthread_sleep(1);
        i++;
        Job *job = (Job*) malloc(sizeof (Job));
    	job->ID = thread_count;
    	job->type = 0; //launch
    	ControlTower(job);
    }
    /* Queue usage example
        Queue *myQ = ConstructQueue(1000);
        Job j;
        j.ID = myID;
        j.type = 2;
        Enqueue(myQ, j);
        Job ret = Dequeue(myQ);
        DestructQueue(myQ);
    */

    // your code goes here

    return 0;
}

// the function that creates plane threads for landing
void* LandingJob(void *arg){

pthread_mutex_lock(&mymutex);
pthread_cond_wait(&pad_B, &mymutex);
printf("A rocket is landing!");
pthread_sleep(1*t);
printf("The rocket is landed");
Job finished = Dequeue(landQ);

pthread_mutex_unlock(&mymutex);

}

// the function that creates plane threads for departure
void* LaunchJob(void *arg){

pthread_mutex_lock(&mymutex);
pthread_cond_wait(&pad_A, &mymutex);
printf("A rocket is launching!");
pthread_sleep(2*t);
printf("The rocket is launched!");
Job finished = Dequeue(launchQ);

pthread_mutex_unlock(&mymutex);

}

// the function that creates plane threads for emergency landing
void* EmergencyJob(void *arg){

}

// the function that creates plane threads for emergency landing
void* AssemblyJob(void *arg){

pthread_mutex_lock(&mymutex);
pthread_cond_wait(&pad_B, &mymutex);
printf("A rocket is being assembled!");
pthread_sleep(6*t);
printf("The rocket is assembled!");
Job finished = Dequeue(assemblyQ);

pthread_mutex_unlock(&mymutex);

}

// the function that controls the air traffic
void* ControlTower(Job *nextJob){
printf("Recieved job type %d", nextJob->ID);
// id = 0, launch
if(nextJob->ID == 0){
Enqueue(launchQ, *nextJob);
printf("Job %d has been queued", nextJob->ID);
}else if(nextJob->ID == 1){// land
Enqueue(landQ, *nextJob);
printf("Job %d has been queued", nextJob->ID);
}else if(nextJob->ID == 2){// assemble
Enqueue(assemblyQ, *nextJob);
printf("Job %d has been queued", nextJob->ID);
}
}
