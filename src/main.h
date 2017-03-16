#ifndef MAIN_H__
#define MAIN_H__

#include "cmd_svc.h"

#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define MAX_DISC_QUEUE 10  /** Max Discovery Queue **/

#define APP_STATUS_SUCCESS              0x0000
#define APP_STATUS_UNKNOWN              0x0001
#define APP_STATUS_ERR_MAX_DISC_QUEUE_FULL              0x0010

#define UUID16_SIZE             2                               /**< Size of 16 bit UUID */
#define UUID32_SIZE             4                               /**< Size of 32 bit UUID */
#define UUID128_SIZE            16                              /**< Size of 128 bit UUID */


typedef struct
{
  ble_gap_addr_t peer_addr;                     /**< Bluetooth address of the peer device. */
  int8_t         rssi;                          /**< Received Signal Strength Indication in dBm. */
  uint8_t         rssi_count;
} gap_data;

#define MAX_RSSI_COUNT 255  /** Max RSSI NORMALIZE COUNT **/

typedef struct
{
  gap_data data[MAX_DISC_QUEUE];
} gap_disc;

#include <stdint.h>

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
  p_address source;
  p_address target;
  p_index index;
}p_header;

typedef struct {
  uint8_t p_data[20];
}p_body;

typedef struct {
  p_header header;
  p_body data;
}packet;


#endif
