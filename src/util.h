#ifndef UTIL_H__
#define UTIL_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define STR(str,rev)                                                  uint8_t_to_str(str,sizeof(str),rev)
#define VSTR(str,str_len,rev)                                     uint8_t_to_str(str,str_len,rev)

#define LOG_PUSH(str)                                             nrf_log_push(str)
#define STR_PUSH(str,rev)                                        nrf_log_push(STR(str,rev))
#define VSTR_PUSH(str,str_len,rev)                          nrf_log_push(VSTR(str,str_len,rev))

char* uint8_t_to_str(uint8_t *data_addr,uint8_t length, bool reverse);

#endif
