#ifndef CMD_SVC_H__
#define CMD_SVC_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "app_error.h"

#include "main.h"

#define BLE_UUID_CMD_SVC_BASE_UUID              {0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} // 128-bit base UUID
#define BLE_UUID_CMD_SVC                0xABCD

#define BLE_UUID_CMD_CHAR_HEADER_UUID          0xA001
#define BLE_UUID_CMD_CHAR_DATA_UUID             0xA002
#define BLE_UUID_CMD_CHAR_RESULT_UUID          0xA003

typedef struct
{
    uint16_t                    conn_handle; 
    uint16_t    service_handle;     /**< Handle of Our Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t    char_handles;

}ble_cmd_svc_t;

void cmd_service_init(ble_cmd_svc_t * p_cmd_service);

#endif
