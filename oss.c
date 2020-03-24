#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include "shared.h"

void setUp(); //allocate shared memory
void detach(); //clear shared memory and message queues
void sigErrors(int signum);
void initBitVector(int n);
void initQueue(int[], int);
void setTimeToNextProc();
void makePCB(int pidnum, int isRealTime);
int getOpenBitVector();
int roll1000();
int getNextOccupiedQueue();
void spawnNewUser();
int isBlockedQueueEmpty();
int checkBlockedQueue();
int incrementIdleTime();
void awakenUser(int);
int removeProcFromQueue(int[], int, int);
void incrementSimClockAfterMessageReceipt(int);
void terminateUser(int);
void blockUser(int);
int getNextProcFromQueue(int[]);
int isTimeToSpawnProc();

unsigned int totalUserLifetime_secs, totalUserLifetime_ns;
unsigned int totalWaitTime_secs, totalWaitTime_ns;
unsigned int totalBlockedTime_secs, totalBlockedTime_ns;
int shmid_sim_secs, shmid_sim_ns;
unsigned int idleTime_secs = 0;
unsigned int idleTime_ns = 0;
int shmid_pct;
int oss_qid;
int arraysize = 19;
unsigned int maxTimeBetweenProcsNS, maxTimeBetweenProcsSecs;
unsigned int timeToNextProcNS, timeToNextProcSecs, seed;
unsigned int spawnNextProcNS, spawnNextProcSecs;
static unsigned int *simClock_secs; //pointer to shm sim clock (seconds)
static unsigned int *simClock_ns; //pointer to shm sim clock (nanoseconds)
int bitVector[19];
int blocked[19];
int numProcsSpawned = 0;
int allowNewUsers = 1;
int numCurrentUsers = 0;
int numProcsUnblocked;
pid_t childpids[20];
int loglines = 0;
int nextProcToRun, nextOccupiedQueue;

int queue0[19]; 
int queue1[19]; 
int queue2[19]; 
int queue3[19]; 

unsigned int quantum_0 = 2000000;
unsigned int quantum_1 = 4000000; 
unsigned int quantum_2 = 8000000;
unsigned int quantum_3 = 16000000;

char outputFileName[] = "log";
FILE* fp;

struct commsbuf infostruct;

struct pcb * pct;


