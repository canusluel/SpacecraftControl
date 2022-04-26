#include "queue.c"
#include <stdio.h> // printf
#include <pthread.h> // pthread_*
#include <time.h>
#include <sys/time.h>
#include <string.h>

#define MAX_EVENT_NUM 1000
#define JOB_NUM 3
int simulationTime = 120;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 40; // frequency of emergency
float p = 0.2;               // probability of a ground job (launch & assembly)
pthread_t tid[1024] = {0};	
int thread_count = 0;
int eventNum = 0;
int pad_A_available = 0; // 0 available
int pad_B_available = 0; // 0 available
int t = 2;
//current seconds in simulation
time_t curTime;
time_t startTime;
int currentSec = 0;
int emergency = 0;
Queue *landQ, *launchQ, *assemblyQ;
void* LandingJob(void *arg); 
void* LaunchJob(void *arg);
void* EmergencyJob(void *arg); 
void* AssemblyJob(void *arg); 
void* ControlTower(Job *nextJob);
void recordLogs();
void printWaitingJobs(int n);
//conditions that indicates the availability of pads

pthread_cond_t pad_A, pad_B;
pthread_mutex_t pad_A_mutex, pad_B_mutex;

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
    pthread_mutex_init(&pad_A_mutex, NULL);
    pthread_mutex_init(&pad_B_mutex, NULL);
    pthread_cond_init(&pad_A,NULL);
    pthread_cond_init(&pad_B,NULL);
    gettimeofday(&tv, NULL);
    curTime = tv.tv_sec;
    time = curTime + simulationTime;
    info = localtime(&curTime);
    printf("Current time of day: %s\n",asctime (info));
    printf("Current start second: %ld\n", curTime);
    startTime = curTime;
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
        //stuff();

	int probability = rand() % 100;
    	gettimeofday(&tv, NULL);
    	curTime = tv.tv_sec;
    	printf("Current second in sim: %ld\n", curTime + simulationTime - time);
    	if((curTime + simulationTime - time) % t == 0){
    	Job *job = (Job*) malloc(sizeof (Job));
    	job->ID = thread_count;
    	if((curTime + simulationTime - time) % (emergencyFrequency*t) == 0){
    	 emergency = 1; 
    	 }
	if(probability < 100*p/2){
        job->type = 0; // launch
        }else if(probability < 100*p){
        job->type = 2; // assembly
        }else{
        job->type = 1; // land
        }
        job->reqTime = curTime + simulationTime - time;
    	ControlTower(job);
	}
	pthread_sleep(1);
    }
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

	printf("The simulation is ended! waiting remaining jobs to finish..");
	pthread_sleep(1);
    // your code goes here
    //if program is called with -n flag, prints ongoing jobs
    	if(n != -1) printWaitingJobs(n);
	recordLogs();
    return 0;
}
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
}
void maintainEvents(int eventId, int eventEndTime, int eventReqTime, char eventStatus, char eventPad){

events[eventNum].reqTime = eventReqTime;
events[eventNum].id = eventId;
events[eventNum].endTime = eventEndTime;
events[eventNum].trndTime = eventEndTime - eventReqTime;
events[eventNum].status = eventStatus;
events[eventNum].pad = eventPad;

}
void recordLogs(){

FILE *f = fopen("eventLog.txt", "w");
if (f == NULL)
{
    printf("Error opening file!\n");
    exit(1);
}

for(int i = 0; i<eventNum; i++){
fprintf(f, "Event ID: %d, Status: %c, Request time: %d, End time: %d, Turnaround time: %d, Pad: %c\n", events[i].id, events[i].status, events[i].reqTime, events[i].endTime, events[i].trndTime, events[i].pad);
}

fclose(f);
}
// the function that creates plane threads for landing
void* LandingJob(void *arg){

while(pad_B_available != 0 && pad_A_available != 0){}

struct timeval tv;
time_t finishTime;

if(pad_B_available == 0){
pthread_mutex_lock(&pad_B_mutex);
pad_B_available = 1;
printf("A rocket is landing!\n");
pthread_sleep(t);
Job finished = Dequeue(landQ);
printf("The rocket is landed to pad B! Job ID: %d\n", finished.ID);
gettimeofday(&tv, NULL);
finishTime = tv.tv_sec - startTime;
maintainEvents(finished.ID, finishTime, finished.reqTime, 'L', 'B');
eventNum++;
pthread_cond_signal(&pad_B);
pad_B_available = 0;
pthread_mutex_unlock(&pad_B_mutex);

}else{
pthread_mutex_lock(&pad_A_mutex);
pad_A_available = 1;
printf("A rocket is landing!\n");
pthread_sleep(t);
Job finished = Dequeue(landQ);
printf("The rocket is landed to pad A! Job ID: %d\n", finished.ID);
gettimeofday(&tv, NULL);
finishTime = tv.tv_sec - startTime;
maintainEvents(finished.ID, finishTime, finished.reqTime, 'L', 'A');
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
pthread_sleep(2*t);
Job finished = Dequeue(launchQ);
gettimeofday(&tv, NULL);
finishTime = tv.tv_sec - startTime;
printf("The rocket is launched!, Job ID: %d\n", finished.ID);
maintainEvents(finished.ID, finishTime, finished.reqTime, 'D', 'A');
eventNum++;
pthread_cond_signal(&pad_A);
pad_A_available = 0;
pthread_mutex_unlock(&pad_A_mutex);
//--
//if(launchQ->size >= 3){ controlTower(Dequeue(launchQ)); }
}

// the function that creates plane threads for emergency landing
void* EmergencyJob(void *arg){
printf("nice\n");
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
pthread_sleep(6*t);
Job finished = Dequeue(assemblyQ);
printf("The rocket is assembled!, Job ID: %d\n", finished.ID);
gettimeofday(&tv, NULL);
finishTime = tv.tv_sec - startTime;
maintainEvents(finished.ID, finishTime, finished.reqTime, 'A', 'B');
eventNum++;
pthread_cond_signal(&pad_B);
pad_B_available = 0;
pthread_mutex_unlock(&pad_B_mutex);
//--
//if(assemblyQ->size >= 3){ controlTower(Dequeue(assemblyQ)); }
}

// the function that controls the air traffic
void* ControlTower(Job *nextJob){
/*
	if(emergency==1){
	printf("Emergency landing initiated!\n");
	pthread_create(&tid[thread_count++], NULL, &EmergencyJob, NULL);
	emergency = 0;
	}
	//to avoid starvation of landing jobs
 	if(landQ->size >= 6){
	pthread_create(&tid[thread_count++], NULL, &LandingJob, NULL);
	}  
	if(assemblyQ->size >= 3){
	printf("Assembly has been prioritized\n");
	pthread_create(&tid[thread_count++], NULL, &AssemblyJob, NULL);
	}
        if(launchQ->size >= 3){
	printf("Launch has been prioritized\n");
        pthread_create(&tid[thread_count++], NULL, &LaunchJob, NULL);
        }
        if(isEmpty(landQ)==0){
        pthread_create(&tid[thread_count++], NULL, &LandingJob, NULL);
        }
        */
// id = 0, launch
if(nextJob->type == 0){
	Enqueue(launchQ, *nextJob);
	printf("Launch job %d has been queued\n", nextJob->ID);
	//waiting until pad A is available
	pthread_create(&tid[thread_count++], NULL, &LaunchJob, NULL);
}else if(nextJob->type == 1){// land
	Enqueue(landQ, *nextJob);
	printf("Land job %d has been queued\n", nextJob->ID);
	pthread_create(&tid[thread_count++], NULL, &LandingJob, NULL);
}else if(nextJob->type == 2){// assemble
	Enqueue(assemblyQ, *nextJob);
	printf("Assembly job %d has been queued\n", nextJob->ID);
	pthread_create(&tid[thread_count++], NULL, &AssemblyJob, NULL);
}
}
