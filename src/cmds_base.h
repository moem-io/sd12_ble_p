#ifndef CMDS_BASE_H__
#define CMDS_BASE_H__

#include "ble.h"

#define BLE_UUID_CMDS_BASE_UUID              {0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} // 128-bit base UUID
#define BLE_UUID_CMDS                                           0xA000
#define BLE_UUID_CMDS_CHAR_HEADER_UUID         0xA001
#define BLE_UUID_CMDS_CHAR_DATA_UUID              0xA002
#define BLE_UUID_CMDS_CHAR_RESULT_UUID          0xA003

static const ble_uuid_t m_cmds_uuid =
  {
    .uuid = BLE_UUID_CMDS,
    .type = BLE_UUID_TYPE_VENDOR_BEGIN
  };
  
#define CMDS_PACKET_TYPE_NETWORK_SCAN_REQUEST 1
#define CMDS_PACKET_TYPE_NETWORK_SCAN_RESPONSE 2

  
#define CMDS_PACKET_RESULT_IDLE              ((uint8_t) 0)
#define CMDS_PACKET_RESULT_HEADER_OK ((uint8_t) 1)
#define CMDS_PACKET_RESULT_DATA_OK      ((uint8_t) 2)
#define CMDS_PACKET_RESULT_INTERPRET_OK      ((uint8_t) 3)
#define CMDS_PACKET_RESULT_INTERPRET_ERROR      ((uint8_t) 4)

#define CMDS_PACKET_RESULT_ERROR      ((uint8_t) 255)


#endif
