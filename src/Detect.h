#ifndef __DETECT_H__
#define __DETECT_H__

#include <stdbool.h>

#include "Sensor_Config.h"

#include "app_gpiote.h"
#include "app_error.h"
#include "nrf_drv_gpiote.h"
#include "boards.h"
#include "nrf_delay.h"

#define MAX_CHANNEL    7
#define MAX_QUEUE            6

#define pinGPOUT                28
#define pinCHG                        29

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
		char		type;
    uint32_t rising;
    uint32_t falling;
} flagDetect;

enum State_Channel {
    Falling = 0,
    Rising,
};

typedef struct {
    uint8_t buffer[MAX_QUEUE];
    uint8_t start;
    uint8_t end;
} queueSensor;

static queueSensor queue;

static uint8_t channel[MAX_CHANNEL];

static uint8_t channel_Type[MAX_CHANNEL];

static uint8_t fsmSensor[BUFFER_QUEUE_SIZE];

uint32_t Detect_Init(void);

__WEAK void Detect_CallBack(uint32_t rising, uint32_t falling);

uint8_t getState_Channel(uint8_t id);

void setState_Channel(uint8_t id, uint8_t state);

uint8_t getSensor_Channel(uint8_t id);

void setSensor_Channel(uint8_t id, uint8_t type);

bool checkChannel(uint8_t pin);

void checkEdge(__IO flagDetect* flag);

void InitalizeQueue(void);

bool push(uint8_t id);

uint8_t pop(void);

bool checkItem(uint8_t id);

bool removeItem(uint8_t id);

void setState_Set_Address(uint8_t pin);

void setState_Request(uint8_t pin);

void setState_Command(uint8_t pin);

void setState_ACK(uint8_t pin);

void setState_Response(uint8_t pin);

uint8_t getState_Sensor(uint8_t);

#endif
