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


int smSec, smNS; 
int smP; 
int qid; 
static unsigned int *clockSec; 
static unsigned int *clockNS; 
unsigned int startSec;
unsigned int startNS; 
int simPid; 
int seed;
unsigned int bSec, bNS;

struct pcb * pct; 

struct messageQueue mstruct;

int main(int argc, char** argv) 
{
	 unsigned int localsec, localns;
    mstruct.userPid = getpid();
    smP = atoi(argv[1]);
    simPid = atoi(argv[2]);
    int roll;
	getSM();
	pct[simPid].startSec = *clockSec; 
    pct[simPid].startNS = *clockNS;

	while(1) {
        if ( msgrcv(qid, &mstruct, sizeof(mstruct), simPid, 0) == -1 ) {
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
            
            localsec = *clockSec; 
            localns = *clockNS;
            bNS = rand_r(&seed) % 1000 + 1;
            bSec = rand_r(&seed) % 5 + 1;
            pct[simPid].bSec = localsec + bSec;
            pct[simPid].bNS = localns + bNS;
            incBlockedTime();
            pct[simPid].bTimes++;
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
    
    pct = (struct pcb *)shmat(smP, 0, 0);
    if ( pct == (struct pcb *)(-1) ) {
        perror("User: error in shmat pct");
        exit(1);
    }
    
    smSec = shmget(SHMKEY_sim_s, BUFF_SZ, 0444);
        if (smSec == -1) { 
            perror("User: error in shmget smSec");
            exit(1);
        }
    clockSec = (unsigned int*) shmat(smSec, 0, 0);
    
    smNS = shmget(SHMKEY_sim_ns, BUFF_SZ, 0444);
        if (smNS == -1) { 
            perror("User: error in shmget smNS");
            exit(1);
        }
    clockNS = (unsigned int*) shmat(smNS, 0, 0);
    
    
    if ( (qid = msgget(MSGQKEY_oss, 0777)) == -1 ) {
        perror("Error generating communication message queue");
        exit(0);
    }
}

void incBlockedTime() {
    unsigned int now_secs = *clockSec;
    unsigned int now_ns = *clockNS;
    unsigned int temp = 0;
    
    pct[simPid].bWholeSec += bSec; 
    pct[simPid].bWholeNS += bNS;
    if (pct[simPid].bWholeNS >= MAX) {
        pct[simPid].bWholeSec++;
        temp = pct[simPid].bWholeNS - MAX;
        pct[simPid].bWholeNS = temp;
    }
            
            
}

void incTime() {
    unsigned int temp = 0;
    pct[simPid].totalNS += mstruct.burst;
    if (pct[simPid].totalNS >= MAX) {
        pct[simPid].totalSec++;
        temp = pct[simPid].totalNS - MAX;
        pct[simPid].totalNS = temp;
    }
}

void compileStats() {
    
    unsigned int myEndTimeSecs = *clockSec; 
    unsigned int myEndTimeNS = *clockNS;
    unsigned int temp;

    if (myEndTimeSecs == pct[simPid].startSec) {
        pct[simPid].totalWholeNS = 
                (myEndTimeNS - pct[simPid].startNS);
        pct[simPid].totalWholeSec = 0;
    }
    else {
        pct[simPid].totalWholeSec = 
                myEndTimeSecs - pct[simPid].startSec;
                myEndTimeNS + (MAX - pct[simPid].startNS);
        pct[simPid].totalWholeSec--;
    }
    if (pct[simPid].totalWholeNS >= MAX) {
        pct[simPid].totalWholeSec++;
        temp = pct[simPid].totalWholeNS - MAX;
        pct[simPid].totalWholeNS = temp;
    }
    
}


void showTime() {
    incTime();
    mstruct.blockedFlg = 0;
    mstruct.termFlg = 0;
    mstruct.timeFlg = 1;
    mstruct.burst = mstruct.timeGivenNS;
    mstruct.sPid = simPid;
    mstruct.msgTyp = 99;
    if ( msgsnd(qid, &mstruct, sizeof(mstruct), 0) == -1 ) {
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
    mstruct.sPid = simPid;
    mstruct.msgTyp = 99;
    if ( msgsnd(qid, &mstruct, sizeof(mstruct), 0) == -1 ) {
        perror("User: error sending msg to oss");
        exit(0);
    }
}


void terminated() {
    mstruct.blockedFlg = 0;
    mstruct.termFlg = 1;
    mstruct.timeFlg = 0;
    mstruct.sPid = simPid;
    mstruct.msgTyp = 99;
    
    if ( msgsnd(qid, &mstruct, sizeof(mstruct), 0) == -1 ) {
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
    if ( msgsnd(qid, &mstruct, sizeof(mstruct), 0) == -1 ) {
        perror("User: error sending msg to oss");
        exit(0);
    }
}

int randomTime() {
    int return_val;
    return_val = rand_r(&seed) % 1000 + 1;
    return return_val;
}