int main(int argc, char** argv) 
{
	int c;
	while((c=getopt(argc, argv, "i:h"))!= EOF)
        {
                switch(c)
                {
        case 'h':
                printf("\nInvocation: master [-h] [-i x]\n");
                printf("------------------------------------------Program Options-----------------------------------\n");
                printf("       -h             Describe how the project should be run and then, terminate\n");
		printf("       -i             Type file name to print program information in (Default of message.log)\n");
		return EXIT_SUCCESS;
        case 'i':
                strcpy(outputFileName, optarg);
                break;
        default:
                return -1;

        	}
	}

	fp = fopen(outputFileName, "w");
	time_t start = time(NULL);

	int finishThem = 0;
    	int i; 
   	double realTimeElapsed = 0;
    	double runtimeAllowed = 10; 
    	maxTimeBetweenProcsNS = 999999998;
    	maxTimeBetweenProcsSecs = 0;
    	seed = (unsigned int) getpid(); 
    	pid_t childpid; 
    	char str_pct_id[20]; 
    	char str_user_sim_pid[4]; 
   	int user_sim_pid;
    	int firstblocked;	

	if (signal(SIGINT, sigErrors) == SIG_ERR) //sigerror on cntrl-c
        {
                exit(0);
        }

        if (signal(SIGALRM, sigErrors) == SIG_ERR) //sigerror on program exceeding 2 second alarm
        {
                exit(0);
        }

	setUp();
	
	initBitVector(arraysize);
    	initQueue(queue0, arraysize);
    	initQueue(queue1, arraysize);
    	initQueue(queue2, arraysize);
    	initQueue(queue3, arraysize);
    	initQueue(blocked, arraysize);
    
   	setTimeToNextProc();
    	printf("First user will spawn at %u:%u\n", 
	spawnNextProcSecs, spawnNextProcNS);
	
	while (1) 
	{
		    if (realTimeElapsed < runtimeAllowed) {
            realTimeElapsed = (double)(time(NULL) - start);
            if (realTimeElapsed >= runtimeAllowed) {
                printf("OSS: Real-time limit of %f has elapsed. No new users"
                        "will be generated\n", runtimeAllowed);
                allowNewUsers = 0;
            }
        }
       
        if (allowNewUsers == 0 && numCurrentUsers == 0) {
            printf("OSS: User generation is disallowed and "
                    "all users have terminated.\n");
            fprintf(fp, "OSS: User generation is disallowed and "
                    "all users have terminated.\n");
            break;
        }

        numProcsUnblocked = 0;
        if (isBlockedQueueEmpty() == 0) { 
            numProcsUnblocked = checkBlockedQueue();
        }


        nextOccupiedQueue = getNextOccupiedQueue();
    
        if ( (nextOccupiedQueue == -1) && (allowNewUsers == 0) ) {
            setTimeToNextProc();
            fprintf(fp, "OSS: No processes ready to run, incrementing sim "
                    "clock from %u:%09u to ", *simClock_secs, *simClock_ns);
            incrementIdleTime();
            *simClock_secs = spawnNextProcSecs;
            *simClock_ns = spawnNextProcNS;
            fprintf(fp, "%u:%09u\n", *simClock_secs, *simClock_ns);
            fflush(fp);
         
            if (numProcsUnblocked == 0) {
                finishThem = 1;
                continue;
            		}
        }	
	else if ( nextOccupiedQueue == -1 && allowNewUsers == 1 ) {
            fprintf(fp, "OSS: Current live users = %d\n", numCurrentUsers);
            fprintf(fp, "OSS: No processes ready to run, incrementing sim "
                    "clock from %u:%09u to ", *simClock_secs, *simClock_ns);
            incrementIdleTime();
            *simClock_secs = spawnNextProcSecs;
            *simClock_ns = spawnNextProcNS;
            fprintf(fp, "%u:%09u\n", *simClock_secs, *simClock_ns);
            fflush(fp);
            if ( (allowNewUsers == 1) && (getOpenBitVector() != -1) ) {
                spawnNewUser(); 
                                
                numCurrentUsers++;
                fprintf(fp, "OSS: Dispatching process PID %d from queue %d at "
                    "time %u:%09u\n", infostruct.user_sim_pid ,
                        pct[infostruct.user_sim_pid].currentQueue,
                        *simClock_secs, *simClock_ns);
                if ( msgsnd(oss_qid, &infostruct, sizeof(infostruct), 0) == -1 ) {
                    perror("OSS: error sending init msg");
                    detach();
                    exit(0);
                }
            }
        }
	else if ( finishThem == 1 && nextOccupiedQueue != -1) {
            finishThem = 0;
                if (nextOccupiedQueue == 0) {
                    nextProcToRun = getNextProcFromQueue(queue0);
                    infostruct.ossTimeSliceGivenNS = quantum_0;
                    removeProcFromQueue(queue0, arraysize, nextProcToRun);
                    addProcToQueue(queue0, arraysize, nextProcToRun);
                }
                else if (nextOccupiedQueue == 1) {
                    nextProcToRun = getNextProcFromQueue(queue1);
                    infostruct.ossTimeSliceGivenNS = quantum_1;
                    removeProcFromQueue(queue1, arraysize, nextProcToRun);
                    addProcToQueue(queue1, arraysize, nextProcToRun);
                }
                
                if (nextProcToRun != -1) {
                    infostruct.msgtyp = (long) nextProcToRun;
                    infostruct.userTerminatingFlag = 0;
                    infostruct.userUsedFullTimeSliceFlag = 0;
                    infostruct.userBlockedFlag = 0;
                    infostruct.user_sim_pid = nextProcToRun;
                    fprintf(fp, "OSS: Dispatching process PID %d from queue %d at "
                    "time %u:%09u\n", nextProcToRun, 
                    pct[nextProcToRun].currentQueue, 
                    *simClock_secs, *simClock_ns);
                    if ( msgsnd(oss_qid, &infostruct, sizeof(infostruct), 0) == -1 ) {
                        perror("OSS: error sending user msg");
                        detach();
                        exit(0);
                    }
                }
        }

	 firstblocked = blocked[1]; 
 
        if ( msgrcv(oss_qid, &infostruct, sizeof(infostruct), 99, 0) == -1 ) {
            perror("User: error in msgrcv");
            detach();
            exit(0);
        }
        blocked[1] = firstblocked; 
        

        pct[infostruct.user_sim_pid].timeUsedLastBurst_ns = 
                infostruct.userTimeUsedLastBurst;
        
        incrementSimClockAfterMessageReceipt(infostruct.user_sim_pid);

	 if (infostruct.userTerminatingFlag == 1) {
            fprintf(fp, "OSS: Receiving that process PID %d ran for %u"
                    " nanoseconds and "
                    "then terminated\n", infostruct.user_sim_pid,
                    infostruct.userTimeUsedLastBurst);
            terminateUser(infostruct.user_sim_pid);
        }
       
        else if (infostruct.userBlockedFlag == 1) {
            fprintf(fp, "OSS: Receiving that process PID %d ran for %u "
                    "nanoseconds and "
                    "then was blocked by an event. Moving to blocked queue\n", 
                    infostruct.user_sim_pid, infostruct.userTimeUsedLastBurst);
            blockUser(infostruct.user_sim_pid);
        }
        
        else if (infostruct.userUsedFullTimeSliceFlag == 0) {
            fprintf(fp, "OSS: Receiving that process PID %d ran for %u "
                    "nanoseconds, not "
                    "using its entire quantum", infostruct.user_sim_pid,
                    infostruct.userTimeUsedLastBurst);
         
            if (pct[infostruct.user_sim_pid].currentQueue == 2) {
                fprintf(fp, ", moving to queue 1\n");
                removeProcFromQueue(queue2, arraysize, infostruct.user_sim_pid);
                addProcToQueue(queue1, arraysize, infostruct.user_sim_pid);
                pct[infostruct.user_sim_pid].currentQueue = 1;
            }
            else if (pct[infostruct.user_sim_pid].currentQueue == 3) {
                fprintf(fp, ", moving to queue 1\n");
                removeProcFromQueue(queue3, arraysize, infostruct.user_sim_pid);
                addProcToQueue(queue1, arraysize, infostruct.user_sim_pid);
                pct[infostruct.user_sim_pid].currentQueue = 1;
            }
            else {
                fprintf(fp, "\n");
            }
        }
	  else if (infostruct.userUsedFullTimeSliceFlag == 1) {
            fprintf(fp, "OSS: Receiving that process PID %d ran for %u "
                    "nanoseconds", 
                    infostruct.user_sim_pid,
                    infostruct.userTimeUsedLastBurst);
            
            if (pct[infostruct.user_sim_pid].currentQueue == 1) {
                fprintf(fp, ", moving to queue 2\n");
                removeProcFromQueue(queue1, arraysize, infostruct.user_sim_pid);
                addProcToQueue(queue2, arraysize, infostruct.user_sim_pid);
                pct[infostruct.user_sim_pid].currentQueue = 2;
            }
         
            else if (pct[infostruct.user_sim_pid].currentQueue == 2) {
                fprintf(fp, ", moving to queue 3\n");
                removeProcFromQueue(queue2, arraysize, infostruct.user_sim_pid);
                addProcToQueue(queue3, arraysize, infostruct.user_sim_pid);
                pct[infostruct.user_sim_pid].currentQueue = 3;
            }
            else {
                fprintf(fp, "\n");
            }
        }
        

        if (allowNewUsers == 1) {
            if (isTimeToSpawnProc()) {
                if (getOpenBitVector() != -1) {
                    spawnNewUser();
                    numCurrentUsers++;
                }
                else {
                    fprintf(fp, "OSS: New process spawn denied: "
                            "process control table is full.\n");
                    setTimeToNextProc();
                }
            }
        }
	
	      nextOccupiedQueue = getNextOccupiedQueue();
        if (nextOccupiedQueue == 0) {
            nextProcToRun = getNextProcFromQueue(queue0);
            infostruct.ossTimeSliceGivenNS = quantum_0;
            removeProcFromQueue(queue0, arraysize, nextProcToRun);
            addProcToQueue(queue0, arraysize, nextProcToRun);
        }
        else if (nextOccupiedQueue == 1) {
            nextProcToRun = getNextProcFromQueue(queue1);
            infostruct.ossTimeSliceGivenNS = quantum_1;
            removeProcFromQueue(queue1, arraysize, nextProcToRun);
            addProcToQueue(queue1, arraysize, nextProcToRun);
        }
        else if (nextOccupiedQueue == 2) {
            nextProcToRun = getNextProcFromQueue(queue2);
            infostruct.ossTimeSliceGivenNS = quantum_2;
            removeProcFromQueue(queue2, arraysize, nextProcToRun);
            addProcToQueue(queue2, arraysize, nextProcToRun);
        }
        else if (nextOccupiedQueue == 3) {
            nextProcToRun = getNextProcFromQueue(queue3);
            infostruct.ossTimeSliceGivenNS = quantum_3;
            removeProcFromQueue(queue3, arraysize, nextProcToRun);
            addProcToQueue(queue3, arraysize, nextProcToRun);
        }
   
        else {
            continue;
        }

	if (nextProcToRun != -1) {
            infostruct.msgtyp = (long) nextProcToRun;
            infostruct.userTerminatingFlag = 0;
            infostruct.userUsedFullTimeSliceFlag = 0;
            infostruct.userBlockedFlag = 0;
            infostruct.user_sim_pid = nextProcToRun;
            fprintf(fp, "OSS: Dispatching process PID %d from queue %d at "
                    "time %u:%09u\n", nextProcToRun, 
                    pct[nextProcToRun].currentQueue, 
                    *simClock_secs, *simClock_ns);
            if ( msgsnd(oss_qid, &infostruct, sizeof(infostruct), 0) == -1 ) {
                perror("OSS: error sending init msg");
                detach();
                exit(0);
            }
        }
    
    }

	//}

	detach();

	return 0;
}

