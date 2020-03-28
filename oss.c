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
#include "queue.h"

void setUp(); //allocate shared memory
void detach(); //clear shared memory and message queues
void sigErrors(int signum);
void bitMapF(int n);
void nextPbegin();
void PCB(int pidnum, int realTime);
int randomTime();
void createChild();
int blockedCheck();
int blockedCheckTwo();
int incTime();
void getChild(int);
void incClock(int);
void term(int);
void block(int);
int getChildQ(int[]);
int checkTime();

unsigned int totalSec, totalNS;
unsigned int totalWaitSec, totalWaitNS;
unsigned int bWholeSec, bWholeNS;
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
int blocked[19];
int numProc = 0;
int newChild = 1;
int numChild = 0;
int numUnblocked;
pid_t pids[20];
int lines = 0;
int nextP, nextQ;

unsigned int cost0 = 2000000;
unsigned int cost1 = 4000000; 
unsigned int cost2 = 8000000;
unsigned int cost3 = 16000000;

char outputFileName[] = "log";
FILE* fp;

struct messageQueue msgstruct;

struct pcb * pcbinfo;


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
	
	alarm(3);
	
	while (1) 
	{
		/*if (realTimeGone < fullTime) 
		{
            		realTimeGone = (double)(time(NULL) - start);
            		if (realTimeGone >= fullTime) 
			{
                		newChild = 0;
            		}
        	}*/
       
        if (newChild == 0 && numChild == 0) 
	{
        	break;
        }

        numUnblocked = 0;
        if (blockedCheck() == 0) 
	{ 
        	numUnblocked = blockedCheckTwo();
        }

        nextQ = putQueue();
    
        if ( (nextQ == -1) && (newChild == 0) ) 
	{
        	nextPbegin();
        	incTime();
        	*clockSec = createSec;
        	*clockNS = createNS;
         
        	if (numUnblocked == 0) 
		{
            		finishThem = 1;
                	continue;
            	}
        }	
	else if ( nextQ == -1 && newChild == 1 ) 
	{
        	incTime();
        	*clockSec = createSec;
        	*clockNS = createNS;
        	if ( (newChild == 1) && (getBitSpot() != -1) ) 
		{
                	createChild();         
                	numChild++;
                	fprintf(fp, "OSS: Dispatching process PID %d from queue %d at time %u:%09u\n", msgstruct.sPid ,pcbinfo[msgstruct.sPid].currentQueue,*clockSec, *clockNS);
			lines++;
			if ( msgsnd(qid, &msgstruct, sizeof(msgstruct), 0) == -1 ) 
			{
                    		perror("OSS: error sending init msg");
                    		detach();
                    		exit(0);
                	}
            	}
        }
	else if ( finishThem == 1 && nextQ != -1) 
	{
        	finishThem = 0;
                if (nextQ == 0) 
		{
                	nextP = getChildQ(q0);
                	msgstruct.timeGivenNS = cost0;
                	queueDelete(q0, array, nextP);
                	addProcToQueue(q0, array, nextP);
                }
                else if (nextQ == 1) 
		{
                	nextP = getChildQ(q1);
                    	msgstruct.timeGivenNS = cost1;
                    	queueDelete(q1, array, nextP);
                    	addProcToQueue(q1, array, nextP);
                }
                
                if (nextP != -1) 
		{
                	msgstruct.msgTyp = (long) nextP;
                	msgstruct.termFlg = 0;
                    	msgstruct.timeFlg = 0;
                    	msgstruct.blockedFlg = 0;
                    	msgstruct.sPid = nextP;
                    	fprintf(fp, "OSS: Dispatching process PID %d from queue %d at time %u:%09u\n", nextP, pcbinfo[nextP].currentQueue, *clockSec, *clockNS);
			lines++;
			if ( msgsnd(qid, &msgstruct, sizeof(msgstruct), 0) == -1 ) 
			{
                        	perror("OSS: error sending user msg");
                        	detach();
                        	exit(0);
                    	}
                }
        }

	firstblocked = blocked[1]; 
 
        if ( msgrcv(qid, &msgstruct, sizeof(msgstruct), 99, 0) == -1 ) 
	{
        	perror("oss: msgrcv error");
            	detach();
            	exit(0);
        }
        blocked[1] = firstblocked; 
        pcbinfo[msgstruct.sPid].burstNS = msgstruct.burst;
        incClock(msgstruct.sPid);

	if (msgstruct.termFlg == 1) 
	{
		fprintf(fp, "OSS: Receiving that process PID %d ran for %u nanoseconds and then terminated\n", msgstruct.sPid,msgstruct.burst);
            	lines++;
		term(msgstruct.sPid);
        }
       
        else if (msgstruct.blockedFlg == 1) 
	{	
        	fprintf(fp, "OSS: Receiving that process PID %d ran for %u nanoseconds and then was blocked by an event. Moving to blocked queue\n", msgstruct.sPid, msgstruct.burst);
            	lines++;
		block(msgstruct.sPid);
        }
 
        else if (msgstruct.timeFlg == 0) 
	{
        	fprintf(fp, "OSS: Receiving that process PID %d ran for %u nanoseconds, not using its entire quantum", msgstruct.sPid,msgstruct.burst);
         
        	if (pcbinfo[msgstruct.sPid].currentQueue == 2) 
		{
                	fprintf(fp, ", moving to queue 1\n");
                	lines++;
			queueDelete(q2, array, msgstruct.sPid);
                	addProcToQueue(q1, array, msgstruct.sPid);
                	pcbinfo[msgstruct.sPid].currentQueue = 1;
            	}
            	else if (pcbinfo[msgstruct.sPid].currentQueue == 3) 
		{
                	fprintf(fp, ", moving to queue 1\n");
                	lines++;
			queueDelete(q3, array, msgstruct.sPid);
                	addProcToQueue(q1, array, msgstruct.sPid);
                	pcbinfo[msgstruct.sPid].currentQueue = 1;
            	}
            	else 
		{
                	fprintf(fp, "\n");
            	}
        }
	else if (msgstruct.timeFlg == 1) 
	{
        	fprintf(fp, "OSS: Receiving that process PID %d ran for %u nanoseconds", msgstruct.sPid,msgstruct.burst);
         	lines++;   
         	if (pcbinfo[msgstruct.sPid].currentQueue == 1) 
		{
                	fprintf(fp, ", moving to queue 2\n");
                	lines++;
			queueDelete(q1, array, msgstruct.sPid);
                	addProcToQueue(q2, array, msgstruct.sPid);
                	pcbinfo[msgstruct.sPid].currentQueue = 2;
            	}
         
            	else if (pcbinfo[msgstruct.sPid].currentQueue == 2) 
		{
                	fprintf(fp, ", moving to queue 3\n");
                	lines++;
			queueDelete(q2, array, msgstruct.sPid);
                	addProcToQueue(q3, array, msgstruct.sPid);
                	pcbinfo[msgstruct.sPid].currentQueue = 3;
            	}
            	else 
		{
                	fprintf(fp, "\n");
            	}
        }

        
        if (newChild == 1) 
	{
        	if (checkTime()) 
		{
                	if (getBitSpot() != -1) 
			{
                    		createChild();
                    		numChild++;
                	}
                else 
		{
                	nextPbegin();
                }
            	}
        }
	
	nextQ = putQueue();
        if (nextQ == 0) 
	{
        	nextP = getChildQ(q0);
            	msgstruct.timeGivenNS = cost0;
            	queueDelete(q0, array, nextP);
            	addProcToQueue(q0, array, nextP);
        }
        else if (nextQ == 1) 
	{
            	nextP = getChildQ(q1);
            	msgstruct.timeGivenNS = cost1;
            	queueDelete(q1, array, nextP);
            	addProcToQueue(q1, array, nextP);
        }
        else if (nextQ == 2) 
	{
            	nextP = getChildQ(q2);
            	msgstruct.timeGivenNS = cost2;
            	queueDelete(q2, array, nextP);
            	addProcToQueue(q2, array, nextP);
        }
        else if (nextQ == 3) 
	{
            	nextP = getChildQ(q3);
            	msgstruct.timeGivenNS = cost3;
            	queueDelete(q3, array, nextP);
            	addProcToQueue(q3, array, nextP);
        }
   
        else 
	{
            	continue;
        }

	if (nextP != -1) 
	{
            	msgstruct.msgTyp = (long) nextP;
            	msgstruct.termFlg = 0;
            	msgstruct.timeFlg = 0;
            	msgstruct.blockedFlg = 0;
            	msgstruct.sPid = nextP;
            	fprintf(fp, "OSS: Dispatching process PID %d from queue %d at time %u:%09u\n", nextP, pcbinfo[nextP].currentQueue, *clockSec, *clockNS);
            	lines++;
		if ( msgsnd(qid, &msgstruct, sizeof(msgstruct), 0) == -1 ) 
		{
                	perror("OSS: error sending init msg");
                	detach();
                	exit(0);
            	}
        }
	if (lines >= 10000)
	{
		detach();
		exit(0);
	}
    	}

	detach();

	return 0;
}

