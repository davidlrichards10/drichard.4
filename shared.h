/*
 * Name: David Richards
 * Assignment: CS4760 Project 4
 * Date: Tue March 24th
 * File: "shared.h"
 */

#ifndef SHARED_H
#define SHARED_H

struct times{
	int nanoseconds;
	int seconds;
};

/* Process control block information */
struct pcb{
    unsigned int startSec;
    unsigned int startNS;
    unsigned int totalSec;
    unsigned int totalNS;
    unsigned int totalWholeSec;
    unsigned int totalWholeNS;
    unsigned int burstNS;
    int blocked;
    int bTimes;
    unsigned int bSec;
    unsigned int bNS;
    unsigned int bWholeSec;
    unsigned int bWholeNS;
    int localPid;
    int realTimeC;
    int currentQueue;
};

/* Contains communication variables for oss->user */
struct messageQueue {
    long msgTyp;
    int sPid;
    pid_t userPid;
    unsigned int timeGivenNS;
    int termFlg;
    int timeFlg;
    int blockedFlg;
    unsigned int burst;
};

#endif
