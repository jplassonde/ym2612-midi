#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include "channel.h"
#include "instruments.h"


CHANQUEUE * create_queue(int size) {
	CHANQUEUE *chqueue = calloc(1, sizeof(CHANQUEUE));
	if (!chqueue) {
		printf("Malloc failed while creating queue\n");
		return NULL;
	}
	chqueue->front=-1;
	chqueue->rear=-1;
	chqueue->capacity = size;
	chqueue->array = calloc(size, sizeof(CHANNEL*));
	if (!chqueue->array) {
		printf("Malloc failed while creating queue array\n");
		return NULL;
	}
	return chqueue;
}

int queue_not_empty(CHANQUEUE * chqueue) {
	return (chqueue->front != -1);
}


void release_chan(CHANQUEUE * chqueue, CHANNEL *chan) {
	chqueue->rear = (chqueue->rear + 1) % chqueue->capacity;
	chqueue->array[chqueue->rear] = chan;
	
	if (chqueue->front == -1) {
		chqueue->front = 0;
	}
}

CHANNEL* get_chan(CHANQUEUE * chqueue) {
	CHANNEL * ch;
	if (chqueue->front == -1) {
		printf("Tried to acquire channel while the queue is empty.\n");
		exit(0);
	}
	
	ch = chqueue->array[chqueue->front];
	
	if (chqueue->front == chqueue->rear) {
		chqueue->front = -1;
		chqueue->rear = -1;
	} else if (chqueue->front == chqueue->capacity-1) {
		chqueue->front = 0;
	} else {
		++chqueue->front;
	}
	return ch;
}
