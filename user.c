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
int roll1000(); 
void reportFinishedTimeSlice();
void reportTermination();
void reportBlocked();
void reportPreempted();
unsigned int randomPortionOfTimeSlice();
void compileStats();
void incrementCPUtime();
void incrementBlockedTime();

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

struct commsbuf myinfo;

int main(int argc, char** argv) 
{
	 unsigned int localsec, localns;
    myinfo.user_sys_pid = getpid();
    shmid_pct = atoi(argv[1]);
    my_sim_pid = atoi(argv[2]);
    int roll;

	printf("User %d launched.\n", my_sim_pid);
	getSM();
	pct[my_sim_pid].startTime_secs = *simClock_secs; 
    pct[my_sim_pid].startTime_ns = *simClock_ns;

	while(1) {
        if ( msgrcv(oss_qid, &myinfo, sizeof(myinfo), my_sim_pid, 0) == -1 ) {
            
            printf("User %02d Terminated: OSS removed message queue.\n", my_sim_pid);
            exit(0);
        }
        myinfo.user_sys_pid = getpid(); 
        
        
        roll = roll1000();
        if (roll < 15) {
            
            myinfo.userTimeUsedLastBurst = randomPortionOfTimeSlice();
            incrementCPUtime();
            compileStats();
            reportTermination();
            return 1;
        }
        
        else if (roll < 17) {
            
            localsec = *simClock_secs; 
            localns = *simClock_ns;
            b_ns = rand_r(&seed) % 1000 + 1;
            b_sec = rand_r(&seed) % 5 + 1;
            pct[my_sim_pid].blockedUntilSecs = localsec + b_sec;
            pct[my_sim_pid].blockedUntilNS = localns + b_ns;
            incrementBlockedTime();
            pct[my_sim_pid].timesBlocked++;
            reportBlocked();
        }
        
        else if (roll < 260) {
            reportPreempted();
        }
        else {
            reportFinishedTimeSlice();
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

void incrementBlockedTime() {
    unsigned int now_secs = *simClock_secs;
    unsigned int now_ns = *simClock_ns;
    unsigned int temp = 0;
    
    pct[my_sim_pid].totalBlockedTime_secs += b_sec; 
    pct[my_sim_pid].totalBlockedTime_ns += b_ns;
    if (pct[my_sim_pid].totalBlockedTime_ns >= BILLION) {
        pct[my_sim_pid].totalBlockedTime_secs++;
        temp = pct[my_sim_pid].totalBlockedTime_ns - BILLION;
        pct[my_sim_pid].totalBlockedTime_ns = temp;
    }
            
            
}

void incrementCPUtime() {
    unsigned int temp = 0;
    pct[my_sim_pid].totalCPUtime_ns += myinfo.userTimeUsedLastBurst;
    if (pct[my_sim_pid].totalCPUtime_ns >= BILLION) {
        pct[my_sim_pid].totalCPUtime_secs++;
        temp = pct[my_sim_pid].totalCPUtime_ns - BILLION;
        pct[my_sim_pid].totalCPUtime_ns = temp;
    }
}

void compileStats() {
    
    unsigned int myEndTimeSecs = *simClock_secs; 
    unsigned int myEndTimeNS = *simClock_ns;
    unsigned int temp;

    if (myEndTimeSecs == pct[my_sim_pid].startTime_secs) {
        pct[my_sim_pid].totalLIFEtime_ns = 
                (myEndTimeNS - pct[my_sim_pid].startTime_ns);
        pct[my_sim_pid].totalLIFEtime_secs = 0;
    }
    else {
        pct[my_sim_pid].totalLIFEtime_secs = 
                myEndTimeSecs - pct[my_sim_pid].startTime_secs;
        pct[my_sim_pid].totalLIFEtime_ns = 
                myEndTimeNS + (BILLION - pct[my_sim_pid].startTime_ns);
        pct[my_sim_pid].totalLIFEtime_secs--;
    }
    if (pct[my_sim_pid].totalLIFEtime_ns >= BILLION) {
        pct[my_sim_pid].totalLIFEtime_secs++;
        temp = pct[my_sim_pid].totalLIFEtime_ns - BILLION;
        pct[my_sim_pid].totalLIFEtime_ns = temp;
    }
    
}


void reportFinishedTimeSlice() {
    incrementCPUtime();
    myinfo.userBlockedFlag = 0;
    myinfo.userTerminatingFlag = 0;
    myinfo.userUsedFullTimeSliceFlag = 1;
    myinfo.userTimeUsedLastBurst = myinfo.ossTimeSliceGivenNS;
    myinfo.user_sim_pid = my_sim_pid;
    myinfo.msgtyp = 99;
    if ( msgsnd(oss_qid, &myinfo, sizeof(myinfo), 0) == -1 ) {
        perror("User: error sending msg to oss");
        exit(0);
    }
}

void reportPreempted() {
    myinfo.userBlockedFlag = 0;
    myinfo.userTerminatingFlag = 0;
    myinfo.userUsedFullTimeSliceFlag = 0;
    myinfo.userTimeUsedLastBurst = randomPortionOfTimeSlice();
    incrementCPUtime();
    myinfo.user_sim_pid = my_sim_pid;
    myinfo.msgtyp = 99;
    if ( msgsnd(oss_qid, &myinfo, sizeof(myinfo), 0) == -1 ) {
        perror("User: error sending msg to oss");
        exit(0);
    }
}


void reportTermination() {
    myinfo.userBlockedFlag = 0;
    myinfo.userTerminatingFlag = 1;
    myinfo.userUsedFullTimeSliceFlag = 0;
    myinfo.user_sim_pid = my_sim_pid;
    myinfo.msgtyp = 99;
    
    if ( msgsnd(oss_qid, &myinfo, sizeof(myinfo), 0) == -1 ) {
        perror("User: error sending msg to oss");
        exit(0);
    }
}

unsigned int randomPortionOfTimeSlice() {
    unsigned int return_val;
    return_val = (rand_r(&seed) % (myinfo.ossTimeSliceGivenNS) + 1);
    return return_val;
}

void reportBlocked() {
    myinfo.msgtyp = 99;
    myinfo.userUsedFullTimeSliceFlag = 0;
    myinfo.userTimeUsedLastBurst = rand_r(&seed) % 99 + 1;
    incrementCPUtime();
    myinfo.userBlockedFlag = 1;
    if ( msgsnd(oss_qid, &myinfo, sizeof(myinfo), 0) == -1 ) {
        perror("User: error sending msg to oss");
        exit(0);
    }
}

int roll1000() {
    int return_val;
    return_val = rand_r(&seed) % 1000 + 1;
    return return_val;
}