int isTimeToSpawnProc() {
    int return_val = 0;
    unsigned int localsec = *simClock_secs;
    unsigned int localns = *simClock_ns;
    if ( (localsec > spawnNextProcSecs) || 
            ( (localsec >= spawnNextProcSecs) && (localns >= spawnNextProcNS) ) ) {
        return_val = 1;
    }
 
    return return_val;
}

void blockUser(int blockpid) {
    int temp;
    unsigned int localsec, localns;
    
    totalBlockedTime_secs += pct[blockpid].totalBlockedTime_secs;
    totalBlockedTime_ns += pct[blockpid].totalBlockedTime_ns;
    if (totalBlockedTime_ns >= BILLION) {
        totalBlockedTime_secs++;
        temp = totalBlockedTime_ns - BILLION;
        totalBlockedTime_ns = temp;
    }
   
    pct[blockpid].blocked = 1;
    if (pct[blockpid].currentQueue == 0)
        {removeProcFromQueue(queue0, arraysize, blockpid);}
    else if (pct[blockpid].currentQueue == 1)
        {removeProcFromQueue(queue1, arraysize, blockpid);}
    else if (pct[blockpid].currentQueue == 2)
        {removeProcFromQueue(queue2, arraysize, blockpid);}
    else if (pct[blockpid].currentQueue == 3)
        {removeProcFromQueue(queue3, arraysize, blockpid);}

    addProcToQueue(blocked, arraysize, blockpid);

    localsec = *simClock_secs;
    localns = *simClock_ns;
    temp = roll1000();
    if (temp < 100) temp = 10000;
    else temp = temp * 100;
    localns = localns + temp; 
    fprintf(fp, "OSS: Time used to move user to blocked queue: %u "
            "nanoseconds\n", temp);
    if (localns >= BILLION) {
        localsec++;
        temp = localns - BILLION;
        localns = temp;
    }
   
    *simClock_secs = localsec;
    *simClock_ns = localns;
}

