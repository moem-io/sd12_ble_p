#ifndef MAIN_H__
#define MAIN_H__


#include "per.h"
#include "cen.h"
#include <stdint.h>

#define MIN_DISC_REG_CNT 5
#define MAX_DISC_QUEUE 5  /** Max Discovery Queue **/
#define MAX_RSSI_CNT 255  /** Max RSSI NORMALIZE COUNT **/
#define MAX_PKT_CNT 100
#define MAX_PKT_DATA_LEN 40
#define MAX_DEV_NAME 10
#define MAX_NODE_CNT 40
#define MAX_DEPTH_CNT 5 

#define APP_STATUS_SUCCESS              0x0000
#define APP_STATUS_UNKNOWN              0x0001
#define APP_STATUS_ERR_MAX_DISC_QUEUE_FULL              0x0010

#define APP_TIMER_STATUS_DISABLED                 false
#define APP_TIMER_STATUS_ENABLED                  true
#define APP_TIMER_TIMEOUT_FALSE                  false
#define APP_TIMER_TIMEOUT_TRUE                   true

#define APP_NET_ESTABLISHED_FALSE                   false
#define APP_NET_ESTABLISHED_TRUE                   true

#define APP_NET_DISCOVERED_FALSE                   false
#define APP_NET_DISCOVERED_TRUE                   true

typedef struct {
    uint8_t node;
    uint8_t sensor;
} p_address;

typedef struct {
    uint8_t err;
    uint8_t total;
} p_index;

// TODO: err type to Type Struct
typedef struct {
    uint8_t type;
    p_index index;
    p_address source;
    p_address target;
} p_header;

typedef struct {
    uint8_t p_data[MAX_PKT_DATA_LEN];
} p_data;

typedef struct {
    uint8_t result;
} p_result;

typedef struct {
    p_header header;
    p_data data;
    p_result result;
} p_pkt;


typedef struct {
    uint8_t id;
    ble_gap_addr_t p_addr;
    int8_t rssi;
    uint8_t rssi_cnt;
    bool disc;
    uint8_t path[MAX_DEPTH_CNT];
} gap_data;

typedef struct {
    uint8_t cnt;
    gap_data peer[MAX_NODE_CNT];
} gap_disc;

typedef struct {
    uint8_t my_id;
    uint8_t root_id; //ALWAYS 0
    char name[MAX_DEV_NAME];
    ble_gap_addr_t my_addr;
    ble_gap_addr_t conn_cen;
    ble_gap_addr_t parent;
    bool parent_set;
} app_dev_condition;

typedef struct {
    bool status;
    bool timeout;
} app_timer_condition;

typedef struct {
    bool established;
    bool discovered;
    gap_disc node;
} app_net_condition;

typedef struct {
    app_dev_condition dev;
    app_net_condition net;
    app_timer_condition timer;
} app_condition;

typedef struct {
    uint8_t pkt_cnt;
    uint8_t header_cnt;
    uint8_t data_cnt;
    bool proc;
    uint8_t proc_cnt;
    p_pkt pkt[MAX_PKT_CNT];
} app_pkt_rx;

typedef struct {
    uint8_t pkt_cnt;

    uint8_t que_idx;
    int8_t tx_que[MAX_PKT_CNT];

    bool proc;
    uint8_t proc_cnt;

    p_pkt pkt[MAX_PKT_CNT];
} app_pkt_tx;

typedef struct {
    app_pkt_rx rx_p;
    app_pkt_tx tx_p;
} app_packet;

extern app_condition APP;
extern app_packet PKT;

extern void scan_start(void);

extern const ble_gap_scan_params_t m_scan_params;
extern const ble_gap_conn_params_t m_connection_param;
#endif
