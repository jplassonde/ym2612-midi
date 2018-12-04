#ifndef _CHANNEL_G
#define _CHANNEL_G

#include "instruments.h"

typedef struct CHANNEL {
    INSTRUMENT * instrument;
    uint8_t channel_number;
    uint8_t op1_addr;
    uint8_t op2_addr;
    uint8_t op3_addr;
    uint8_t op4_addr;
    uint8_t ch_addr;
    uint8_t part;
    uint8_t chip_cs;
	uint8_t pan;
	uint8_t volume;
	uint8_t key;
} CHANNEL;

typedef struct CHANQUEUE {
    int front, rear;
    int capacity;
    CHANNEL ** array;
} CHANQUEUE;

CHANQUEUE * create_queue(int size);
void release_chan(CHANQUEUE * chanqueue, CHANNEL * channel);
CHANNEL * get_chan(CHANQUEUE * chqueue);
int queue_not_empty(CHANQUEUE * chqueue);
#endif