int getNextProcFromQueue(int q[]) {
    if (q[1] == 0) { 
        return -1;
    }
    else return q[1];
}

void terminateUser(int termpid) {
    int status;
    unsigned int temp;
    waitpid(infostruct.user_sys_pid, &status, 0);
    
    totalUserLifetime_secs += pct[termpid].totalLIFEtime_secs;
    totalUserLifetime_ns += pct[termpid].totalLIFEtime_ns;
    if (totalUserLifetime_ns >= BILLION) {
        totalUserLifetime_secs++;
        temp = totalUserLifetime_ns - BILLION;
        totalUserLifetime_ns = temp;
    }

    
    totalWaitTime_secs += 
            (pct[termpid].totalLIFEtime_secs - pct[termpid].totalCPUtime_secs);
    totalWaitTime_ns +=
            (pct[termpid].totalLIFEtime_ns - pct[termpid].totalCPUtime_ns);
    if (totalWaitTime_ns >= BILLION) {
        totalWaitTime_secs++;
        temp = totalWaitTime_ns - BILLION;
        totalWaitTime_ns = temp;
    }
    
    
    if (pct[termpid].currentQueue == 0) {
        removeProcFromQueue(queue0, arraysize, termpid);
    }
    else if (pct[termpid].currentQueue == 1) {
        removeProcFromQueue(queue1, arraysize, termpid);
    }
    else if (pct[termpid].currentQueue == 2) {
        removeProcFromQueue(queue2, arraysize, termpid);
    }
    else if (pct[termpid].currentQueue == 3) {
        removeProcFromQueue(queue3, arraysize, termpid);
    }
    bitVector[termpid] = 0;
    numCurrentUsers--;
    pct[termpid].totalCPUtime_secs = 0;
    pct[termpid].totalCPUtime_ns = 0;
    pct[termpid].totalLIFEtime_secs = 0;
    pct[termpid].totalLIFEtime_ns = 0;
    printf("OSS: User %d has terminated. Users alive: %d\n",
        infostruct.user_sim_pid, numCurrentUsers);
}

