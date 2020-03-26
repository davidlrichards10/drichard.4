/*
 * Name: David Richards
 * Assignment: CS4760 Project 4
 * Date: Tue March 24th
 * File: oss.c (main file)
 */

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
void bitMapF(int n);
void queue(int[], int);
void nextPbegin();
void PCB(int pidnum, int realTime);
int getBitSpot();
int roll1000();
int putQueue();
void createChild();
int blockedCheck();
int blockedCheckTwo();
int incTime();
void getChild(int);
int queueDelete(int[], int, int);
void incClock(int);
void term(int);
void block(int);
int getChildQ(int[]);
int checkTime();

unsigned int totalSec, totalNS;
unsigned int totalWaitSec, totalWaitNS;
unsigned int totalBlockedTime_secs, totalBlockedTime_ns;
int smSec, smNS;
unsigned int stopSec = 0;
unsigned int stopNS = 0;
int shmid;
int qid;
int array = 19;
unsigned int maxNS, maxSec;
unsigned int nextProcNS, nextProcSec, begin;
unsigned int createNS, createSec;
static unsigned int *clockSec; //pointer to shm sim clock (seconds)
static unsigned int *clockNS; //pointer to shm sim clock (nanoseconds)
int bitMap[19];
int blocked[19];
int numProc = 0;
int newChild = 1;
int numChild = 0;
int numUnblocked;
pid_t pids[20];
int lines = 0;
int nextP, nextQ;

int q0[19]; 
int q1[19]; 
int q2[19]; 
int q3[19]; 

