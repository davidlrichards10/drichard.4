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

void getSM();
int randomTime(); 
void showTime();
void terminated();
void blocked();
void started();
unsigned int randomSTime();
void incTime ();
void incBlockedTime ();
void stats();

int shmid_sim_secs, shmid_sim_ns; 
int shmid_pct; 
int oss_qid; 
static unsigned int *simClock_secs; 
static unsigned int *simClock_ns; 
unsigned int myStartTimeSecs;
unsigned int myStartTimeNS; 
int my_sim_pid; 
unsigned int seed;
unsigned int b_sec, b_ns;

struct pcb * pcbinfo; 

struct messageQueue mstruct;

int main(int argc, char** argv) 
{
	unsigned int localsec, localns;
    	mstruct.userPid = getpid();
    	shmid_pct = atoi(argv[1]);
    	my_sim_pid = atoi(argv[2]);
    	int rand;
	getSM();
	pcbinfo[my_sim_pid].startSec = *simClock_secs; 
    	pcbinfo[my_sim_pid].startNS = *simClock_ns;

	while(1) 
	{
        	if ( msgrcv(oss_qid, &mstruct, sizeof(mstruct), my_sim_pid, 0) == -1 ) 
		{
            	exit(0);
        	}
        	mstruct.userPid = getpid(); 
        	rand = randomTime();
        	/* User will get terminated in this range */
		if (rand < 15) 
		{
            		mstruct.burst = randomSTime();
            		incTime();
			stats();
            		terminated();
            		return 1;
        	}
        
		/* User will get blocked in this range */
        	else if (rand < 17) 
		{
            		localsec = *simClock_secs; 
           	 	localns = *simClock_ns;
            		b_ns = rand_r(&seed) % 1000 + 1;
            		b_sec = rand_r(&seed) % 3 + 1;
            		pcbinfo[my_sim_pid].bSec = localsec + b_sec;
            		pcbinfo[my_sim_pid].bNS = localns + b_ns;
            		incBlockedTime();
            		pcbinfo[my_sim_pid].bTimes++;
            		blocked();
        	}
        
		/* User will get preempted in this range */
        	else if (rand < 260) 
		{
            		started();
        	}
		/* Get the accurate time */
        	else 
		{
            		showTime();
        	}

    	}
	return EXIT_SUCCESS;
}

void stats() 
{    
    	unsigned int myEndTimeSecs = *simClock_secs; 
    	unsigned int myEndTimeNS = *simClock_ns;
    	unsigned int temp;

    	if (myEndTimeSecs == pcbinfo[my_sim_pid].startSec) 
	{
        	pcbinfo[my_sim_pid].totalWholeNS = (myEndTimeNS - pcbinfo[my_sim_pid].startNS);
        	pcbinfo[my_sim_pid].totalWholeSec = 0;
    	}
    	else 
	{
        	pcbinfo[my_sim_pid].totalWholeSec = myEndTimeSecs - pcbinfo[my_sim_pid].startSec;
        	pcbinfo[my_sim_pid].totalWholeNS = myEndTimeNS + ( 1000000000 - pcbinfo[my_sim_pid].startNS);
        	pcbinfo[my_sim_pid].totalWholeSec--;
    	}
    	if (pcbinfo[my_sim_pid].totalWholeNS >=  1000000000) 
	{
        	pcbinfo[my_sim_pid].totalWholeSec++;
        	temp = pcbinfo[my_sim_pid].totalWholeNS -  1000000000;
        	pcbinfo[my_sim_pid].totalWholeNS = temp;
    	}
    
}

/* Get shared memory information */
void getSM() 
{   
    	pcbinfo = (struct pcb *)shmat(shmid_pct, 0, 0);
    	if ( pcbinfo == (struct pcb *)(-1) ) 
	{
        	perror("user: shmat error");
        	exit(1);
    	}
    
    	shmid_sim_secs = shmget(SHMKEY_sim_s, BUFF_SZ, 0444);
        if (shmid_sim_secs == -1) 
	{ 
            	perror("user: shmget error");
            	exit(1);
        }
    	simClock_secs = (unsigned int*) shmat(shmid_sim_secs, 0, 0);
    
    	shmid_sim_ns = shmget(SHMKEY_sim_ns, BUFF_SZ, 0444);
        if (shmid_sim_ns == -1) 
	{ 
            	perror("user: shmget error");
            	exit(1);
        }
    	simClock_ns = (unsigned int*) shmat(shmid_sim_ns, 0, 0);
    
    	if ( (oss_qid = msgget(MSGQKEY_oss, 0777)) == -1 ) 
	{
        	perror("user: msgget error");
        	exit(0);
    	}
}

