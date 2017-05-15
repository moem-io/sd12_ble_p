#ifndef CMDS_H__
#define CMDS_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "app_error.h"
#include "ble_hci.h"

#include "cmds_base.h"
#include "util.h"
#include "main.h"

#define CMDS_MAX_DATA_LEN (GATT_MTU_SIZE_DEFAULT - 3)

#define CMDS_HEADER_MAX_LEN 7
#define CMDS_DATA_MAX_LEN 20
#define CMDS_RESULT_MAX_LEN 1

typedef struct
{
    bool                                    notification;
    uint8_t                                 uuid_type;
    uint16_t                                conn_handle; 
    uint16_t                                service_handle;
    ble_gatts_char_handles_t     header_handles;                                          /**< Handles related to the Heart Rate Measurement characteristic. */
    ble_gatts_char_handles_t     data_1_handles;                                          /**< Handles related to the Body Sensor Location characteristic. */
    ble_gatts_char_handles_t     data_2_handles;                                          /**< Handles related to the Body Sensor Location characteristic. */
    ble_gatts_char_handles_t     result_handles;                                         /**< Handles related to the Heart Rate Control Point characteristic. */  
}ble_cmds_t;


void packet_interpret(ble_cmds_t * p_cmds, ble_evt_t * p_ble_evt);

uint32_t cmds_init(ble_cmds_t * p_cmd_service);
void ble_cmds_on_ble_evt(ble_cmds_t * p_cmds, ble_evt_t * p_ble_evt);

uint32_t cmds_value_update(ble_cmds_t *p_cmds,ble_gatts_char_handles_t* data_handle, uint8_t * p_string, uint16_t length);

#endif
