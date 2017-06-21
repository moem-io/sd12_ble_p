#ifndef CMDS_BASE_H__
#define CMDS_BASE_H__


#include "ble.h"

#define CMDS_BASE_UUID                {0x55, 0x44, 0x33, 0x22, 0x11, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} // 128-bit base UUID
#define CMDS_UUID                      0x9000
#define CMDS_HEADER_UUID         0x9001
#define CMDS_DATA_1_UUID         0x9002
#define CMDS_DATA_2_UUID         0x9003
#define CMDS_RESULT_UUID           0x9004

static const ble_uuid_t m_cmds_uuid = {
    .uuid = CMDS_UUID,
    .type = BLE_UUID_TYPE_VENDOR_BEGIN
};

#define CEN_MAX_REQ_CNT 60

#define MAX_CHAR_LEN (GATT_MTU_SIZE_DEFAULT - 3)

#define HEADER_LEN 7+MAX_DEPTH_CNT
#define DATA_LEN 20
#define RESULT_LEN 1

#define PKT_TYPE_NET_SCAN_REQ 1
#define PKT_TYPE_NET_SCAN_RES 2
#define PKT_TYPE_SNSR_STATE_REQ 3
#define PKT_TYPE_SNSR_STATE_RES 4
#define PKT_TYPE_SNSR_DATA_REQ 5
#define PKT_TYPE_SNSR_DATA_RES 6
#define PKT_TYPE_SNSR_ACT_REQ 7
#define PKT_TYPE_SNSR_ACT_RES 8
#define PKT_TYPE_SNSR_CMD_REQ 9
#define PKT_TYPE_SNSR_CMD_RES 10

#define PKT_TYPE_NODE_STAT_REQ 17
#define PKT_TYPE_NODE_STAT_RES 18
#define PKT_TYPE_NODE_LED_REQ 19
#define PKT_TYPE_NODE_LED_RES 20
#define PKT_TYPE_NODE_BTN_PRESS_REQ 21
#define PKT_TYPE_NODE_BTN_PRESS_RES 22

#define PKT_TYPE_NET_PATH_UPDATE_REQ 101
#define PKT_TYPE_NET_PATH_UPDATE_RES 102
#define PKT_TYPE_NET_ACK_REQ 103
#define PKT_TYPE_NET_ACK_RES 104
#define PKT_TYPE_NET_JOIN_REQ 105
#define PKT_TYPE_NET_JOIN_RES 106

#define PKT_TYPE_SCAN_TGT_REQ 238
#define PKT_TYPE_SCAN_TGT_RES 239

#define PKT_DATA_ERROR 0
#define PKT_DATA_SUCCESS 1


#define CEN_SEND_TARGET_ERROR 240
#define CEN_SEND_ROUTE_ERROR 241
#define CEN_BUILD_PACKET_ROUTE 247

#define PKT_RSLT_IDLE              ((uint8_t) 0)
#define PKT_RSLT_HEADER_OK ((uint8_t) 1)
#define PKT_RSLT_DATA_1_OK      ((uint8_t) 2)
#define PKT_RSLT_DATA_2_OK      ((uint8_t) 3)
#define PKT_RSLT_INTERPRET_OK      ((uint8_t) 4)
#define PKT_RSLT_INTERPRET_ERROR      ((uint8_t) 5)

#define PKT_RSLT_ERROR      ((uint8_t) 255)


#endif