int checkTime() 
{
    	int rvalue = 0;
    	unsigned int localSec = *clockSec;
    	unsigned int localNS = *clockNS;
    	if ( (localSec > createSec) ||  ( (localSec >= createSec) && (localNS >= createNS) ) ) 
	{
        	rvalue = 1;
    	}
 
    	return rvalue;
}

void block(int blockpid) 
{
    	int temp;
    	unsigned int localSec, localNS;
    
    	bWholeSec += pcbinfo[blockpid].bWholeSec;
	bWholeNS += pcbinfo[blockpid].bWholeNS;
    	if (bWholeNS >= MAX) 
	{
        	bWholeSec++;
        	temp = bWholeNS - MAX;
        	bWholeNS = temp;
    	}
   
    	pcbinfo[blockpid].blocked = 1;
    	if (pcbinfo[blockpid].currentQueue == 0)
        {queueDelete(q0, array, blockpid);}
    	else if (pcbinfo[blockpid].currentQueue == 1)
        {queueDelete(q1, array, blockpid);}
    	else if (pcbinfo[blockpid].currentQueue == 2)
        {queueDelete(q2, array, blockpid);}
    	else if (pcbinfo[blockpid].currentQueue == 3)
        {queueDelete(q3, array, blockpid);}

    	addProcToQueue(blocked, array, blockpid);

    	localSec = *clockSec;
    	localNS = *clockNS;
    	temp = random();
    	if (temp < 100) temp = 10000;
    	else temp = temp * 100;
    	localNS = localNS + temp; 
    	fprintf(fp, "OSS: Time used to move user to blocked queue: %u nanoseconds\n", temp);
	lines++;	
	if (localNS >= MAX) 
	{
        	localSec++;
        	temp = localNS - MAX;
        	localNS = temp;
    	}
   
    	*clockSec = localSec;
    	*clockNS = localNS;
}