/* Increment the time that a member is blocked */
void incBlockedTime() 
{
    	unsigned int now_secs = *simClock_secs;
    	unsigned int now_ns = *simClock_ns;
    	unsigned int temp = 0;
    
    	pcbinfo[my_sim_pid].bWholeSec += b_sec; 
   	pcbinfo[my_sim_pid].bWholeNS += b_ns;
    	if (pcbinfo[my_sim_pid].bWholeNS >= 1000000000) 
	{
        	pcbinfo[my_sim_pid].bWholeSec++;
        	temp = pcbinfo[my_sim_pid].bWholeNS - 1000000000;
        	pcbinfo[my_sim_pid].bWholeNS = temp;
    	}       
}

/* Increments total CPU time */
void incTime() 
{
    	unsigned int temp = 0;
    	pcbinfo[my_sim_pid].totalNS += mstruct.burst;
    	if (pcbinfo[my_sim_pid].totalNS >= 1000000000) 
	{
        	pcbinfo[my_sim_pid].totalSec++;
        	temp = pcbinfo[my_sim_pid].totalNS - 1000000000;
        	pcbinfo[my_sim_pid].totalNS = temp;
    	}
}

/* Sends information through the message queue that is needed for OSS */
void showTime() 
{
    	incTime();
    	mstruct.blockedFlg = 0;
    	mstruct.termFlg = 0;
    	mstruct.timeFlg = 1;
    	mstruct.burst = mstruct.timeGivenNS;
    	mstruct.sPid = my_sim_pid;
    	mstruct.msgTyp = 99;
    	if ( msgsnd(oss_qid, &mstruct, sizeof(mstruct), 0) == -1 ) 
	{
        	perror("User: error sending msg to oss");
        	exit(0);
    	}
}

/* Report if a member was preempted */
void started() 
{
    	mstruct.blockedFlg = 0;
    	mstruct.termFlg = 0;
    	mstruct.timeFlg = 0;
    	mstruct.burst = randomSTime();
    	incTime();
    	mstruct.sPid = my_sim_pid;
    	mstruct.msgTyp = 99;
    	if ( msgsnd(oss_qid, &mstruct, sizeof(mstruct), 0) == -1 ) 
	{
        	perror("User: error sending msg to oss");
        	exit(0);
    	}
}

/* Report if the member was terminated */
void terminated() 
{
    	mstruct.blockedFlg = 0;
    	mstruct.termFlg = 1;
    	mstruct.timeFlg = 0;
    	mstruct.sPid = my_sim_pid;
    	mstruct.msgTyp = 99;
    
    	if ( msgsnd(oss_qid, &mstruct, sizeof(mstruct), 0) == -1 ) 
	{
        	perror("User: error sending msg to oss");
        	exit(0);
    	}
}

/* Gets the random time slice in nanoseconds */
unsigned int randomSTime() 
{
    	unsigned int return_val;
    	return_val = (rand_r(&seed) % (mstruct.timeGivenNS) + 1);
    	return return_val;
}

/* Report if the member was blocked */
void blocked() 
{
    	mstruct.msgTyp = 99;
    	mstruct.timeFlg = 0;
    	mstruct.burst = rand_r(&seed) % 99 + 1;
    	incTime();
    	mstruct.blockedFlg = 1;
    	if ( msgsnd(oss_qid, &mstruct, sizeof(mstruct), 0) == -1 ) 
	{
        	perror("user: error sending msg");
        	exit(0);
    	}
}

/* Create random integer */
int randomTime() 
{
    	int return_val;
    	return_val = rand_r(&seed) % 1000 + 1;
    	return return_val;
}
