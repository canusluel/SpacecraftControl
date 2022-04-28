#include "queue.c"
#include <stdio.h> // printf
#include <pthread.h> // pthread_*
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <sched.h>

#define MAX_EVENT_NUM 1000
#define JOB_NUM 3
int simulationTime = 120;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 40; // frequency of emergency
float p = 0.2;               // probability of a ground job (launch & assembly)
pthread_t tid[1024] = {0};	
int thread_count = 0;
int eventNum = 0; // number of events
int pad_A_available = 0; // 0 available
int pad_B_available = 0; // 0 available
int t = 2;
time_t curTime; // current seconds in simulation
time_t startTime; // start time of simulation
int currentSec = 0;
int emergency = 0; // emergency condition
Queue *landQ, *launchQ, *assemblyQ;
void* LandingJob(void *arg); 
void* LaunchJob(void *arg);
void* AssemblyJob(void *arg); 
void* ControlTower(Job *nextJob);
void recordLogs();
void printWaitingJobs(int n);
//conditions that indicates the availability of pads
pthread_cond_t pad_A, pad_B;
pthread_mutex_t pad_A_mutex, pad_B_mutex;
//struct that is used to print the logs
typedef struct craftEvent{
int id, reqTime, endTime, trndTime;
char status, pad;
}craftEvent;

