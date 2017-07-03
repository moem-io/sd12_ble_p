#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <stdbool.h>
#include "nrf_drv_gpiote.h"
#include "nrf_delay.h"
#include "app_error.h"
#include "boards.h"

/* @GPIO Definition */
#define Button_IN                                                                                                    12

__WEAK void Button_Click_CallBack(void);

ret_code_t Button_Init(void);

bool checkButton(void);

#endif
