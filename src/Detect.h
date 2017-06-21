#ifndef __DETECT_H__
#define __DETECT_H__

#include <stdbool.h>
#include "app_gpiote.h"
#include "app_error.h"
#include "nrf_drv_gpiote.h"
#include "boards.h"
#include "nrf_delay.h"

#define MAX_CHANNEL 	7
#define MAX_QUEUE			6

#define pinID1 23
#define pinID2 24
#define pinID3 25
#define pinID4 3
#define pinID5 17

#define pinGPOUT				28
#define pinCHG						29

#define PIN_ID1_BITMASK (1<<pinID1)
#define PIN_ID2_BITMASK (1<<pinID2) 
#define PIN_ID3_BITMASK (1<<pinID3)
#define PIN_ID4_BITMASK (1<<pinID4)
#define PIN_ID5_BITMASK (1<<pinID5)  

#define PIN_ID_GPOUT_BITMASK (1<<pinGPOUT)
#define PIN_ID_CHG_BITMASK (1<<pinCHG)

typedef struct {
	bool flag;
	uint8_t pin;
	uint8_t state;
	uint32_t rising;
	uint32_t falling;
}flagDetect;

enum num_ID{
	ID1 = 0,
	ID2,
	ID3,
	ID4,
	ID5,
	ID_GPOUT,
	ID_CHG,
	ID_BUTTON,
	ID_None
};

enum State_Channel{
	Falling = 0,
	Rising ,
};

typedef struct{
	uint8_t buffer[MAX_QUEUE];
	uint8_t start;
	uint8_t end;
}queueSensor;

static queueSensor queue;

static uint8_t channel[MAX_CHANNEL];

uint32_t Detect_Init(void);

__WEAK void Detect_CallBack(uint32_t rising, uint32_t falling);

uint8_t getState_Channel(uint8_t id);
void setState_Channel(uint8_t id, uint8_t state);
bool checkChannel(uint8_t pin);

void checkEdge(__IO flagDetect* flag);

void InitalizeQueue(void);
bool push(uint8_t id);
uint8_t pop(void);

bool checkItem(uint8_t id);
bool removeItem(uint8_t id);

#endif
