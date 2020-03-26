/*
 * Name: David Richards
 * Assignment: CS4760 Project 4
 * Date: tue March 24th
 * File: "user.c"
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

void getSM();
int randomTime(); 
void showTime();
void terminated();
void blocked();
void started();
unsigned int randomSTime();
void compileStats();
void incTime();
void incBlockedTime();

int smSec, smNS; 
int sm; 
int qid; 
static unsigned int *clockSec; 
static unsigned int *clockNS; 
unsigned int startSec;
unsigned int startNS; 
int simPid; 
int begin;
unsigned int bSec, bNS;

struct pcb * pct; 

struct messageQueue mstruct;

int main(int argc, char** argv) 
{
	 unsigned int localSec, localNS;
    mstruct.userPid = getpid();
    sm = atoi(argv[1]);
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
            
            localSec = *clockSec; 
            localNS = *clockNS;
            bNS = rand_r(&begin) % 1000 + 1;
            bSec = rand_r(&begin) % 5 + 1;
            pct[simPid].bSec = localSec + bSec;
            pct[simPid].bNS = localNS + bNS;
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
    
    pct = (struct pcb *)shmat(sm, 0, 0);
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
    unsigned int seconds = *clockSec;
    unsigned int nanoseconds = *clockNS;
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
    
    unsigned int endSec = *clockSec; 
    unsigned int endNS = *clockNS;
    unsigned int temp;

    if (endSec == pct[simPid].startSec) {
        pct[simPid].totalWholeNS = 
                (endNS - pct[simPid].startNS);
        pct[simPid].totalWholeSec = 0;
    }
    else {
        pct[simPid].totalWholeSec = 
                endSec - pct[simPid].startSec;
        pct[simPid].totalWholeNS = 
                endNS + (MAX - pct[simPid].startNS);
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
    unsigned int value;
    value = (rand_r(&begin) % (mstruct.timeGivenNS) + 1);
    return value;
}

void blocked() {
    mstruct.msgTyp = 99;
    mstruct.timeFlg = 0;
    mstruct.burst = rand_r(&begin) % 99 + 1;
    incTime();
    mstruct.blockedFlg = 1;
    if ( msgsnd(qid, &mstruct, sizeof(mstruct), 0) == -1 ) {
        perror("User: error sending msg to oss");
        exit(0);
    }
}

int randomTime() {
    int value;
    value = rand_r(&begin) % 1000 + 1;
    return value;
}