craftEvent events[MAX_EVENT_NUM];
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
    struct tm *info;
    int n = -1;
    int jobNums[JOB_NUM] = {0, 0, 0};
    long time;
    for(int i=1; i<argc; i++){
        if(!strcmp(argv[i], "-p")) {p = atof(argv[++i]);}
        else if(!strcmp(argv[i], "-t")) {simulationTime = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-s"))  {seed = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-n"))  {n = atoi(argv[++i]);}
    }
    
    srand(seed); // feed the seed
    //initializing mutexes
    pthread_mutex_init(&pad_A_mutex, NULL);
    pthread_mutex_init(&pad_B_mutex, NULL);
    pthread_cond_init(&pad_A,NULL);
    pthread_cond_init(&pad_B,NULL);
    //getting current time of day and using it as star time for simulation
    gettimeofday(&tv, NULL);
    curTime = tv.tv_sec;
    time = curTime + simulationTime;
    info = localtime(&curTime);
    printf("Current time of day: %s\n",asctime (info));
    printf("Current start second: %ld\n", curTime);
    startTime = curTime;
    //creating queues for each job type
    landQ = ConstructQueue(MAX_EVENT_NUM);
    launchQ = ConstructQueue(MAX_EVENT_NUM);
    assemblyQ = ConstructQueue(MAX_EVENT_NUM);
    Job *firstJob = (Job*) malloc(sizeof (Job));
    firstJob->ID = 1;
    firstJob->type = 0; //launch
    firstJob->reqTime = curTime + simulationTime - time;
    printf("Simulation will run %ld seconds!\n", time - curTime);
    // creating a seperate thread for control tower
    pthread_create(&tid[thread_count++], NULL, (void *)&ControlTower, firstJob);
    pthread_sleep(1);
    //simulation
    while(curTime<time) {
	int probability = rand() % 100;
	//getting current time in each iteration to identify the request time of current job
    	gettimeofday(&tv, NULL);
    	curTime = tv.tv_sec;
    	printf("Current second in sim: %ld\n", curTime + simulationTime - time);
    	//calling a job in every t sec
    	if((curTime + simulationTime - time) % t == 0){
    	Job *job = (Job*) malloc(sizeof (Job));
    	job->ID = thread_count;
    	if((curTime + simulationTime - time) % (emergencyFrequency*t) == 0){// emergency
        emergency = 1;
	job->type = 1;
	Job *job2 = (Job*) malloc(sizeof (Job));
	job2->ID = thread_count;
	job->ID = thread_count+1;
	job2->type = 1;
	job2->reqTime = curTime + simulationTime - time;
	ControlTower(job2); 
    	}
	else if(probability < 100*p/2){
	emergency = 0;
        job->type = 0; // launch
        }else if(probability < 100*p){
	emergency = 0;
        job->type = 2; // assembly
        }else{
	emergency = 0;
        job->type = 1; // land
        }
        job->reqTime = curTime + simulationTime - time;
    	ControlTower(job);
	}
	pthread_sleep(1);
    }
    printf("The simulation is ended! waiting remaining jobs to finish..\n");
    pthread_sleep(1);
    //closing threads
    for (int i=0; i<thread_count; ++i)
		pthread_join(tid[i], NULL);
		
	
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
    //if program is called with -n flag, prints waiting jobs
    	if(n != -1) printWaitingJobs(n);
	recordLogs();
    return 0;
}
//this function prints the waiting jobs in the nth second
void printWaitingJobs(int n){

printf("At %d sec landing: ", n);
for(int i = 0; i<eventNum; i++){
	if(events[i].endTime > n && events[i].reqTime < n && events[i].status == 'L' && events[i].trndTime > t){
	printf("%d ", events[i].id);
	}
}
printf("\n");
printf("At %d sec launch: ", n);
for(int i = 0; i<eventNum; i++){
	if(events[i].endTime > n && events[i].reqTime < n && events[i].status == 'D' && events[i].trndTime > 2*t){
	printf("%d ", events[i].id);
	}
}
printf("\n");
printf("At %d sec assembly: ", n);
for(int i = 0; i<eventNum; i++){
	if(events[i].endTime > n && events[i].reqTime < n && events[i].status == 'A' && events[i].trndTime > 6*t){
	printf("%d ", events[i].id);
	}
}
printf("\n");
}
//this function keeps the logs of events in global array, later to be printed in eventLog.txt
void maintainEvents(int eventId, int eventEndTime, int eventReqTime, char eventStatus, char eventPad){

events[eventNum].reqTime = eventReqTime;
events[eventNum].id = eventId;
events[eventNum].endTime = eventEndTime;
events[eventNum].trndTime = eventEndTime - eventReqTime;
events[eventNum].status = eventStatus;
events[eventNum].pad = eventPad;

}
//this function prints the logs in arroy to eventLog.txt
void recordLogs(){
int check = 0;
FILE *f = fopen("eventLog.txt", "w");
if (f == NULL)
{
    printf("Error opening file!\n");
    exit(1);
}

for(int i = 0; i<eventNum; i++){
if(events[i].endTime > simulationTime && check == 0){
fprintf(f, "The events below ended after simulation was terminated.\n");
check = 1;
}
fprintf(f, "Event ID: %d, Status: %c, Request time: %d, End time: %d, Turnaround time: %d, Pad: %c\n", events[i].id, events[i].status, events[i].reqTime, events[i].endTime, events[i].trndTime, events[i].pad);
}

fclose(f);
}
// the function that creates plane threads for landing
void* LandingJob(void *arg){
//waiting for a pad to become available
while(pad_B_available != 0 && pad_A_available != 0){}

struct timeval tv;
time_t finishTime;
//if pad B becomes available, lands rocket to the pad
if(pad_B_available == 0){
pthread_mutex_lock(&pad_B_mutex);
pad_B_available = 1;
printf("A rocket is landing!\n");
Job finished = Dequeue(landQ);
pthread_sleep(t);
printf("The rocket is landed to pad B! Job ID: %d\n", finished.ID);
gettimeofday(&tv, NULL);
finishTime = tv.tv_sec - startTime;
if(emergency == 1){//prioritizing emergency
maintainEvents(finished.ID, finishTime, finished.reqTime, 'E', 'B');
}else{
maintainEvents(finished.ID, finishTime, finished.reqTime, 'L', 'B');
}
eventNum++;
pthread_cond_signal(&pad_B);
pad_B_available = 0;
pthread_mutex_unlock(&pad_B_mutex);

}else{//if pad A becomes available, lands rocket to the pad
pthread_mutex_lock(&pad_A_mutex);
pad_A_available = 1;
printf("A rocket is landing!\n");
Job finished = Dequeue(landQ);
pthread_sleep(t);
printf("The rocket is landed to pad A! Job ID: %d\n", finished.ID);
gettimeofday(&tv, NULL);
finishTime = tv.tv_sec - startTime;
if(emergency == 1){
maintainEvents(finished.ID, finishTime, finished.reqTime, 'E', 'A');
}else{
maintainEvents(finished.ID, finishTime, finished.reqTime, 'L', 'A');
}
eventNum++;
pthread_cond_signal(&pad_A);
pad_A_available = 0;
pthread_mutex_unlock(&pad_A_mutex);
}
}
// the function that creates plane threads for departure
void* LaunchJob(void *arg){

pthread_mutex_lock(&pad_A_mutex);
while(pad_A_available != 0){
	   pthread_cond_wait(&pad_A, &pad_A_mutex); //wait for the condition
	}
	
struct timeval tv;
time_t finishTime;

pad_A_available = 1;
printf("A rocket is launching!\n");
Job finished = Dequeue(launchQ);
pthread_sleep(2*t);
gettimeofday(&tv, NULL);
finishTime = tv.tv_sec - startTime;
printf("The rocket is launched!, Job ID: %d\n", finished.ID);
maintainEvents(finished.ID, finishTime, finished.reqTime, 'D', 'A');
eventNum++;
pthread_cond_signal(&pad_A);
pad_A_available = 0;
pthread_mutex_unlock(&pad_A_mutex);
}

