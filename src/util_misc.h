#ifndef UTIL_MISC_H__
#define UTIL_MISC_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "nrf_delay.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"

char *uint8_t_to_str(uint8_t *data_addr, uint8_t length, bool reverse);

#define TO_STR(str, rev)                   uint8_t_to_str(str,sizeof(str),rev)
#define TO_VSTR(str, str_len, rev)        uint8_t_to_str(str,str_len,rev)

#define LOG_PUSH(str)                      nrf_log_push(str)
#define STR_PUSH(str, rev)                 nrf_log_push(TO_STR(str,rev))
#define VSTR_PUSH(str, str_len, rev)      nrf_log_push(TO_VSTR(str,str_len,rev))

#define LOG_D                               NRF_LOG_DEBUG
#define LOG_I                               NRF_LOG_INFO
#define LOG_E                               NRF_LOG_ERROR

//#define TO_UINT8(strct,len)                                     struct_to_uint8_t(strct,len)
//Struct to uint8_t
#define ERR_CHK(str) if(err_code) NRF_LOG_ERROR(str" : %d\r\n", err_code)

#endif
