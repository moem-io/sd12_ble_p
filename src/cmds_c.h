#ifndef CMDS_C_H__
#define CMDS_C_H__

#include <stdint.h>
#include "ble.h"
#include "ble_db_discovery.h"

#include "app_error.h"
#include "main.h"


typedef struct {
    uint16_t                header_handle;      /**< Handle of the NUS RX characteristic as provided by a discovery. */
    uint16_t                header_cccd_handle; /**< Handle of the CCCD of the NUS RX characteristic as provided by a discovery. */
    uint16_t                data_handle;      /**< Handle of the NUS TX characteristic as provided by a discovery. */
    uint16_t                data_cccd_handle; /**< Handle of the CCCD of the NUS RX characteristic as provided by a discovery. */
    uint16_t                result_handle;      /**< Handle of the NUS TX characteristic as provided by a discovery. */
    uint16_t                result_cccd_handle; /**< Handle of the CCCD of the NUS RX characteristic as provided by a discovery. */
    bool                     assigned;
} ble_cmds_c_handles_t;

typedef struct
{
    uint8_t                 uuid_type;          /**< UUID type. */
    uint16_t                conn_handle;        /**< Handle of the current connection. Set with @ref ble_nus_c_handles_assign when connected. */
    ble_cmds_c_handles_t     handles;            /**< Handles on the connected peer device needed to interact with it. */
}ble_cmds_c_t;

void ble_cmds_c_on_db_disc_evt(ble_cmds_c_t * p_cmds_c, ble_db_discovery_evt_t * p_evt);
void ble_cmds_c_on_ble_evt(ble_cmds_c_t * p_cmds_c, const ble_evt_t * p_ble_evt);


uint32_t ble_cmds_c_init(ble_cmds_c_t * p_cmds_c);

#endif