// the function that creates plane threads for emergency landing
void* AssemblyJob(void *arg){

pthread_mutex_lock(&pad_B_mutex);
while(pad_B_available != 0){
	   pthread_cond_wait(&pad_B, &pad_B_mutex); //wait for the condition
	}
	
struct timeval tv;
time_t finishTime;

pad_B_available = 1;
printf("A rocket is being assembled!\n");
Job finished = Dequeue(assemblyQ);
pthread_sleep(6*t);
printf("The rocket is assembled!, Job ID: %d\n", finished.ID);
gettimeofday(&tv, NULL);
finishTime = tv.tv_sec - startTime;
maintainEvents(finished.ID, finishTime, finished.reqTime, 'A', 'B');
eventNum++;
pthread_cond_signal(&pad_B);
pad_B_available = 0;
pthread_mutex_unlock(&pad_B_mutex);
}

// the function that controls the air traffic
void* ControlTower(Job *nextJob){
pthread_attr_t tattr;
int jobThread;
int jobPriority = 10;
struct sched_param param;
//initializing the attributes to default
pthread_attr_init (&tattr);
//checking the current scheduling parameter
pthread_attr_getschedparam (&tattr, &param);
// id = 0, launch
if(nextJob->type == 0){
	Enqueue(launchQ, *nextJob);
	printf("Launch job %d has been queued\n", nextJob->ID);
	if(launchQ->size >= 3){
	printf("Launch has been prioritized\n");
        jobPriority = 3; 
        }
        //setting the current scheduling priority to job priority
	param.sched_priority = jobPriority;
	jobThread = pthread_attr_setschedparam (&tattr, &param);
	printf("Task priority of launch, (Job ID: %d): %d\n",nextJob->ID, param.sched_priority);
	//creating thread with the related function and priority parameter
	jobThread = pthread_create (&tid[thread_count++], &tattr, &LaunchJob, NULL);
}else if(nextJob->type == 1){// land
	Enqueue(landQ, *nextJob);
	printf("Land job %d has been queued\n", nextJob->ID);
	if(emergency == 1){
	printf("Emergency landings in sequence!\n");
	jobPriority = 1; 
	}
	else if(landQ->size >= 6){
	printf("Land has been prioritized\n");
	jobPriority = 2;
	}else{
	jobPriority = 4;
	} 
	param.sched_priority = jobPriority;
	jobThread = pthread_attr_setschedparam (&tattr, &param);
	printf("Task priority of land, (Job ID: %d): %d\n",nextJob->ID, param.sched_priority);
	jobThread = pthread_create (&tid[thread_count++], &tattr, &LandingJob, NULL);
}else if(nextJob->type == 2){// assemble
	Enqueue(assemblyQ, *nextJob);
	printf("Assembly job %d has been queued\n", nextJob->ID);
	if(assemblyQ->size >= 3){
	printf("Assembly has been prioritized\n");
	jobPriority = 3; 
	}
	param.sched_priority = jobPriority;
	jobThread = pthread_attr_setschedparam (&tattr, &param);
	printf("Task priority of assemble, (Job ID: %d): %d\n",nextJob->ID, param.sched_priority);
	jobThread = pthread_create (&tid[thread_count++], &tattr, &AssemblyJob, NULL);
}
}
