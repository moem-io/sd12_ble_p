#ifndef UTIL_APP_H__
#define UTIL_APP_H__


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"

#define UUID16_SIZE             2                               /**< Size of 16 bit UUID */
#define UUID32_SIZE             4                               /**< Size of 32 bit UUID */
#define UUID128_SIZE            16                              /**< Size of 128 bit UUID */

#define GAP_DISC_ROOT_FOUND        -1
#define GAP_DISC_ADDR_NOT_FOUND  NULL
#define GAP_DISC_ID_NOT_FOUND        NULL

int8_t get_id_idx(uint8_t *id);
int8_t get_addr_idx(uint8_t *p_data);

ble_gap_addr_t *get_node(uint8_t *p_data, bool id, bool addr);

void update_node(p_pkt *rxp);

void app_dev_parent_set(ble_gap_addr_t *addr);

bool is_uuid_present(const ble_uuid_t *p_target_uuid, const ble_gap_evt_adv_report_t *adv_report);

#endif
