/*
 * Name: David Richards
 * Assignment: CS4760 Project 4
 * Date: Tue March 24th
 * File: "shared.h"
 */

#ifndef SHARED_H
#define SHARED_H

#define SHMKEY_sim_s 4020012
#define SHMKEY_sim_ns 4020013
#define SHMKEY_pct 4020014
#define MSGQKEY_oss 4020069
#define BUFF_SZ sizeof (unsigned int)
#define BILLION 1000000000

struct pcb{
    unsigned int startTime_secs;
    unsigned int startTime_ns;
    unsigned int totalCPUtime_secs;
    unsigned int totalCPUtime_ns;
    unsigned int totalLIFEtime_secs;
    unsigned int totalLIFEtime_ns;
    unsigned int timeUsedLastBurst_ns;
    int blocked;
    int timesBlocked;
    unsigned int blockedUntilSecs;
    unsigned int blockedUntilNS;
    unsigned int totalBlockedTime_secs;
    unsigned int totalBlockedTime_ns;
    int localPID;
    int isRealTimeClass;
    int currentQueue;
};


struct commsbuf {
    long msgtyp;
    int sPid;
    pid_t user_sys_pid;
    unsigned int ossTimeSliceGivenNS;
    int userTerminatingFlag;
    int userUsedFullTimeSliceFlag;
    int userBlockedFlag;
    unsigned int userTimeUsedLastBurst;
};

#endif
