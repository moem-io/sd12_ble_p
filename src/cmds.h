#ifndef CMDS_H__
#define CMDS_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "app_error.h"

#include "cmds_base.h"
#include "util.h"
#include "main.h"

#define BLE_CMDS_MAX_DATA_LEN (GATT_MTU_SIZE_DEFAULT - 3)
#define BLE_CMDS_RESULT_CHAR_MAX_DATA_LEN 2

#define BLE_CMDS_PACKET_TYPE_NETWORK_SCAN_REQUEST 1
#define BLE_CMDS_PACKET_TYPE_NETWORK_SCAN_RESPONSE 2


typedef struct
{
    uint8_t                                 uuid_type;
    uint16_t                                service_handle;
    ble_gatts_char_handles_t     header_handles;                                          /**< Handles related to the Heart Rate Measurement characteristic. */
    ble_gatts_char_handles_t     data_handles;                                          /**< Handles related to the Body Sensor Location characteristic. */
    ble_gatts_char_handles_t     result_handles;                                         /**< Handles related to the Heart Rate Control Point characteristic. */  
    uint16_t                    conn_handle; 
    bool                     is_notification_enabled; /**< Variable to indicate if the peer has enabled notification of each characteristic.*/
}ble_cmds_t;



uint32_t cmds_init(ble_cmds_t * p_cmd_service);
void ble_cmds_on_ble_evt(ble_cmds_t * p_cmds, ble_evt_t * p_ble_evt);
uint32_t cmd_header_char_update(ble_cmds_t *p_cmd_service, uint8_t * p_string, uint16_t length);
uint32_t cmd_header_data_update(ble_cmds_t *p_cmd_service, uint8_t * p_string, uint16_t length);
uint32_t cmd_header_result_update(ble_cmds_t *p_cmd_service, uint8_t * p_string, uint16_t length);


#endif
