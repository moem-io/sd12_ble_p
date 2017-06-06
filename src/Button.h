#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <stdbool.h>
#include "nrf_drv_gpiote.h"
#include "app_error.h"
#include "boards.h"

/* @GPIO Definition */
#define Button_IN 																									12

ret_code_t Button_Init(void);

void Button_Click_CallBack(void);

#endif
