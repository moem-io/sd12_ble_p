#ifndef __LED_H__
#define __LED_H__

#include "nrf.h"
#include "app_error.h"
#include "bsp.h"
#include "nrf_delay.h"
#include "app_pwm.h"

#include "util.h"

#define len_bufferRGB 3
#define Delimiter ','

/* @LED Values */
enum State{
	Green = 13,
	Red,
	Blue,
	All,
	None
};


uint8_t* str_to_int(uint8_t * string);

ret_code_t LED_Init(void);

void LED_Not_Enough(void);
void LED_Charging(void);
void LED_Enough(void);

void LED_Not_Connect(void);
void LED_Connect(void);

bool LED_Control(char *);

static void LED_Red(uint8_t);
static void LED_Green( uint8_t value, bool flag);
static void LED_Blue( uint8_t);
#endif
