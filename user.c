/* Name: David Richards
 * Date: March 29th
 * Assignment: hw 4
 * File: user.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <signal.h>
#include "shared.h"
#include "queue.h"

void incTime ();

int sim_secs, sim_ns; 
int shmid_pct; 
int qid; 
static unsigned int *simClock_secs; 
static unsigned int *simClock_ns; 
int pid; 
unsigned int seed;
unsigned int b_sec, b_ns;
int termProb = 15;
int blockedProb = 17;
int preemptProb = 260;

struct pcb * pcbinfo; 

struct messageQueue mstruct;

int main(int argc, char** argv) 
{
	unsigned int localsec, localns;
    	mstruct.userPid = getpid();
    	shmid_pct = atoi(argv[1]);
    	pid = atoi(argv[2]);
    	int rand;
	const int chanceToTakeFullTime = 75;
	 pcbinfo = (struct pcb *)shmat(shmid_pct, 0, 0);
        if ( pcbinfo == (struct pcb *)(-1) )
        {
                perror("user: shmat error");
                exit(1);
        }

        sim_secs = shmget(4020012, sizeof (unsigned int), 0444);
        if (sim_secs == -1)
        {
                perror("user: shmget error");
                exit(1);
        }
        simClock_secs = (unsigned int*) shmat(sim_secs, 0, 0);

        sim_ns = shmget(4020013, sizeof (unsigned int), 0444);
        if (sim_ns == -1)
        {
                perror("user: shmget error");
                exit(1);
        }
        simClock_ns = (unsigned int*) shmat(sim_ns, 0, 0);

        if ( (qid = msgget(4020069, 0777)) == -1 )
        {
                perror("user: msgget error");
                exit(0);
        }
	
	pcbinfo[pid].startSec = *simClock_secs; 
    	pcbinfo[pid].startNS = *simClock_ns;

	while(1) 
	{
        	if ( msgrcv(qid, &mstruct, sizeof(mstruct), pid, 0) == -1 ) 
		{
            	exit(0);
        	}
        	mstruct.userPid = getpid(); 
        	int rand = rand_r(&seed) % 1000 + 1;;
        	/* User will get terminated in this range */
		if (rand < termProb) 
		{
			unsigned int randomSlice = (rand_r(&seed) % (mstruct.timeGivenNS) + 1);
            		mstruct.burst = randomSlice;
            		incTime();
			unsigned int myEndTimeSecs = *simClock_secs; 
    			unsigned int myEndTimeNS = *simClock_ns;
    			unsigned int temp;

    			if (myEndTimeSecs == pcbinfo[pid].startSec) 
			{
        			pcbinfo[pid].totalWholeNS = (myEndTimeNS - pcbinfo[pid].startNS);
        			pcbinfo[pid].totalWholeSec = 0;
    			}
    			else 
			{
        			pcbinfo[pid].totalWholeSec = myEndTimeSecs - pcbinfo[pid].startSec;
        			pcbinfo[pid].totalWholeNS = myEndTimeNS + ( 1000000000 - pcbinfo[pid].startNS);
        			pcbinfo[pid].totalWholeSec--;
    			}
    			if (pcbinfo[pid].totalWholeNS >=  1000000000) 
			{
        			pcbinfo[pid].totalWholeSec++;
        			temp = pcbinfo[pid].totalWholeNS -  1000000000;
        			pcbinfo[pid].totalWholeNS = temp;
    			}
            		mstruct.blockedFlg = 0;
    			mstruct.termFlg = 1;
    			mstruct.timeFlg = 0;
    			mstruct.sPid = pid;
    			mstruct.msgTyp = 99;
    
    			if ( msgsnd(qid, &mstruct, sizeof(mstruct), 0) == -1 ) 
			{
        			perror("User: error sending msg to oss");
        			exit(0);
    			}
            		return 1;
        	}
        
		/* User will get blocked in this range */
        	else if (rand < blockedProb) 
		{
            		localsec = *simClock_secs; 
           	 	localns = *simClock_ns;
            		b_ns = rand_r(&seed) % 1000 + 1;
            		b_sec = rand_r(&seed) % 3 + 1;
            		pcbinfo[pid].bSec = localsec + b_sec;
            		pcbinfo[pid].bNS = localns + b_ns;
            		unsigned int now_secs = *simClock_secs;
    			unsigned int now_ns = *simClock_ns;
    			unsigned int temp = 0;
    
    			pcbinfo[pid].bWholeSec += b_sec; 
   			pcbinfo[pid].bWholeNS += b_ns;
    			if (pcbinfo[pid].bWholeNS >= 1000000000) 
			{
        			pcbinfo[pid].bWholeSec++;
        			temp = pcbinfo[pid].bWholeNS - 1000000000;
        			pcbinfo[pid].bWholeNS = temp;
    			}       
            		pcbinfo[pid].bTimes++;
            		mstruct.msgTyp = 99;
    			mstruct.timeFlg = 0;
    			mstruct.burst = rand_r(&seed) % 99 + 1;
    			incTime();
    			mstruct.blockedFlg = 1;
    			if ( msgsnd(qid, &mstruct, sizeof(mstruct), 0) == -1 ) 
			{
        			perror("user: error sending msg");
        			exit(0);
    			}
        	}
        
		/* User will get preempted in this range */
        	else if (rand < preemptProb) 
		{
            		mstruct.blockedFlg = 0;
    			mstruct.termFlg = 0;
    			mstruct.timeFlg = 0;
			unsigned int randomSlice = (rand_r(&seed) % (mstruct.timeGivenNS) + 1);
    			mstruct.burst = randomSlice;
    			incTime();
    			mstruct.sPid = pid;
    			mstruct.msgTyp = 99;
    			if ( msgsnd(qid, &mstruct, sizeof(mstruct), 0) == -1 ) 
			{
        			perror("User: error sending msg to oss");
        			exit(0);
    			}
        	}
		/* Get the accurate time */
        	else 
		{
            		incTime();
    			mstruct.blockedFlg = 0;
    			mstruct.termFlg = 0;
    			mstruct.timeFlg = 1;
    			mstruct.burst = mstruct.timeGivenNS;
    			mstruct.sPid = pid;
    			mstruct.msgTyp = 99;
    			if ( msgsnd(qid, &mstruct, sizeof(mstruct), 0) == -1 ) 
			{
        			perror("User: error sending msg to oss");
        			exit(0);
    			}
        	}

    	}
	return EXIT_SUCCESS;
}

/* Increments total CPU time */
void incTime() 
{
    	unsigned int temp = 0;
    	pcbinfo[pid].totalNS += mstruct.burst;
    	if (pcbinfo[pid].totalNS >= 1000000000) 
	{
        	pcbinfo[pid].totalSec++;
        	temp = pcbinfo[pid].totalNS - 1000000000;
        	pcbinfo[pid].totalNS = temp;
    	}
}
