#ifndef PER_H__
#define PER_H__


#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "app_error.h"
#include "ble_hci.h"

#include "cmds_base.h"
#include "util.h"

typedef struct {
    bool notification;
    uint8_t uuid_type;
    uint16_t conn_handle;
    uint16_t service_handle;
    ble_gatts_char_handles_t header_hdlrs;
    ble_gatts_char_handles_t data_1_hdlrs;
    ble_gatts_char_handles_t data_2_hdlrs;
    ble_gatts_char_handles_t result_hdlrs;
} per_t;


void pkt_interpret(per_t *p_per);

uint32_t per_init(per_t *p_per);

void app_per_evt(per_t *p_per, ble_evt_t *p_ble_evt);

uint32_t per_char_update(per_t *p_per, ble_gatts_char_handles_t *data_handle, uint8_t *p_string, uint16_t length);

#endif