int getChildQ(int q[]) 
{
    	if (q[1] == 0) 
	{ 
        	return -1;
    	}
    	else return q[1];
}

void term(int termPid) 
{
    	int status;
    	unsigned int temp;
    	waitpid(msgstruct.userPid, &status, 0);
    
    	totalSec += pcbinfo[termPid].totalWholeSec;
    	totalNS += pcbinfo[termPid].totalWholeNS;
    	if (totalNS >= MAX) 
	{
        	totalSec++;
        	temp = totalNS - MAX;
        	totalNS = temp;
    	}
 	totalWaitSec += (pcbinfo[termPid].totalWholeSec - pcbinfo[termPid].totalSec);
    	totalWaitNS +=(pcbinfo[termPid].totalWholeNS - pcbinfo[termPid].totalNS);
    	if (totalWaitNS >= MAX) 
	{
        	totalWaitSec++;
        	temp = totalWaitNS - MAX;
        	totalWaitNS = temp;
    	}
    
    	if (pcbinfo[termPid].currentQueue == 0) 
	{
        	queueDelete(q0, array, termPid);
    	}
    	else if (pcbinfo[termPid].currentQueue == 1) 
	{
        	queueDelete(q1, array, termPid);
    	}
    	else if (pcbinfo[termPid].currentQueue == 2) 
	{
        	queueDelete(q2, array, termPid);
    	}
    	else if (pcbinfo[termPid].currentQueue == 3) 
	{
        	queueDelete(q3, array, termPid);
    	}
    	bitMap[termPid] = 0;
    	numChild--;
    	pcbinfo[termPid].totalSec = 0;
    	pcbinfo[termPid].totalNS = 0;
    	pcbinfo[termPid].totalWholeSec = 0;
    	pcbinfo[termPid].totalWholeNS = 0;
	fprintf(fp,"OSS: User %d has terminated\n",msgstruct.sPid);
	lines++;
}