void incrementSimClockAfterMessageReceipt(int userpid) {
    unsigned int localsec = *simClock_secs;
    unsigned int localns = *simClock_ns;
    unsigned int temp;
    temp = roll1000();
    if (temp < 100) temp = 100;
    else temp = temp * 10;
    localns = localns + temp; 
    fprintf(fp, "OSS: Time spent this dispatch: %ld nanoseconds\n", temp);
    
    localns = localns + pct[userpid].timeUsedLastBurst_ns;
    
    if (localns >= BILLION) {
        localsec++;
        temp = localns - BILLION;
        localns = temp;
    }
    
    *simClock_secs = localsec;
    *simClock_ns = localns;
}

int removeProcFromQueue(int q[], int arrsize, int proc_num) {
    int i;
    for (i=1; i<arrsize; i++) {
        if (q[i] == proc_num) { 
            while(i+1 < arrsize) { 
                q[i] = q[i+1];
                i++;
            }
            q[18] = 0; 
            return 1;
        }
    }
    return -1;
}

void awakenUser(int wakepid) {
    unsigned int localsec, localns, temp;
    fprintf(fp, "OSS: Waking up user %d ", wakepid);

    removeProcFromQueue(blocked, arraysize, wakepid);

    pct[wakepid].blocked = 0;
    pct[wakepid].blockedUntilNS = 0;
    pct[wakepid].blockedUntilSecs = 0; 

    if (pct[wakepid].isRealTimeClass == 1) {
        addProcToQueue(queue0, arraysize, wakepid);
        pct[wakepid].currentQueue = 0;
        fprintf(fp, "and putting into queue 0\n");
    }
    
    else {
        addProcToQueue(queue1, arraysize, wakepid);
        pct[wakepid].currentQueue = 1;
        fprintf(fp, "and putting into queue 1\n");
    }

    localsec = *simClock_secs;
    localns = *simClock_ns;
    temp = roll1000();
    if (temp < 100) temp = 10000;
    else temp = temp * 100;
    localns = localns + temp; 
    fprintf(fp, "OSS: Time used to awaken user from blocked state: %u "
            "nanoseconds\n", temp);
}

int getNextOccupiedQueue() {
    if (queue0[1] != 0) {
        return 0;
    }
    else if (queue1[1] != 0) {
        return 1;
    }
    else if (queue2[1] != 0) {
        return 2;
    }
    else if (queue3[1] != 0) {
        return 3;
    }
    else return -1;
}

int addProcToQueue (int q[], int arrsize, int proc_num) {
    int i;
    for (i=1; i<arrsize; i++) {
        if (q[i] == 0) { 
            q[i] = proc_num;
            return 1;
        }
    }
    return -1; 
}

