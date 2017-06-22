#ifndef CEN_H__
#define CEN_H__


#include <stdint.h>
#include "ble.h"
#include "ble_db_discovery.h"

#include "cmds_base.h"

#include "app_error.h"
#include "util.h"

#define CEN_TXP_QUEUE_UNAVAILABLE -1

typedef struct {
    uint16_t header_hdlr;
    uint16_t data_1_hdlr;
    uint16_t data_2_hdlr;
    uint16_t result_hdlr;
    uint16_t result_cccd_hdlr;
    bool assigned;
} cen_handlers_t;

typedef struct {
    bool idle;
    bool header;
    bool data_1;
    bool data_2;
    bool send;
    bool interpret;
} cen_state_t;

typedef struct {
    bool notification;
    uint8_t uuid_type;          /**< UUID type. */
    uint16_t conn_handle;        /**< Handle of the current connection. Set with @ref ble_nus_c_handles_assign when connected. */
    cen_handlers_t hdlrs;            /**< Handles on the connected peer device needed to interact with it. */
    cen_state_t state;
} cen_t;

void cen_on_db_disc_evt(cen_t *p_cen, ble_db_discovery_evt_t *p_evt);

void app_cen_evt(cen_t *p_cen, const ble_evt_t *p_ble_evt);

void pkt_build(uint8_t build_type, uint8_t *p_data, uint8_t sensor);

void pkt_send(cen_t *p_cen);

uint32_t cen_init(cen_t *p_cen);

#endif