void incClock(int childPid) 
{
    	unsigned int localsec = *clockSec;
    	unsigned int localns = *clockNS;
    	unsigned int temp;
    	temp = random();
    	if (temp < 100) temp = 100;
    	else temp = temp * 10;
    	localns = localns + temp; 
	fprintf(fp, "OSS: Time spent this dispatch: %ld nanoseconds\n", temp);
    	lines++;
	localns = localns + pcbinfo[childPid].burstNS;
    
    	if (localns >= MAX) 
	{
        	localsec++;
        	temp = localns - MAX;
        	localns = temp;
    	}
    
    	*clockSec = localsec;
    	*clockNS = localns;
}

void getChild(int wakepid) 
{
    	unsigned int localsec, localns, temp;

    	queueDelete(blocked, array, wakepid);

    	pcbinfo[wakepid].blocked = 0;
    	pcbinfo[wakepid].bNS = 0;
    	pcbinfo[wakepid].bSec = 0; 

    	if (pcbinfo[wakepid].realTimeC == 1) 
	{
        	addProcToQueue(q0, array, wakepid);
        	pcbinfo[wakepid].currentQueue = 0;
    	}
    
    	else 
	{
        	addProcToQueue(q1, array, wakepid);
        	pcbinfo[wakepid].currentQueue = 1;
    	}

    	localsec = *clockSec;
    	localns = *clockNS;
    	temp = random();
    	if (temp < 100) temp = 10000;
    	else temp = temp * 100;
    	localns = localns + temp; 
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
	if (createNS >= MAX)
	{
		createSec++;
		temp = createNS - MAX;
		createNS = temp;
	}
}

void createChild() 
{
    	char id[20]; 
    	char childSPid[4]; 
    	int sPid, rand;
    	pid_t childpid;
    	nextPbegin(); 
    	sPid = getBitSpot();
    	if (sPid == -1) 
	{
        	detach();
        	exit(0);
    	}
   	numProc++;

    	if (numProc >= 100) 
	{
        	newChild = 0;
    	}
    	msgstruct.msgTyp = sPid;
    	msgstruct.termFlg = 0;
    	msgstruct.timeFlg = 0;
    	msgstruct.sPid = sPid;

    	rand = random();
    	if (rand < 45) 
	{
        	msgstruct.timeGivenNS = cost0;
        	PCB(sPid, 1);
        	addProcToQueue(q0, array, sPid);
        	pcbinfo[sPid].currentQueue = 0;
    	}

    	else 
	{
        	msgstruct.timeGivenNS = cost1;
        	PCB(sPid, 0);
        	addProcToQueue(q1, array, sPid);
        	pcbinfo[sPid].currentQueue = 1;
    	}
    	fprintf(fp, "OSS: Generating process PID %d and putting it in queue %d at time %u:%09u\n", pcbinfo[sPid].localPid, pcbinfo[sPid].currentQueue, *clockSec, *clockNS);
	lines++;
	fflush(fp);
    	if ( (childpid = fork()) < 0 )
	{ 
        	perror("OSS: Error forking user");
        	fprintf(fp, "Fork error\n");
        	detach();
        	exit(0);
    	}
    	if (childpid == 0) 
	{ 
        	sprintf(id, "%d", shmid); 
        	sprintf(childSPid, "%d", sPid);
       		execlp("./user", "./user", id, childSPid, (char *)NULL);
        	perror("OSS: execl() failure"); 
        	exit(0);
    	}
    	pids[sPid] = childpid;
}

int blockedCheckTwo() 
{
    	int returnval = 0;
    	int j;
    
    	unsigned int localsec = *clockSec;
    	unsigned int localns = *clockNS;
    	for (j=1; j<array; j++) 
	{

        	if (blocked[j] != 0) 
		{

            		if ( (localsec > pcbinfo[blocked[j]].bSec) || ( (localsec >= pcbinfo[blocked[j]].bSec) && (localns >= pcbinfo[blocked[j]].bNS) ) ) 
			{
                		getChild(blocked[j]);
                		returnval++;
            		}
        	}
    	}
    	return returnval;
}

