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
void printStats();

unsigned int totalSec = 0;
unsigned int totalNS = 0;
unsigned int totalWaitSec = 0; 
unsigned int totalWaitNS = 0;
unsigned int bWholeSec;
unsigned int bWholeNS;
int smSec, smNS;
unsigned int stopSec = 0;
unsigned int stopNS = 0;
int shmid;
int qid;
int array = 19;
unsigned int maxNS, maxSec;
unsigned int nextProcNS, nextProcSec, begin;
unsigned int createNS, createSec;
static unsigned int *clockSec;
static unsigned int *clockNS; 
int blocked[19];
int numProc = 0;
int newChild = 1;
int numChild = 0;
int numUnblocked;
pid_t pids[20];
int lines = 0;
int nextP, nextQ;

/* Set values for quantum for all four queues */
const static unsigned int cost0 = 10  * 1000000;
const static unsigned int cost1 = 10 * 2 * 1000000;
const static unsigned int cost2 = 10 * 3 * 1000000;
const static unsigned int cost3 = 10 * 4 * 1000000;

/* Default output file */
char outputFileName[] = "log";
FILE* fp;

/* Contains message queue and pcb info from shared.h */
struct messageQueue msgstruct;
struct pcb * pcbinfo;


int main(int argc, char** argv) 
{
	int c;
	
	/* Using getopt to parse command line options */
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
    	maxNS = 1000000000;
    	maxSec = 5;
    	begin = (unsigned int) getpid(); 
    	pid_t childpid; 
    	char id[20]; 
    	char childSPid[4]; 
   	int sPid;
    	int firstblocked;	

	/* Catch ctrl-c and 3 second alarm interupts */
	if (signal(SIGINT, sigErrors) == SIG_ERR) //sigerror on ctrl-c
        {
                exit(0);
        }

        if (signal(SIGALRM, sigErrors) == SIG_ERR) //sigerror on program exceeding 3 second alarm
        {
                exit(0);
        }

	/* Set up shared memory */
	setUp();
	
	/* Set up queues and bit vector */
	bitMapF(array);
    	createQueue(q0, array);
    	createQueue(q1, array);
    	createQueue(q2, array);
    	createQueue(q3, array);
    	createQueue(blocked, array);
    
	/* Schedule the first process */
   	nextPbegin();
	
	alarm(3);
	
	while (1) 
	{

	/* Exit loop once 100 are generated */
	if (newChild == 0 && numChild == 0) 
	{
        	break;
        }

	/* Check the blocked queue for users needing to be generated */
        numUnblocked = 0;
        if (blockedCheck() == 0) 
	{ 
        	numUnblocked = blockedCheckTwo();
        }

	/* Get the highest priority member, advance clock and get ready to spawn new member */
        nextQ = checkQueue();
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
	
	/* Adance clock and spawn/schedule new member if the queues are empty */	
	else if ( nextQ == -1 && newChild == 1 ) 
	{
        	incTime();
        	*clockSec = createSec;
        	*clockNS = createNS;
        	if ( (newChild == 1) && (getBitSpot() != -1) ) 
		{
                	createChild();         
                	numChild++;
			if(lines<10000)
			{
                	fprintf(fp, "OSS: Dispatching process PID %d from queue %d at time %u:%09u\n", msgstruct.sPid ,pcbinfo[msgstruct.sPid].currentQueue,*clockSec, *clockNS);
			lines++;
			}
			if ( msgsnd(qid, &msgstruct, sizeof(msgstruct), 0) == -1 ) 
			{
                    		perror("OSS: error sending init msg");
                    		detach();
                    		exit(0);
                	}
            	}
        }

	/* Wait for a message from a member */
	firstblocked = blocked[1]; 
        if ( msgrcv(qid, &msgstruct, sizeof(msgstruct), 99, 0) == -1 ) 
	{
        	perror("oss: msgrcv error");
            	detach();
            	exit(0);
        }
        blocked[1] = firstblocked; 
        
	/* Store the burst time in pcb and advance clock */
	pcbinfo[msgstruct.sPid].burstNS = msgstruct.burst;
        incClock(msgstruct.sPid);

	/* If member terminated, remove from the queue */
	if (msgstruct.termFlg == 1) 
	{
		if(lines<10000)
                {
		fprintf(fp, "OSS: Receiving that process PID %d ran for %u nanoseconds and then terminated\n", msgstruct.sPid,msgstruct.burst);
            	lines++;
		}
		term(msgstruct.sPid);
        }
       
	/* Means that the member was blocked */
        else if (msgstruct.blockedFlg == 1) 
	{	
		if(lines<10000)
                {

        	fprintf(fp, "OSS: Receiving that process PID %d ran for %u nanoseconds and then was blocked by an event. Moving to blocked queue\n", msgstruct.sPid, msgstruct.burst);
            	lines++;
		}
		block(msgstruct.sPid);
        }
 
	/* The process did not finish its allocated time slice, move to appropriate queue */
        else if (msgstruct.timeFlg == 0) 
	{
		if(lines<10000)
                {
        	fprintf(fp, "OSS: Receiving that process PID %d ran for %u nanoseconds, not using its entire quantum", msgstruct.sPid,msgstruct.burst);
         	lines++;
		}
        	if (pcbinfo[msgstruct.sPid].currentQueue == 2) 
		{
			if(lines<10000)
                	{
                	fprintf(fp, ", moving to queue 1\n");
                	lines++;
			}
			deleteFromQueue(q2, array, msgstruct.sPid);
                	addToQueue(q1, array, msgstruct.sPid);
                	pcbinfo[msgstruct.sPid].currentQueue = 1;
            	}
            	else if (pcbinfo[msgstruct.sPid].currentQueue == 3) 
		{
			if(lines<10000)
                	{
                	fprintf(fp, ", moving to queue 1\n");
                	lines++;
			}
			deleteFromQueue(q3, array, msgstruct.sPid);
                	addToQueue(q1, array, msgstruct.sPid);
                	pcbinfo[msgstruct.sPid].currentQueue = 1;
            	}
            	else 
		{
			if(lines<10000)
                {
                	fprintf(fp, "\n");
			lines++;
            	}
		}
        }
	
	/* The process used full time slice, move to appropriate queue */
	else if (msgstruct.timeFlg == 1) 
	{
		if(lines<10000)
                {
        	fprintf(fp, "OSS: Receiving that process PID %d ran for %u nanoseconds", msgstruct.sPid,msgstruct.burst);
         	lines++;
		}
         	if (pcbinfo[msgstruct.sPid].currentQueue == 1) 
		{
			if(lines<10000)
                	{
                	fprintf(fp, ", moving to queue 2\n");
                	lines++;
			}
			deleteFromQueue(q1, array, msgstruct.sPid);
                	addToQueue(q2, array, msgstruct.sPid);
                	pcbinfo[msgstruct.sPid].currentQueue = 2;
            	}
         
            	else if (pcbinfo[msgstruct.sPid].currentQueue == 2) 
		{
			if(lines<10000)
                	{
                	fprintf(fp, ", moving to queue 3\n");
                	lines++;
			}
			deleteFromQueue(q2, array, msgstruct.sPid);
                	addToQueue(q3, array, msgstruct.sPid);
                	pcbinfo[msgstruct.sPid].currentQueue = 3;
            	}
            	else 
		{
			if(lines<10000)
                {
                	fprintf(fp, "\n");
			lines++;
            	}
		}
        }

        /* Spawn a new member */
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
	
	/* Get the next queue and schedule the appropraite process */
	nextQ = checkQueue();
        if (nextQ == 0) 
	{
        	nextP = getChildQ(q0);
            	msgstruct.timeGivenNS = cost0;
            	deleteFromQueue(q0, array, nextP);
            	addToQueue(q0, array, nextP);
        }
        else if (nextQ == 1) 
	{
            	nextP = getChildQ(q1);
            	msgstruct.timeGivenNS = cost1;
            	deleteFromQueue(q1, array, nextP);
            	addToQueue(q1, array, nextP);
        }
        else if (nextQ == 2) 
	{
            	nextP = getChildQ(q2);
            	msgstruct.timeGivenNS = cost2;
            	deleteFromQueue(q2, array, nextP);
            	addToQueue(q2, array, nextP);
        }
        else if (nextQ == 3) 
	{
            	nextP = getChildQ(q3);
            	msgstruct.timeGivenNS = cost3;
            	deleteFromQueue(q3, array, nextP);
            	addToQueue(q3, array, nextP);
        }
   
        else 
	{
            	continue;
        }

	/* Schedule appropriate member */
	if (nextP != -1) 
	{
            	msgstruct.msgTyp = (long) nextP;
            	msgstruct.termFlg = 0;
            	msgstruct.timeFlg = 0;
            	msgstruct.blockedFlg = 0;
            	msgstruct.sPid = nextP;
            	if(lines<10000)
                {
		fprintf(fp, "OSS: Dispatching process PID %d from queue %d at time %u:%09u\n", nextP, pcbinfo[nextP].currentQueue, *clockSec, *clockNS);
            	lines++;
		}
		if ( msgsnd(qid, &msgstruct, sizeof(msgstruct), 0) == -1 ) 
		{
                	perror("OSS: error sending init msg");
                	detach();
                	exit(0);
            	}
        }
    	}

	printStats();
	detach();

	return 0;
}

