#ifndef __LED_H__
#define __LED_H__

#include "nrf.h"
#include "app_error.h"
#include "bsp.h"
#include "nrf_delay.h"
#include "app_pwm.h"

/* @LED Values */
enum State{
	Green = 13,
	Red,
	Blue,
	All,
	None
};

ret_code_t LED_Init(void);

void LED_Not_Enough(void);
void LED_Charging(void);
void LED_Enough(void);

void LED_Not_Connect(void);
void LED_Connect(void);

void LED_Red(bool, uint8_t);
void LED_Green(bool, uint8_t);
void LED_Blue(bool, uint8_t);
#endif
