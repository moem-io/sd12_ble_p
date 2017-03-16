#ifndef CMD_SVC_H__
#define CMD_SVC_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "app_error.h"

#include "main.h"

#define BLE_UUID_CMD_SVC_BASE_UUID              {0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} // 128-bit base UUID
#define BLE_UUID_CMD_SVC                0xA000

#define BLE_UUID_CMD_CHAR_HEADER_UUID          0xA001
#define BLE_UUID_CMD_CHAR_DATA_UUID             0xA002
#define BLE_UUID_CMD_CHAR_RESULT_UUID          0xA003

typedef struct
{
    uint16_t    service_handle;
    ble_gatts_char_handles_t     header_handles;                                          /**< Handles related to the Heart Rate Measurement characteristic. */
    ble_gatts_char_handles_t     data_handles;                                          /**< Handles related to the Body Sensor Location characteristic. */
    ble_gatts_char_handles_t     result_handles;                                         /**< Handles related to the Heart Rate Control Point characteristic. */  
    uint16_t                    conn_handle; 

}ble_cmd_svc_t;

static const ble_uuid_t m_cmd_svc_uuid =
  {
    .uuid = BLE_UUID_CMD_SVC,
    .type = BLE_UUID_TYPE_VENDOR_BEGIN
  };

void cmd_service_init(ble_cmd_svc_t * p_cmd_service);

#endif
