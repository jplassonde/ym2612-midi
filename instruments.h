#ifndef _INSTRUMENT_G
#define _INSTRUMENT_G

#include <stdint.h>

typedef struct OPERATOR {
    uint8_t dt1;
    uint8_t mul;
    uint8_t tl;
    uint8_t rs;
    uint8_t ar;
    uint8_t am;
    uint8_t d1r;
    uint8_t d2r;
    uint8_t d1l;
    uint8_t rr;
    uint8_t ssg_eg;
} OPERATOR;

typedef struct INSTRUMENT {
    OPERATOR op1;
    OPERATOR op2;
    OPERATOR op3;
    OPERATOR op4;
    uint8_t ops_enabled;
    uint8_t lfo_enabled;
    uint8_t ch1_feedback;
    uint8_t algo;
    uint8_t pan; // Move to channel... or  ignore? 
    uint8_t ams;
    uint8_t fms;
} INSTRUMENT;

typedef struct PATCH_LIST {
	INSTRUMENT * instrument;
	unsigned int count;
} PATCH_LIST;

void create_instruments(PATCH_LIST ** patch_list);

#endif
