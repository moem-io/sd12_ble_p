#ifndef CMD_SVC_H__
#define CMD_SVC_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "app_error.h"

#include "main.h"

#define BLE_UUID_CMD_SVC_BASE_UUID              {0x34, 0x12, 0x00, 0x00, 0x34, 0x12, 0x00, 0x00, 0x34, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} // 128-bit base UUID
#define BLE_UUID_CMD_SERVICE                0xABCD

typedef struct
{
    uint16_t    service_handle;     /**< Handle of Our Service (as provided by the BLE stack). */
}ble_cmd_svc_t;

void cmd_service_init(ble_cmd_svc_t * p_cmd_service);

#endif