void setTimeToNextProc()
{
	unsigned int temp;
	unsigned int localsecs = *simClock_secs;
	unsigned int localns = *simClock_ns;
	timeToNextProcSecs = rand_r(&seed) % (maxTimeBetweenProcsSecs + 1);
    	timeToNextProcNS = rand_r(&seed) % (maxTimeBetweenProcsNS + 1);
    	spawnNextProcSecs = localsecs + timeToNextProcSecs;
    	spawnNextProcNS = localns + timeToNextProcNS;
	if (spawnNextProcNS >= BILLION)
	{
		spawnNextProcSecs++;
		temp = spawnNextProcNS - BILLION;
		spawnNextProcNS = temp;
	}
	
	fprintf(fp, "OSS: Next user spawn scheduled for %ld:%09ld\n", 
        spawnNextProcSecs, spawnNextProcNS );
}

void spawnNewUser() 
{
    char str_pct_id[20]; 
    char str_user_sim_pid[4]; 
    int user_sim_pid, roll;
    pid_t childpid;
    setTimeToNextProc(); 
    user_sim_pid = getOpenBitVector();
    if (user_sim_pid == -1) {
        printf("OSS: Error in spawnNewUser: no open bitvector\n");
        detach();
        exit(0);
    }
    numProcsSpawned++;

    if (numProcsSpawned >= 10) {
        printf("OSS: 100 total users spawned. No new users will be "
                "generated after this one.\n");
        fprintf(fp, "OSS: 100 total users spawned. "
                "No new users will be generated after this one.\n");
        allowNewUsers = 0;
    }
    infostruct.msgtyp = user_sim_pid;
    infostruct.userTerminatingFlag = 0;
    infostruct.userUsedFullTimeSliceFlag = 0;
    infostruct.user_sim_pid = user_sim_pid;

    roll = roll1000();
    if (roll < 45) {
        printf("OSS: Rolled a real-time class user process.\n");
        infostruct.ossTimeSliceGivenNS = quantum_0;
        makePCB(user_sim_pid, 1);
        addProcToQueue(queue0, arraysize, user_sim_pid);
        pct[user_sim_pid].currentQueue = 0;
    }

    else {
        infostruct.ossTimeSliceGivenNS = quantum_1;
        makePCB(user_sim_pid, 0);
        addProcToQueue(queue1, arraysize, user_sim_pid);
        pct[user_sim_pid].currentQueue = 1;
    }
    fprintf(fp, "OSS: Generating process PID %d and putting it in queue "
            "%d at time %u:%09u, total spawned: %d\n", 
        pct[user_sim_pid].localPID, pct[user_sim_pid].currentQueue, 
        *simClock_secs, *simClock_ns, numProcsSpawned);
    fflush(fp);
    if ( (childpid = fork()) < 0 ){ 
        perror("OSS: Error forking user");
        fprintf(fp, "Fork error\n");
        detach();
        exit(0);
    }
    if (childpid == 0) { 
        sprintf(str_pct_id, "%d", shmid_pct); 
        sprintf(str_user_sim_pid, "%d", user_sim_pid);
        execlp("./user", "./user", str_pct_id, str_user_sim_pid, (char *)NULL);
        perror("OSS: execl() failure"); 
        exit(0);
    }
    childpids[user_sim_pid] = childpid;
}

int checkBlockedQueue() {
    int returnval = 0;
    int j;
    
    unsigned int localsec = *simClock_secs;
    unsigned int localns = *simClock_ns;
    for (j=1; j<arraysize; j++) {

        if (blocked[j] != 0) {

            if ( (localsec > pct[blocked[j]].blockedUntilSecs) || 
            ( (localsec >= pct[blocked[j]].blockedUntilSecs) && 
                    (localns >= pct[blocked[j]].blockedUntilNS) ) ) {
 
                awakenUser(blocked[j]);
                returnval++;
            }
        }
    }
    return returnval;
}

int isBlockedQueueEmpty() {
    if (blocked[1] != 0) {
        return 0;
    }
    return 1;
}

int incrementIdleTime(){
    unsigned int temp, localsec, localns, localsim_s, localsim_ns;
    localsec = 0;
    localsim_s = *simClock_secs;
    localsim_ns = *simClock_ns;
    if (localsim_s == spawnNextProcSecs) {
        localns = (spawnNextProcNS - localsim_ns);
    }
    else {
        localsec = (spawnNextProcSecs - localsim_s);
        localns = spawnNextProcNS + (BILLION - localsim_ns);
        localsec--;
    }
    if (localns >= BILLION) {
        localsec++;
        temp = localns - BILLION;
        localns = temp;
    }
    idleTime_secs = idleTime_secs + localsec; 
    idleTime_ns = idleTime_ns + localns;
    if (idleTime_ns >= BILLION) {
        idleTime_secs++;
        temp = idleTime_ns - BILLION;
        idleTime_ns = temp;
    }
    return 1;
}

