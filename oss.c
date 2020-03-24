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

void setUp(); //allocate shared memory
void detach(); //clear shared memory and message queues
void sigErrors(int signum);

int shmid_sim_secs, shmid_sim_ns;
int shmid_pct;
int oss_qid;
unsigned int maxTimeBetweenProcsNS, maxTimeBetweenProcsSecs;
unsigned int timeToNextProcNS, timeToNextProcSecs, seed;
unsigned int spawnNextProcNS, spawnNextProcSecs;
static unsigned int *simClock_secs; //pointer to shm sim clock (seconds)
static unsigned int *simClock_ns; //pointer to shm sim clock (nanoseconds)

char outputFileName[] = "logs";
FILE* fp;

struct commsbuf infostruct;

struct pcb * pct;


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

	int finishThem = 0;
    	int i; 
   	double realTimeElapsed = 0;
    	double runtimeAllowed = 3; 
    	maxTimeBetweenProcsNS = 999999998;
    	maxTimeBetweenProcsSecs = 0;
    	seed = (unsigned int) getpid(); 
    	pid_t childpid; 
    	char str_pct_id[20]; 
    	char str_user_sim_pid[4]; 
   	int user_sim_pid;
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
	detach();

	return 0;
}

void setUp() 
{
    printf("OSS: Allocating shared memory\n");
    
    shmid_pct = shmget(SHMKEY_pct, 19*sizeof(struct pcb), 0777 | IPC_CREAT);
     if (shmid_pct == -1) { 
            perror("OSS: error in shmget shmid_pct");
            exit(1);
        }
    pct = (struct pcb *)shmat(shmid_pct, 0, 0);
    if ( pct == (struct pcb *)(-1) ) {
        perror("OSS: error in shmat pct");
        exit(1);
    }   

    shmid_sim_secs = shmget(SHMKEY_sim_s, BUFF_SZ, 0777 | IPC_CREAT);
        if (shmid_sim_secs == -1) { 
            perror("OSS: error in shmget shmid_sim_secs");
            exit(1);
        }
    simClock_secs = (unsigned int*) shmat(shmid_sim_secs, 0, 0);

    shmid_sim_ns = shmget(SHMKEY_sim_ns, BUFF_SZ, 0777 | IPC_CREAT);
        if (shmid_sim_ns == -1) { 
            perror("OSS: error in shmget shmid_sim_ns");
            exit(1);
        }
    simClock_ns = (unsigned int*) shmat(shmid_sim_ns, 0, 0);
    

    if ( (oss_qid = msgget(MSGQKEY_oss, 0777 | IPC_CREAT)) == -1 ) {
        perror("Error generating communication message queue");
        exit(0);
    }
}

void detach() 
{
    printf("OSS: Clearing IPC resources...\n");

    if ( shmctl(shmid_sim_secs, IPC_RMID, NULL) == -1) {
        perror("error removing shared memory");
    }
    if ( shmctl(shmid_sim_ns, IPC_RMID, NULL) == -1) {
        perror("error removing shared memory");
    }

    if ( shmctl(shmid_pct, IPC_RMID, NULL) == -1) {
        perror("error removing shared memory");
    }

    if ( msgctl(oss_qid, IPC_RMID, NULL) == -1 ) {
        perror("OSS: Error removing ODD message queue");
        exit(0);
    }
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
