#ifndef QUEUE_H
#define QUEUE_H
 
#include <stdlib.h>
#include "shared.h"

int bitMap[19];
int q0[19];
int q1[19];
int q2[19];
int q3[19];

/* Initialize the queue */
void createQueue(int q[], int size) 
{
    	int i;
    	for(i=0; i<size; i++) 
	{
        	q[i] = 0;
    	}
}

/* Chekc the status of queue members */
int checkQueue() 
{
    	if (q0[1] != 0) 
	{
        	return 0;
    	}
    	else if (q1[1] != 0) 
	{
        	return 1;
    	}
    	else if (q2[1] != 0) 
	{
        	return 2;
    	}
    	else if (q3[1] != 0) 
	{
        	return 3;
    	}
    	else return -1;
}

/* Add member to appropriate queue */
int addToQueue (int q[], int arrsize, int pNum) 
{
    	int i;
    	for (i=1; i<arrsize; i++) 
	{
        	if (q[i] == 0) 
		{ 
            		q[i] = pNum;
            		return 1;
        	}
    	}
    	return -1; 
}

/* Delete a member from appropriate queue */
int deleteFromQueue(int q[], int arrsize, int pNum) 
{
    	int i;
    	for (i=1; i<arrsize; i++) 
	{
        	if (q[i] == pNum) 
		{ 
            		while(i+1 < arrsize) 
			{ 
                		q[i] = q[i+1];
                		i++;
            		}
            		q[18] = 0; 
            		return 1;
        	}
    	}
    	return -1;
}

/* Initialize the bit vector */
void bitMapF(int n) 
{
    	int i;
    	for (i=0; i<n; i++) 
	{
        	bitMap[i] = 0;
    	}
}

/* Check the bit vector in specified spot */
int getBitSpot() 
{
    	int i;
    	int rvalue = -1;
    	for (i=1; i<19; i++) 
	{
        	if (bitMap[i] == 0) 
		{
            		rvalue = i;
            		break;
        	}
    	}
    	return rvalue;
}

#endif 