/* Function to print final statistics */
void printStats()
{
	double x = (double)totalWaitNS/1000000000;
        double y = (double)totalWaitSec + x;
	
	double avgCPU = (double)bWholeSec + ((double)bWholeNS/1000000000);
	printf("Average user wait time = %.9f nanoseconds\n", (y / 100)/100);
	x = (double)bWholeNS/1000000000;
        y = (double)bWholeSec + x;
	printf("Average blocked time = %.9f nanoseconds\n", y / 100);
	printf("CPU idle time = %u seconds\n", stopSec);
}

/* Function to increment idle time */
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
                localns = createNS + (1000000000 - localsim_ns);
                localsec--;
        }
        if (localns >= 1000000000)
        {
                localsec++;
                temp = localns - 1000000000;
                localns = temp;
        }
        stopSec = stopSec + localsec;
        stopNS = stopNS + localns;
        if (stopNS >= 1000000000)
        {
                stopSec++;
                temp = stopNS - 1000000000;
                stopNS = temp;
        }
        return 1;
}

/* function to see if its time to spawn new member */
int checkTime() 
{
    	int rvalue = 0;
    	unsigned int localsec = *clockSec;
    	unsigned int localns = *clockNS;
    	if ( (localsec > createSec) ||  ( (localsec >= createSec) && (localns >= createNS) ) ) 
	{
        	rvalue = 1;
    	}
 
    	return rvalue;
}

