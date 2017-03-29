#ifndef CMDS_H__
#define CMDS_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "app_error.h"

#include "cmds_base.h"
#include "cmds_c.h"
#include "util.h"
#include "main.h"

#define CMDS_MAX_DATA_LEN (GATT_MTU_SIZE_DEFAULT - 3)

#define CMDS_HEADER_MAX_DATA_LEN 7
#define CMDS_DATA_MAX_DATA_LEN 20
#define CMDS_RESULT_MAX_DATA_LEN 1

#define CMDS_PACKET_TYPE_NETWORK_SCAN_REQUEST 1
#define CMDS_PACKET_TYPE_NETWORK_SCAN_RESPONSE 2

#define CMDS_PACKET_RESULT_IDLE              ((uint8_t) 0)
#define CMDS_PACKET_RESULT_HEADER_OK ((uint8_t) 1)
#define CMDS_PACKET_RESULT_DATA_OK      ((uint8_t) 2)
#define CMDS_PACKET_RESULT_ERROR      ((uint8_t) 255)

typedef struct { 
    bool                    header;
    bool                    data;
    bool                    result;
    bool                     all;
} ble_cmds_notification_t;

typedef struct
{
    uint8_t                                 uuid_type;
    uint16_t                                conn_handle; 
    uint16_t                                service_handle;
    ble_gatts_char_handles_t     header_handles;                                          /**< Handles related to the Heart Rate Measurement characteristic. */
    ble_gatts_char_handles_t     data_handles;                                          /**< Handles related to the Body Sensor Location characteristic. */
    ble_gatts_char_handles_t     result_handles;                                         /**< Handles related to the Heart Rate Control Point characteristic. */  
    ble_cmds_notification_t notification;
}ble_cmds_t;


void packet_interpret(ble_cmds_t * p_cmds, ble_evt_t * p_ble_evt);

uint32_t cmds_init(ble_cmds_t * p_cmd_service);
void ble_cmds_on_ble_evt(ble_cmds_t * p_cmds, ble_evt_t * p_ble_evt);

uint32_t cmds_value_update(ble_cmds_t *p_cmds,ble_gatts_char_handles_t* data_handle, uint8_t * p_string, uint16_t length);

#endif
