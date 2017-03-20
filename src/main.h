#ifndef MAIN_H__
#define MAIN_H__

#include "cmd_svc.h"
#include <stdint.h>

#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define MAX_DISC_QUEUE 10  /** Max Discovery Queue **/
#define MAX_RSSI_COUNT 255  /** Max RSSI NORMALIZE COUNT **/
#define MAX_PACKET_COUNT 40
#define MAX_DATA_LENGTH 80

#define APP_STATUS_SUCCESS              0x0000
#define APP_STATUS_UNKNOWN              0x0001
#define APP_STATUS_ERR_MAX_DISC_QUEUE_FULL              0x0010

#define UUID16_SIZE             2                               /**< Size of 16 bit UUID */
#define UUID32_SIZE             4                               /**< Size of 32 bit UUID */
#define UUID128_SIZE            16                              /**< Size of 128 bit UUID */

#define APP_TIMER_STATUS_DISABLED                 false
#define APP_TIMER_STATUS_ENABLED                  true
#define APP_TIMER_TIMEOUT_FALSE                  false
#define APP_TIMER_TIMEOUT_TRUE                   true
    
#define APP_NET_ESTABLISHED_FALSE                   false
#define APP_NET_ESTABLISHED_TRUE                   true

#define GAP_DISC_ADDR_NOT_FOUND                   -1

typedef struct {
  uint8_t node;
  uint8_t sensor;
}p_address;

typedef struct {
  uint8_t now;
  uint8_t total;
}p_index;

typedef struct {
  uint8_t type;
  p_index index;
  p_address source;
  p_address target;
}p_header;

typedef struct {
  uint8_t p_data[MAX_DATA_LENGTH];
}p_data;

typedef struct {
  p_header header;
  p_data data;
}p_packet;


typedef struct
{
  ble_gap_addr_t peer_addr;
  int8_t         rssi;
  uint8_t         rssi_count;
} gap_data;

typedef struct
{
  uint8_t count;
  gap_data data[MAX_DISC_QUEUE];
} gap_disc;

typedef struct
{
  uint8_t id;
  ble_gap_addr_t p_addr;
}app_dev_condition;

typedef struct
{
    bool status;
    bool timeout;  
} app_timer_condition;

typedef struct
{
    uint8_t my_id;
    bool established;
    gap_disc disc;  
} app_net_condition;

typedef struct
{
    uint8_t packet_count;
    uint8_t header_count;
    uint8_t data_count;
    
    p_packet packet[MAX_PACKET_COUNT];
} app_packet_s;

typedef struct
{
    app_dev_condition dev;
    app_net_condition net;
    app_timer_condition timer;
    app_packet_s packet;
} app_condition;

extern app_condition app_state;
extern void scan_start(void);
extern const ble_gap_scan_params_t m_scan_params;
extern const ble_gap_conn_params_t m_connection_param;
#endif