/* Function to maintain blocked time and chack blocked queues */
void block(int blockpid) 
{
    	int temp;
    	unsigned int localsec, localns;
    
    	bWholeSec += pcbinfo[blockpid].bWholeSec;
	bWholeNS += pcbinfo[blockpid].bWholeNS;
    	if (bWholeNS >= 1000000000) 
	{
        	bWholeSec++;
        	temp = bWholeNS - 1000000000;
        	bWholeNS = temp;
    	}
   
    	pcbinfo[blockpid].blocked = 1;
    	if (pcbinfo[blockpid].currentQueue == 0)
        {deleteFromQueue(q0, array, blockpid);}
    	else if (pcbinfo[blockpid].currentQueue == 1)
        {deleteFromQueue(q1, array, blockpid);}
    	else if (pcbinfo[blockpid].currentQueue == 2)
        {deleteFromQueue(q2, array, blockpid);}
    	else if (pcbinfo[blockpid].currentQueue == 3)
        {deleteFromQueue(q3, array, blockpid);}

    	addToQueue(blocked, array, blockpid);

    	localsec = *clockSec;
    	localns = *clockNS;
    	temp = random();
    	if (temp < 100) temp = 10000;
    	else temp = temp * 100;
    	localns = localns + temp; 
    	if(lines<10000)
        {
	fprintf(fp, "OSS: Time used to move user to blocked queue: %u nanoseconds\n", temp);
	lines++;
	}	
	if (localns >= 1000000000) 
	{
        	localsec++;
        	temp = localns - 1000000000;
        	localns = temp;
    	}
   
    	*clockSec = localsec;
    	*clockNS = localns;
}

/* Get the member from the queue */
int getChildQ(int q[]) 
{
    	if (q[1] == 0) 
	{ 
        	return -1;
    	}
    	else return q[1];
}

/* Function that terminates appropriate member */
void term(int termPid) 
{
    	int status;
    	unsigned int temp;
    	waitpid(msgstruct.userPid, &status, 0);
    
    	totalSec += pcbinfo[termPid].totalWholeSec;
    	totalNS += pcbinfo[termPid].totalWholeNS;
    	if (totalNS >= 1000000000) 
	{
        	totalSec++;
        	temp = totalNS - 1000000000;
        	totalNS = temp;
    	}
 	totalWaitSec += (pcbinfo[termPid].totalWholeSec - pcbinfo[termPid].totalSec);
    	totalWaitNS +=(pcbinfo[termPid].totalWholeNS - pcbinfo[termPid].totalNS);
    	if (totalWaitNS >= 1000000000) 
	{
        	totalWaitSec++;
        	temp = totalWaitNS - 1000000000;
        	totalWaitNS = temp;
    	}
    
    	if (pcbinfo[termPid].currentQueue == 0) 
	{
        	deleteFromQueue(q0, array, termPid);
    	}
    	else if (pcbinfo[termPid].currentQueue == 1) 
	{
        	deleteFromQueue(q1, array, termPid);
    	}
    	else if (pcbinfo[termPid].currentQueue == 2) 
	{
        	deleteFromQueue(q2, array, termPid);
    	}
    	else if (pcbinfo[termPid].currentQueue == 3) 
	{
        	deleteFromQueue(q3, array, termPid);
    	}
    	bitMap[termPid] = 0;
    	numChild--;
    	pcbinfo[termPid].totalSec = 0;
    	pcbinfo[termPid].totalNS = 0;
    	pcbinfo[termPid].totalWholeSec = 0;
    	pcbinfo[termPid].totalWholeNS = 0;
	if(lines<10000)
        {
	fprintf(fp,"OSS: User %d has terminated\n",msgstruct.sPid);
	lines++;
	}
}