int roll1000() {
    int return_val;
    return_val = rand_r(&seed) % (1000 + 1);
    return return_val;
}

int getOpenBitVector() {
    int i;
    int return_val = -1;
    for (i=1; i<19; i++) {
        if (bitVector[i] == 0) {
            return_val = i;
            break;
        }
    }
    return return_val;
}

void makePCB(int pidnum, int isRealTime) 
{
    unsigned int localsec = *simClock_secs;
    unsigned int localns = *simClock_ns;
    pct[pidnum].startTime_secs = 0;
    pct[pidnum].startTime_ns = 0;
    pct[pidnum].totalCPUtime_secs = 0;
    pct[pidnum].totalCPUtime_ns = 0;
    pct[pidnum].totalLIFEtime_secs = 0;
    pct[pidnum].totalLIFEtime_ns = 0;
    pct[pidnum].timeUsedLastBurst_ns = 0;
    pct[pidnum].blocked = 0;
    pct[pidnum].timesBlocked = 0;
    pct[pidnum].blockedUntilSecs = 0;
    pct[pidnum].blockedUntilNS = 0;
    pct[pidnum].totalBlockedTime_secs = 0;
    pct[pidnum].totalBlockedTime_ns = 0;
    pct[pidnum].localPID = pidnum; 
    pct[pidnum].isRealTimeClass = isRealTime;

    if (isRealTime == 1) {
        pct[pidnum].currentQueue = 0;
    }
    else {
        pct[pidnum].currentQueue = 1;
    }
    bitVector[pidnum] = 1;
}

void initQueue(int q[], int size) 
{
    int i;
    for(i=0; i<size; i++) {
        q[i] = 0;
    }
}

void initBitVector(int n) 
{
    int i;
    for (i=0; i<n; i++) {
        bitVector[i] = 0;
    }
}

void setUp() 
{
    printf("OSS: Allocating shared memory\n");
    
    shmid_pct = shmget(SHMKEY_pct, 19*sizeof(struct pcb), 0777 | IPC_CREAT);
     if (shmid_pct == -1) { 
            perror("OSS: error in shmget shmid_pct");
            exit(1);
        }
    pct = (struct pcb *)shmat(shmid_pct, 0, 0);
    if ( pct == (struct pcb *)(-1) ) {
        perror("OSS: error in shmat pct");
        exit(1);
    }   

    shmid_sim_secs = shmget(SHMKEY_sim_s, BUFF_SZ, 0777 | IPC_CREAT);
        if (shmid_sim_secs == -1) { 
            perror("OSS: error in shmget shmid_sim_secs");
            exit(1);
        }
    simClock_secs = (unsigned int*) shmat(shmid_sim_secs, 0, 0);

    shmid_sim_ns = shmget(SHMKEY_sim_ns, BUFF_SZ, 0777 | IPC_CREAT);
        if (shmid_sim_ns == -1) { 
            perror("OSS: error in shmget shmid_sim_ns");
            exit(1);
        }
    simClock_ns = (unsigned int*) shmat(shmid_sim_ns, 0, 0);
    

    if ( (oss_qid = msgget(MSGQKEY_oss, 0777 | IPC_CREAT)) == -1 ) {
        perror("Error generating communication message queue");
        exit(0);
    }
}

void detach() 
{
    printf("OSS: Clearing IPC resources...\n");

    if ( shmctl(shmid_sim_secs, IPC_RMID, NULL) == -1) {
        perror("error removing shared memory");
    }
    if ( shmctl(shmid_sim_ns, IPC_RMID, NULL) == -1) {
        perror("error removing shared memory");
    }

    if ( shmctl(shmid_pct, IPC_RMID, NULL) == -1) {
        perror("error removing shared memory");
    }

    if ( msgctl(oss_qid, IPC_RMID, NULL) == -1 ) {
        perror("OSS: Error removing ODD message queue");
        exit(0);
    }
}

void sigErrors(int signum)
{
        if (signum == SIGINT)
        {
                printf("\nSIGINT\n");
        }
        else
        {
                printf("\nSIGALRM\n");
        }
	
	detach();
        exit(0);
}
