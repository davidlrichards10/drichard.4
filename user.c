#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <signal.h>
#include "shared.h"

void getSM();
int randomTime(); 
void showTime();
void terminated();
void blocked();
void started();
unsigned int randomSTime();
void compileStats();
void incTime ();
void incBlockedTime ();


int shmid_sim_secs, shmid_sim_ns; 
int shmid_pct; 
int oss_qid; 
static unsigned int *simClock_secs; 
static unsigned int *simClock_ns; 
unsigned int myStartTimeSecs;
unsigned int myStartTimeNS; 
int my_sim_pid; 
int seed;
unsigned int b_sec, b_ns;

struct pcb * pct; 

struct messageQueue mstruct;

int main(int argc, char** argv) 
{
	 unsigned int localsec, localns;
    mstruct.userPid = getpid();
    shmid_pct = atoi(argv[1]);
    my_sim_pid = atoi(argv[2]);
    int roll;
	getSM();
	pct[my_sim_pid].startSec = *simClock_secs; 
    pct[my_sim_pid].startNS = *simClock_ns;

	while(1) {
        if ( msgrcv(oss_qid, &mstruct, sizeof(mstruct), my_sim_pid, 0) == -1 ) {
            exit(0);
        }
        mstruct.userPid = getpid(); 
        
        
        roll = randomTime();
        if (roll < 15) {
            
            mstruct.burst = randomSTime();
            incTime();
            compileStats();
            terminated();
            return 1;
        }
        
        else if (roll < 17) {
            
            localsec = *simClock_secs; 
            localns = *simClock_ns;
            b_ns = rand_r(&seed) % 1000 + 1;
            b_sec = rand_r(&seed) % 5 + 1;
            pct[my_sim_pid].bSec = localsec + b_sec;
            pct[my_sim_pid].bNS = localns + b_ns;
            incBlockedTime();
            pct[my_sim_pid].bTimes++;
            blocked();
        }
        
        else if (roll < 260) {
            started();
        }
        else {
            showTime();
        }

    }

	return EXIT_SUCCESS;
}

void getSM() 
{
    
    pct = (struct pcb *)shmat(shmid_pct, 0, 0);
    if ( pct == (struct pcb *)(-1) ) {
        perror("User: error in shmat pct");
        exit(1);
    }
    
    shmid_sim_secs = shmget(SHMKEY_sim_s, BUFF_SZ, 0444);
        if (shmid_sim_secs == -1) { 
            perror("User: error in shmget shmid_sim_secs");
            exit(1);
        }
    simClock_secs = (unsigned int*) shmat(shmid_sim_secs, 0, 0);
    
    shmid_sim_ns = shmget(SHMKEY_sim_ns, BUFF_SZ, 0444);
        if (shmid_sim_ns == -1) { 
            perror("User: error in shmget shmid_sim_ns");
            exit(1);
        }
    simClock_ns = (unsigned int*) shmat(shmid_sim_ns, 0, 0);
    
    
    if ( (oss_qid = msgget(MSGQKEY_oss, 0777)) == -1 ) {
        perror("Error generating communication message queue");
        exit(0);
    }
}

void incBlockedTime() {
    unsigned int now_secs = *simClock_secs;
    unsigned int now_ns = *simClock_ns;
    unsigned int temp = 0;
    
    pct[my_sim_pid].bWholeSec += b_sec; 
    pct[my_sim_pid].bWholeNS += b_ns;
    if (pct[my_sim_pid].bWholeNS >= MAX) {
        pct[my_sim_pid].bWholeSec++;
        temp = pct[my_sim_pid].bWholeNS - MAX;
        pct[my_sim_pid].bWholeNS = temp;
    }
            
            
}

void incTime() {
    unsigned int temp = 0;
    pct[my_sim_pid].totalNS += mstruct.burst;
    if (pct[my_sim_pid].totalNS >= MAX) {
        pct[my_sim_pid].totalSec++;
        temp = pct[my_sim_pid].totalNS - MAX;
        pct[my_sim_pid].totalNS = temp;
    }
}

void compileStats() {
    
    unsigned int myEndTimeSecs = *simClock_secs; 
    unsigned int myEndTimeNS = *simClock_ns;
    unsigned int temp;

    if (myEndTimeSecs == pct[my_sim_pid].startSec) {
        pct[my_sim_pid].totalWholeNS = 
                (myEndTimeNS - pct[my_sim_pid].startNS);
        pct[my_sim_pid].totalWholeSec = 0;
    }
    else {
        pct[my_sim_pid].totalWholeSec = 
                myEndTimeSecs - pct[my_sim_pid].startSec;
        pct[my_sim_pid].totalWholeNS = 
                myEndTimeNS + (MAX - pct[my_sim_pid].startNS);
        pct[my_sim_pid].totalWholeSec--;
    }
    if (pct[my_sim_pid].totalWholeNS >= MAX) {
        pct[my_sim_pid].totalWholeSec++;
        temp = pct[my_sim_pid].totalWholeNS - MAX;
        pct[my_sim_pid].totalWholeNS = temp;
    }
    
}


void showTime() {
    incTime();
    mstruct.blockedFlg = 0;
    mstruct.termFlg = 0;
    mstruct.timeFlg = 1;
    mstruct.burst = mstruct.timeGivenNS;
    mstruct.sPid = my_sim_pid;
    mstruct.msgTyp = 99;
    if ( msgsnd(oss_qid, &mstruct, sizeof(mstruct), 0) == -1 ) {
        perror("User: error sending msg to oss");
        exit(0);
    }
}

void started() {
    mstruct.blockedFlg = 0;
    mstruct.termFlg = 0;
    mstruct.timeFlg = 0;
    mstruct.burst = randomSTime();
    incTime();
    mstruct.sPid = my_sim_pid;
    mstruct.msgTyp = 99;
    if ( msgsnd(oss_qid, &mstruct, sizeof(mstruct), 0) == -1 ) {
        perror("User: error sending msg to oss");
        exit(0);
    }
}


void terminated() {
    mstruct.blockedFlg = 0;
    mstruct.termFlg = 1;
    mstruct.timeFlg = 0;
    mstruct.sPid = my_sim_pid;
    mstruct.msgTyp = 99;
    
    if ( msgsnd(oss_qid, &mstruct, sizeof(mstruct), 0) == -1 ) {
        perror("User: error sending msg to oss");
        exit(0);
    }
}

unsigned int randomSTime() {
    unsigned int return_val;
    return_val = (rand_r(&seed) % (mstruct.timeGivenNS) + 1);
    return return_val;
}

void blocked() {
    mstruct.msgTyp = 99;
    mstruct.timeFlg = 0;
    mstruct.burst = rand_r(&seed) % 99 + 1;
    incTime();
    mstruct.blockedFlg = 1;
    if ( msgsnd(oss_qid, &mstruct, sizeof(mstruct), 0) == -1 ) {
        perror("User: error sending msg to oss");
        exit(0);
    }
}

int randomTime() {
    int return_val;
    return_val = rand_r(&seed) % 1000 + 1;
    return return_val;
}