int blockedCheck() 
{
    	if (blocked[1] != 0) 
	{
        	return 0;
    	}
    	return 1;
}

int incTime()
{
    	unsigned int temp, localsec, localns, localsim_s, localsim_ns;
    	localsec = 0;
   	localsim_s = *clockSec;
    	localsim_ns = *clockNS;
    	if (localsim_s == createSec) 
	{
        	localns = (createNS - localsim_ns);
    	}
    	else 
	{
        	localsec = (createSec - localsim_s);
        	localns = createNS + (MAX - localsim_ns);
        	localsec--;
    	}
    	if (localns >= MAX) 
	{
        	localsec++;
        	temp = localns - MAX;
        	localns = temp;
    	}
    	stopSec = stopSec + localsec; 
    	stopNS = stopNS + localns;
    	if (stopNS >= MAX) 
	{
        	stopSec++;
        	temp = stopNS - MAX;
        	stopNS = temp;
    	}
    	return 1;
}

int randomTime() 
{
    	int rvalue;
    	rvalue = rand_r(&begin) % (1000 + 1);
    	return rvalue;
}

void PCB(int pidnum, int realTime) 
{
    	unsigned int localSec = *clockSec;
    	unsigned int localNS = *clockNS;
    	pcbinfo[pidnum].startSec = 0;
    	pcbinfo[pidnum].startNS = 0;
    	pcbinfo[pidnum].totalSec = 0;
    	pcbinfo[pidnum].totalNS = 0;
    	pcbinfo[pidnum].totalWholeSec = 0;
    	pcbinfo[pidnum].totalWholeNS = 0;
    	pcbinfo[pidnum].burstNS = 0;
    	pcbinfo[pidnum].blocked = 0;
    	pcbinfo[pidnum].bTimes = 0;
    	pcbinfo[pidnum].bSec = 0;
    	pcbinfo[pidnum].bNS = 0;
    	pcbinfo[pidnum].bWholeSec = 0;
    	pcbinfo[pidnum].bWholeNS = 0;
    	pcbinfo[pidnum].localPid = pidnum; 
    	pcbinfo[pidnum].realTimeC = realTime;

    	if (realTime == 1) 
	{
        	pcbinfo[pidnum].currentQueue = 0;
    	}
    	else 
	{
        	pcbinfo[pidnum].currentQueue = 1;
    	}
    	bitMap[pidnum] = 1;
}

void setUp() 
{   
    	shmid = shmget(SHMKEY_pct, 19*sizeof(struct pcb), 0777 | IPC_CREAT);
     	if (shmid == -1) 
	{ 
            	perror("oss: shmget error");
            	exit(1);
        }
    	pcbinfo = (struct pcb *)shmat(shmid, 0, 0);
    	if ( pcbinfo == (struct pcb *)(-1) ) 
	{
        	perror("oss: shmat error");
        	exit(1);
    	}   

    	smSec = shmget(SHMKEY_sim_s, BUFF_SZ, 0777 | IPC_CREAT);
        if (smSec == -1) 
	{ 
            	perror("oss: shmget error");
            	exit(1);
        }
    	clockSec = (unsigned int*) shmat(smSec, 0, 0);
    	smNS = shmget(SHMKEY_sim_ns, BUFF_SZ, 0777 | IPC_CREAT);
        if (smNS == -1) 
	{ 
            	perror("oss: shmget error");
            	exit(1);
        }
    	clockNS = (unsigned int*) shmat(smNS, 0, 0);
    	if ( (qid = msgget(MSGQKEY_oss, 0777 | IPC_CREAT)) == -1 ) 
	{
        	perror("oss: msgget error");
        	exit(0);
    	}
}

void detach() 
{
    	shmctl(smSec, IPC_RMID, NULL);
    	shmctl(smNS, IPC_RMID, NULL);
    	shmctl(shmid, IPC_RMID, NULL);
    	msgctl(qid, IPC_RMID, NULL);
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