unsigned int cost0 = 2000000;
unsigned int cost1 = 4000000; 
unsigned int cost2 = 8000000;
unsigned int cost3 = 16000000;

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
   	double realTimeGone = 0;
    	double fullTime = 10; 
    	maxNS = 999999998;
    	maxSec = 0;
    	begin = (unsigned int) getpid(); 
    	pid_t childpid; 
    	char id[20]; 
    	char childSPid[4]; 
   	int sPid;
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
	
	bitMapF(array);
    	queue(q0, array);
    	queue(q1, array);
    	queue(q2, array);
    	queue(q3, array);
    	queue(blocked, array);
    
   	nextPbegin();
	
	while (1) 
	{
		    if (realTimeGone < fullTime) {
            realTimeGone = (double)(time(NULL) - start);
            if (realTimeGone >= fullTime) {
                newChild = 0;
            }
        }
       
        if (newChild == 0 && numChild == 0) {
            break;
        }

        numUnblocked = 0;
        if (blockedCheck() == 0) { 
            numUnblocked = blockedCheckTwo();
        }


        nextQ = putQueue();
    
        if ( (nextQ == -1) && (newChild == 0) ) {
            nextPbegin();
            incTime();
            *clockSec = createSec;
            *clockNS = createNS;
         
            if (numUnblocked == 0) {
                finishThem = 1;
                continue;
            		}
        }	
	else if ( nextQ == -1 && newChild == 1 ) {
            incTime();
            *clockSec = createSec;
            *clockNS = createNS;
            fprintf(fp, "%u:%09u\n", *clockSec, *clockNS);
            fflush(fp);
            if ( (newChild == 1) && (getBitSpot() != -1) ) {
                createChild(); 
                                
                numChild++;
                fprintf(fp, "OSS: Dispatching process PID %d from queue %d at "
                    "time %u:%09u\n", infostruct.sPid ,
                        pct[infostruct.sPid].currentQueue,
                        *clockSec, *clockNS);
                if ( msgsnd(qid, &infostruct, sizeof(infostruct), 0) == -1 ) {
                    perror("OSS: error sending init msg");
                    detach();
                    exit(0);
                }
            }
        }
	else if ( finishThem == 1 && nextQ != -1) {
            finishThem = 0;
                if (nextQ == 0) {
                    nextP = getChildQ(q0);
                    infostruct.ossTimeSliceGivenNS = cost0;
                    queueDelete(q0, array, nextP);
                    addProcToQueue(q0, array, nextP);
                }
                else if (nextQ == 1) {
                    nextP = getChildQ(q1);
                    infostruct.ossTimeSliceGivenNS = cost1;
                    queueDelete(q1, array, nextP);
                    addProcToQueue(q1, array, nextP);
                }
                
                if (nextP != -1) {
                    infostruct.msgtyp = (long) nextP;
                    infostruct.userTerminatingFlag = 0;
                    infostruct.userUsedFullTimeSliceFlag = 0;
                    infostruct.userBlockedFlag = 0;
                    infostruct.sPid = nextP;
                    fprintf(fp, "OSS: Dispatching process PID %d from queue %d at "
                    "time %u:%09u\n", nextP, 
                    pct[nextP].currentQueue, 
                    *clockSec, *clockNS);
                    if ( msgsnd(qid, &infostruct, sizeof(infostruct), 0) == -1 ) {
                        perror("OSS: error sending user msg");
                        detach();
                        exit(0);
                    }
                }
        }

	 firstblocked = blocked[1]; 
 
        if ( msgrcv(qid, &infostruct, sizeof(infostruct), 99, 0) == -1 ) {
            perror("User: error in msgrcv");
            detach();
            exit(0);
        }
        blocked[1] = firstblocked; 
        

        pct[infostruct.sPid].timeUsedLastBurst_ns = 
                infostruct.userTimeUsedLastBurst;
        
        incClock(infostruct.sPid);

	 if (infostruct.userTerminatingFlag == 1) {
            fprintf(fp, "OSS: Receiving that process PID %d ran for %u"
                    " nanoseconds and "
                    "then terminated\n", infostruct.sPid,
                    infostruct.userTimeUsedLastBurst);
            term(infostruct.sPid);
        }
       
        else if (infostruct.userBlockedFlag == 1) {
            fprintf(fp, "OSS: Receiving that process PID %d ran for %u "
                    "nanoseconds and "
                    "then was blocked by an event. Moving to blocked queue\n", 
                    infostruct.sPid, infostruct.userTimeUsedLastBurst);
            block(infostruct.sPid);
        }
        
        else if (infostruct.userUsedFullTimeSliceFlag == 0) {
            fprintf(fp, "OSS: Receiving that process PID %d ran for %u "
                    "nanoseconds, not "
                    "using its entire quantum", infostruct.sPid,
                    infostruct.userTimeUsedLastBurst);
         
            if (pct[infostruct.sPid].currentQueue == 2) {
                fprintf(fp, ", moving to queue 1\n");
                queueDelete(q2, array, infostruct.sPid);
                addProcToQueue(q1, array, infostruct.sPid);
                pct[infostruct.sPid].currentQueue = 1;
            }
            else if (pct[infostruct.sPid].currentQueue == 3) {
                fprintf(fp, ", moving to queue 1\n");
                queueDelete(q3, array, infostruct.sPid);
                addProcToQueue(q1, array, infostruct.sPid);
                pct[infostruct.sPid].currentQueue = 1;
            }
            else {
                fprintf(fp, "\n");
            }
        }
	  else if (infostruct.userUsedFullTimeSliceFlag == 1) {
            fprintf(fp, "OSS: Receiving that process PID %d ran for %u "
                    "nanoseconds", 
                    infostruct.sPid,
                    infostruct.userTimeUsedLastBurst);
            
            if (pct[infostruct.sPid].currentQueue == 1) {
                fprintf(fp, ", moving to queue 2\n");
                queueDelete(q1, array, infostruct.sPid);
                addProcToQueue(q2, array, infostruct.sPid);
                pct[infostruct.sPid].currentQueue = 2;
            }
         
            else if (pct[infostruct.sPid].currentQueue == 2) {
                fprintf(fp, ", moving to queue 3\n");
                queueDelete(q2, array, infostruct.sPid);
                addProcToQueue(q3, array, infostruct.sPid);
                pct[infostruct.sPid].currentQueue = 3;
            }
            else {
                fprintf(fp, "\n");
            }
        }
        

        if (newChild == 1) {
            if (checkTime()) {
                if (getBitSpot() != -1) {
                    createChild();
                    numChild++;
                }
                else {
                    nextPbegin();
                }
            }
        }
	
	      nextQ = putQueue();
        if (nextQ == 0) {
            nextP = getChildQ(q0);
            infostruct.ossTimeSliceGivenNS = cost0;
            queueDelete(q0, array, nextP);
            addProcToQueue(q0, array, nextP);
        }
        else if (nextQ == 1) {
            nextP = getChildQ(q1);
            infostruct.ossTimeSliceGivenNS = cost1;
            queueDelete(q1, array, nextP);
            addProcToQueue(q1, array, nextP);
        }
        else if (nextQ == 2) {
            nextP = getChildQ(q2);
            infostruct.ossTimeSliceGivenNS = cost2;
            queueDelete(q2, array, nextP);
            addProcToQueue(q2, array, nextP);
        }
        else if (nextQ == 3) {
            nextP = getChildQ(q3);
            infostruct.ossTimeSliceGivenNS = cost3;
            queueDelete(q3, array, nextP);
            addProcToQueue(q3, array, nextP);
        }
   
        else {
            continue;
        }

	if (nextP != -1) {
            infostruct.msgtyp = (long) nextP;
            infostruct.userTerminatingFlag = 0;
            infostruct.userUsedFullTimeSliceFlag = 0;
            infostruct.userBlockedFlag = 0;
            infostruct.sPid = nextP;
            fprintf(fp, "OSS: Dispatching process PID %d from queue %d at "
                    "time %u:%09u\n", nextP, 
                    pct[nextP].currentQueue, 
                    *clockSec, *clockNS);
            if ( msgsnd(qid, &infostruct, sizeof(infostruct), 0) == -1 ) {
                perror("OSS: error sending init msg");
                detach();
                exit(0);
            }
        }
    
    }

	detach();

	return 0;
}

