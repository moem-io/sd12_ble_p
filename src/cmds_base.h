#ifndef CMDS_BASE_H__
#define CMDS_BASE_H__

#include "ble.h"

#define CMDS_BASE_UUID                {0x55, 0x44, 0x33, 0x22, 0x11, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} // 128-bit base UUID
#define CMDS_UUID                      0x9000
#define CMDS_CHAR_HEADER_UUID         0x9001
#define CMDS_CHAR_DATA_1_UUID         0x9002
#define CMDS_CHAR_DATA_2_UUID         0x9003
#define CMDS_CHAR_RESULT_UUID           0x9004

static const ble_uuid_t m_cmds_uuid = {
    .uuid = CMDS_UUID,
    .type = BLE_UUID_TYPE_VENDOR_BEGIN
};

#define CMDS_PKT_TYPE_NET_SCAN_REQUEST 1
#define CMDS_PKT_TYPE_NET_SCAN_RESPONSE 2


#define CMDS_PKT_RSLT_IDLE              ((uint8_t) 0)
#define CMDS_PKT_RSLT_HEADER_OK ((uint8_t) 1)
#define CMDS_PKT_RSLT_DATA_1_OK      ((uint8_t) 2)
#define CMDS_PKT_RSLT_DATA_2_OK      ((uint8_t) 3)
#define CMDS_PKT_RSLT_INTERPRET_OK      ((uint8_t) 4)
#define CMDS_PKT_RSLT_INTERPRET_ERROR      ((uint8_t) 5)

#define CMDS_PKT_RSLT_ERROR      ((uint8_t) 255)


#endif
