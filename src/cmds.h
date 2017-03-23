#ifndef CMDS_H__
#define CMDS_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "app_error.h"

#include "util.h"
#include "main.h"

#define BLE_UUID_CMD_SVC_BASE_UUID              {0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} // 128-bit base UUID
#define BLE_UUID_CMD_SVC                0xA000

#define BLE_UUID_CMD_CHAR_HEADER_UUID          0xA001
#define BLE_UUID_CMD_CHAR_DATA_UUID             0xA002
#define BLE_UUID_CMD_CHAR_RESULT_UUID          0xA003

#define BLE_CMD_SVC_MAX_DATA_LEN (GATT_MTU_SIZE_DEFAULT - 3)
#define BLE_CMD_SVC_RESULT_CHAR_MAX_DATA_LEN 2

#define BLE_CMD_SVC_PACKET_TYPE_NETWORK_SCAN_REQUEST 1
#define BLE_CMD_SVC_PACKET_TYPE_NETWORK_SCAN_RESPONSE 2


typedef struct
{
    uint8_t                                 uuid_type;
    uint16_t                                service_handle;
    ble_gatts_char_handles_t     header_handles;                                          /**< Handles related to the Heart Rate Measurement characteristic. */
    ble_gatts_char_handles_t     data_handles;                                          /**< Handles related to the Body Sensor Location characteristic. */
    ble_gatts_char_handles_t     result_handles;                                         /**< Handles related to the Heart Rate Control Point characteristic. */  
    uint16_t                    conn_handle; 
    bool                     is_notification_enabled; /**< Variable to indicate if the peer has enabled notification of the RX characteristic.*/

}ble_cmds_t;

static const ble_uuid_t m_cmds_uuid =
  {
    .uuid = BLE_UUID_CMD_SVC,
    .type = BLE_UUID_TYPE_VENDOR_BEGIN
  };


uint32_t cmd_service_init(ble_cmds_t * p_cmd_service);
void ble_cmds_on_ble_evt(ble_cmds_t * p_cmds, ble_evt_t * p_ble_evt);
uint32_t cmd_header_char_update(ble_cmds_t *p_cmd_service, uint8_t * p_string, uint16_t length);
uint32_t cmd_header_data_update(ble_cmds_t *p_cmd_service, uint8_t * p_string, uint16_t length);
uint32_t cmd_header_result_update(ble_cmds_t *p_cmd_service, uint8_t * p_string, uint16_t length);


#endif