int checkTime() {
    int rvalue = 0;
    unsigned int localSec = *clockSec;
    unsigned int localNS = *clockNS;
    if ( (localSec > createSec) || 
            ( (localSec >= createSec) && (localns >= createNS) ) ) {
        rvalue = 1;
    }
 
    return rvalue;
}

void block(int blockpid) {
    int temp;
    unsigned int localSec, localNS;
    
    totalBlockedTime_secs += pct[blockpid].totalBlockedTime_secs;
    totalBlockedTime_ns += pct[blockpid].totalBlockedTime_ns;
    if (totalBlockedTime_ns >= BILLION) {
        totalBlockedTime_secs++;
        temp = totalBlockedTime_ns - BILLION;
        totalBlockedTime_ns = temp;
    }
   
    pct[blockpid].blocked = 1;
    if (pct[blockpid].currentQueue == 0)
        {queueDelete(q0, array, blockpid);}
    else if (pct[blockpid].currentQueue == 1)
        {queueDelete(q1, array, blockpid);}
    else if (pct[blockpid].currentQueue == 2)
        {queueDelete(q2, array, blockpid);}
    else if (pct[blockpid].currentQueue == 3)
        {queueDelete(q3, array, blockpid);}

    addProcToQueue(blocked, array, blockpid);

    localSec = *clockSec;
    localNS = *clockNS;
    temp = roll1000();
    if (temp < 100) temp = 10000;
    else temp = temp * 100;
    localns = localns + temp; 
    fprintf(fp, "OSS: Time used to move user to blocked queue: %u "
            "nanoseconds\n", temp);
    if (localNS >= BILLION) {
        localSec++;
        temp = localNS - BILLION;
        localns = temp;
    }
   
    *clockSec = localsec;
    *clockNS = localns;
}

int getChildQ(int q[]) {
    if (q[1] == 0) { 
        return -1;
    }
    else return q[1];
}

void term(int termPid) {
    int status;
    unsigned int temp;
    waitpid(infostruct.user_sys_pid, &status, 0);
    
    totalSec += pct[termPid].totalLIFEtime_secs;
    totalNS += pct[termPid].totalLIFEtime_ns;
    if (totalNS >= BILLION) {
        totalSec++;
        temp = totalNS - BILLION;
        totalNS = temp;
    }

    
    totalWaitSec += 
            (pct[termPid].totalLIFEtime_secs - pct[termPid].totalCPUtime_secs);
    totalWaitNS +=
            (pct[termPid].totalLIFEtime_ns - pct[termPid].totalCPUtime_ns);
    if (totalWaitNS >= BILLION) {
        totalWaitSec++;
        temp = totalWaitNS - BILLION;
        totalWaitNS = temp;
    }
    
    
    if (pct[termPid].currentQueue == 0) {
        queueDelete(q0, array, termPid);
    }
    else if (pct[termPid].currentQueue == 1) {
        queueDelete(q1, array, termPid);
    }
    else if (pct[termPid].currentQueue == 2) {
        queueDelete(q2, array, termPid);
    }
    else if (pct[termPid].currentQueue == 3) {
        queueDelete(q3, array, termPid);
    }
    bitMap[termPid] = 0;
    numChild--;
    pct[termPid].totalCPUtime_secs = 0;
    pct[termPid].totalCPUtime_ns = 0;
    pct[termPid].totalLIFEtime_secs = 0;
    pct[termPid].totalLIFEtime_ns = 0;
    fprintf(fp,"OSS: User %d has terminated\n",
        infostruct.sPid);
}