/* Increment clock after message is recieved */
void incClock(int childPid) 
{
    	unsigned int localsec = *clockSec;
    	unsigned int localns = *clockNS;
    	unsigned int temp;
    	temp = random();
    	if (temp < 100) temp = 100;
    	else temp = temp * 10;
    	localns = localns + temp; 
	if(lines<10000)
        {
	fprintf(fp, "OSS: Time spent this dispatch: %ld nanoseconds\n", temp);
    	lines++;
	}
	localns = localns + pcbinfo[childPid].burstNS;
    
    	if (localns >= 1000000000) 
	{
        	localsec++;
        	temp = localns - 1000000000;
        	localns = temp;
    	}
    
    	*clockSec = localsec;
    	*clockNS = localns;
}

/* Get the used from a blocked state when necessary */
void getChild(int wakepid) 
{
    	unsigned int localsec, localns, temp;

    	deleteFromQueue(blocked, array, wakepid);

    	pcbinfo[wakepid].blocked = 0;
    	pcbinfo[wakepid].bNS = 0;
    	pcbinfo[wakepid].bSec = 0; 

    	if (pcbinfo[wakepid].realTimeC == 1) 
	{
        	addToQueue(q0, array, wakepid);
        	pcbinfo[wakepid].currentQueue = 0;
    	}
    
    	else 
	{
        	addToQueue(q1, array, wakepid);
        	pcbinfo[wakepid].currentQueue = 1;
    	}

    	localsec = *clockSec;
    	localns = *clockNS;
    	temp = random();
    	if (temp < 100) temp = 10000;
    	else temp = temp * 100;
    	localns = localns + temp; 
}

/* Sets the time for when the next member will spawn */
void nextPbegin()
{
	unsigned int temp;
	unsigned int localsec = *clockSec;
	unsigned int localns = *clockNS;
	nextProcSec = rand_r(&begin) % (maxSec + 1);
    	nextProcNS = rand_r(&begin) % (maxNS + 1);
    	createSec = localsec + nextProcSec;
    	createNS = localns + nextProcNS;
	if (createNS >= 1000000000)
	{
		createSec++;
		temp = createNS - 1000000000;
		createNS = temp;
	}
}

/* Spawn new member */
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
        	addToQueue(q0, array, sPid);
        	pcbinfo[sPid].currentQueue = 0;
    	}

    	else 
	{
        	msgstruct.timeGivenNS = cost1;
        	PCB(sPid, 0);
        	addToQueue(q1, array, sPid);
        	pcbinfo[sPid].currentQueue = 1;
    	}
	if(lines<10000)
        {
    	fprintf(fp, "OSS: Generating process PID %d and putting it in queue %d at time %u:%09u\n", pcbinfo[sPid].localPid, pcbinfo[sPid].currentQueue, *clockSec, *clockNS);
	lines++;
	}
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

/* Checks the blocked queue to see if it is time to spawn new member */
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

/* Check if the blocked queue is empty */
int blockedCheck() 
{
    	if (blocked[1] != 0) 
	{
        	return 0;
    	}
    	return 1;
}

/* Create random integer */
int randomTime() 
{
    	int rvalue;
    	rvalue = rand_r(&begin) % (1000 + 1);
    	return rvalue;
}

/* initialize the process control block */
void PCB(int pidnum, int realTime) 
{
    	unsigned int localsec = *clockSec;
    	unsigned int localns = *clockNS;
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

/* Set up the shared memory */
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

/* Detach shared memory */
void detach() 
{
    	shmctl(smSec, IPC_RMID, NULL);
    	shmctl(smNS, IPC_RMID, NULL);
    	shmctl(shmid, IPC_RMID, NULL);
    	msgctl(qid, IPC_RMID, NULL);
}

/* Function to control two types of interupts */
void sigErrors(int signum)
{
        if (signum == SIGINT)
        {
		printStats();
		printf("\nInterupted by ctrl-c\n");
        }
        else
        {
		printStats();
                printf("\nInterupted by 3 second alarm\n");
        }
	
	detach();
        exit(0);
}