void incClock(int childPid) {
    unsigned int localsec = *clockSec;
    unsigned int localns = *clockNS;
    unsigned int temp;
    temp = roll1000();
    if (temp < 100) temp = 100;
    else temp = temp * 10;
    localns = localns + temp; 
    fprintf(fp, "OSS: Time spent this dispatch: %ld nanoseconds\n", temp);
    
    localns = localns + pct[childPid].timeUsedLastBurst_ns;
    
    if (localns >= BILLION) {
        localsec++;
        temp = localns - BILLION;
        localns = temp;
    }
    
    *clockSec = localsec;
    *clockNS = localns;
}

int queueDelete(int q[], int arrsize, int pNum) {
    int i;
    for (i=1; i<arrsize; i++) {
        if (q[i] == pNum) { 
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

void getChild(int wakepid) {
    unsigned int localsec, localns, temp;

    queueDelete(blocked, array, wakepid);

    pct[wakepid].blocked = 0;
    pct[wakepid].blockedUntilNS = 0;
    pct[wakepid].blockedUntilSecs = 0; 

    if (pct[wakepid].isRealTimeClass == 1) {
        addProcToQueue(q0, array, wakepid);
        pct[wakepid].currentQueue = 0;
    }
    
    else {
        addProcToQueue(q1, array, wakepid);
        pct[wakepid].currentQueue = 1;
    }

    localsec = *clockSec;
    localns = *clockNS;
    temp = roll1000();
    if (temp < 100) temp = 10000;
    else temp = temp * 100;
    localns = localns + temp; 
}

int putQueue() {
    if (q0[1] != 0) {
        return 0;
    }
    else if (q1[1] != 0) {
        return 1;
    }
    else if (q2[1] != 0) {
        return 2;
    }
    else if (q3[1] != 0) {
        return 3;
    }
    else return -1;
}

int addProcToQueue (int q[], int arrsize, int pNum) {
    int i;
    for (i=1; i<arrsize; i++) {
        if (q[i] == 0) { 
            q[i] = pNum;
            return 1;
        }
    }
    return -1; 
}

void nextPbegin()
{
	unsigned int temp;
	unsigned int localsecs = *clockSec;
	unsigned int localns = *clockNS;
	nextProcSec = rand_r(&begin) % (maxSec + 1);
    	nextProcNS = rand_r(&begin) % (maxNS + 1);
    	createSec = localsecs + nextProcSec;
    	createNS = localns + nextProcNS;
	if (createNS >= BILLION)
	{
		createSec++;
		temp = createNS - BILLION;
		createNS = temp;
	}
}

void createChild() 
{
    char id[20]; 
    char childSPid[4]; 
    int sPid, roll;
    pid_t childpid;
    nextPbegin(); 
    sPid = getBitSpot();
    if (sPid == -1) {
        detach();
        exit(0);
    }
    numProc++;

    if (numProc >= 10) {
        newChild = 0;
    }
    infostruct.msgtyp = sPid;
    infostruct.userTerminatingFlag = 0;
    infostruct.userUsedFullTimeSliceFlag = 0;
    infostruct.sPid = sPid;

    roll = roll1000();
    if (roll < 45) {
        infostruct.ossTimeSliceGivenNS = cost0;
        PCB(sPid, 1);
        addProcToQueue(q0, array, sPid);
        pct[sPid].currentQueue = 0;
    }

    else {
        infostruct.ossTimeSliceGivenNS = cost1;
        PCB(sPid, 0);
        addProcToQueue(q1, array, sPid);
        pct[sPid].currentQueue = 1;
    }
    fprintf(fp, "OSS: Generating process PID %d and putting it in queue "
            "%d at time %u:%09u, total spawned: %d\n", 
        pct[sPid].localPID, pct[sPid].currentQueue, 
        *clockSec, *clockNS, numProc);
    fflush(fp);
    if ( (childpid = fork()) < 0 ){ 
        perror("OSS: Error forking user");
        fprintf(fp, "Fork error\n");
        detach();
        exit(0);
    }
    if (childpid == 0) { 
        sprintf(id, "%d", shmid); 
        sprintf(childSPid, "%d", sPid);
        execlp("./user", "./user", id, childSPid, (char *)NULL);
        perror("OSS: execl() failure"); 
        exit(0);
    }
    pids[sPid] = childpid;
}

int blockedCheckTwo() {
    int returnval = 0;
    int j;
    
    unsigned int localsec = *clockSec;
    unsigned int localns = *clockNS;
    for (j=1; j<array; j++) {

        if (blocked[j] != 0) {

            if ( (localsec > pct[blocked[j]].blockedUntilSecs) || 
            ( (localsec >= pct[blocked[j]].blockedUntilSecs) && 
                    (localns >= pct[blocked[j]].blockedUntilNS) ) ) {
 
                getChild(blocked[j]);
                returnval++;
            }
        }
    }
    return returnval;
}

int blockedCheck() {
    if (blocked[1] != 0) {
        return 0;
    }
    return 1;
}

int incTime(){
    unsigned int temp, localsec, localns, localsim_s, localsim_ns;
    localsec = 0;
    localsim_s = *clockSec;
    localsim_ns = *clockNS;
    if (localsim_s == createSec) {
        localns = (createNS - localsim_ns);
    }
    else {
        localsec = (createSec - localsim_s);
        localns = createNS + (BILLION - localsim_ns);
        localsec--;
    }
    if (localns >= BILLION) {
        localsec++;
        temp = localns - BILLION;
        localns = temp;
    }
    stopSec = stopSec + localsec; 
    stopNS = stopNS + localns;
    if (stopNS >= BILLION) {
        stopSec++;
        temp = stopNS - BILLION;
        stopNS = temp;
    }
    return 1;
}

int roll1000() {
    int rvalue;
    rvalue = rand_r(&begin) % (1000 + 1);
    return rvalue;
}

int getBitSpot() {
    int i;
    int rvalue = -1;
    for (i=1; i<19; i++) {
        if (bitMap[i] == 0) {
            rvalue = i;
            break;
        }
    }
    return rvalue;
}

void PCB(int pidnum, int realTime) 
{
    unsigned int localSec = *clockSec;
    unsigned int localNS = *clockNS;
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
    pct[pidnum].isRealTimeClass = realTime;

    if (realTime == 1) {
        pct[pidnum].currentQueue = 0;
    }
    else {
        pct[pidnum].currentQueue = 1;
    }
    bitMap[pidnum] = 1;
}

void queue(int q[], int size) 
{
    int i;
    for(i=0; i<size; i++) {
        q[i] = 0;
    }
}

void bitMapF(int n) 
{
    int i;
    for (i=0; i<n; i++) {
        bitMap[i] = 0;
    }
}

void setUp() 
{   
    shmid = shmget(SHMKEY_pct, 19*sizeof(struct pcb), 0777 | IPC_CREAT);
     if (shmid == -1) { 
            perror("OSS: error in shmget");
            exit(1);
        }
    pct = (struct pcb *)shmat(shmid, 0, 0);
    if ( pct == (struct pcb *)(-1) ) {
        perror("OSS: error in shmat");
        exit(1);
    }   

    smSec = shmget(SHMKEY_sim_s, BUFF_SZ, 0777 | IPC_CREAT);
        if (smSec == -1) { 
            perror("OSS: error in shmget");
            exit(1);
        }
    clockSec = (unsigned int*) shmat(smSec, 0, 0);

    smNS = shmget(SHMKEY_sim_ns, BUFF_SZ, 0777 | IPC_CREAT);
        if (smNS == -1) { 
            perror("OSS: error in shmget");
            exit(1);
        }
    clockNS = (unsigned int*) shmat(smNS, 0, 0);
    

    if ( (qid = msgget(MSGQKEY_oss, 0777 | IPC_CREAT)) == -1 ) {
        perror("Error generating communication message queue");
        exit(0);
    }
}

void detach() 
{
    if ( shmctl(smSec, IPC_RMID, NULL) == -1) {
        perror("error removing shared memory");
    }
    if ( shmctl(smNS, IPC_RMID, NULL) == -1) {
        perror("error removing shared memory");
    }

    if ( shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("error removing shared memory");
    }

    if ( msgctl(qid, IPC_RMID, NULL) == -1 ) {
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
