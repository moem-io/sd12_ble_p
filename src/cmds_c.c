#include "cmds_c.h"

void ble_cmds_c_on_db_disc_evt(ble_cmds_c_t * p_cmds_c, ble_db_discovery_evt_t * p_evt)
{
    ble_gatt_db_char_t * p_chars = p_evt->params.discovered_db.charateristics;

    // Check if the CMDS was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == BLE_UUID_CMDS &&
        p_evt->params.discovered_db.srv_uuid.type == p_cmds_c->uuid_type)
    {

        uint32_t i;

        for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            switch (p_chars[i].characteristic.uuid.uuid)
            {
                case BLE_UUID_CMDS_CHAR_HEADER_UUID:
                    p_cmds_c->handles.header_handle         = p_chars[i].characteristic.handle_value;
                    p_cmds_c->handles.header_cccd_handle    = p_chars[i].cccd_handle;
                    break;

                case BLE_UUID_CMDS_CHAR_DATA_UUID:
                    p_cmds_c->handles.data_handle           = p_chars[i].characteristic.handle_value;
                    p_cmds_c->handles.data_cccd_handle      = p_chars[i].cccd_handle;
                    break;

                case BLE_UUID_CMDS_CHAR_RESULT_UUID:
                    p_cmds_c->handles.result_handle         = p_chars[i].characteristic.handle_value;
                    p_cmds_c->handles.result_cccd_handle    = p_chars[i].cccd_handle;
                    break;
                
                default:
                    break;
            }
        }
        p_cmds_c->handles.assigned=true;
        NRF_LOG_DEBUG("HANDLER ASSIGNED\r\n");
    }
}


static void on_hvx(ble_cmds_c_t * p_cmds_c, const ble_evt_t * p_ble_evt)
{
    const ble_gattc_evt_hvx_t * p_evt_hvx = &p_ble_evt->evt.gattc_evt.params.hvx;
    // HVX can only occur from client sending.

    if(p_cmds_c->handles.assigned){
        if (p_evt_hvx->handle == p_cmds_c->handles.header_handle)
        {
            NRF_LOG_INFO("HEADER HANDLER [R] OR [S]");
        }
        else if (p_evt_hvx->handle == p_cmds_c->handles.header_cccd_handle)
        {
            NRF_LOG_INFO("HEADER CCCD HANDLER [R] OR [S]");
        }
        else if (p_evt_hvx->handle == p_cmds_c->handles.data_handle)
        {
            NRF_LOG_INFO("DATA HANDLER [R] OR [S]");
        }
         else if (p_evt_hvx->handle == p_cmds_c->handles.header_cccd_handle)
        {
            NRF_LOG_INFO("DATA CCCD HANDLER [R] OR [S]");
        }
         else if (p_evt_hvx->handle == p_cmds_c->handles.result_handle)
        {
            NRF_LOG_INFO("RESULT HANDLER [R] OR [S]");
        }
         else if (p_evt_hvx->handle == p_cmds_c->handles.result_cccd_handle)
        {
            NRF_LOG_INFO("RESULT CCCD HANDLER [R] OR [S]");
        }
    }
}

void ble_cmds_c_on_ble_evt(ble_cmds_c_t * p_cmds_c, const ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            p_cmds_c->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;
        
        case BLE_GATTC_EVT_HVX:
            on_hvx(p_cmds_c, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            p_cmds_c->conn_handle = BLE_CONN_HANDLE_INVALID;
            break;
        
        default:
            // No implementation needed.
            break;
    }
}

uint32_t ble_cmds_c_init(ble_cmds_c_t * p_cmds_c)
{
    uint32_t      err_code;
    ble_uuid_t    cmds_c_uuid;
    ble_uuid128_t cmds_base_uuid = BLE_UUID_CMDS_BASE_UUID;
    
    err_code = sd_ble_uuid_vs_add(&cmds_base_uuid, &cmds_c_uuid.type);
    APP_ERROR_CHECK(err_code);
    
    cmds_c_uuid.uuid = BLE_UUID_CMDS;

    p_cmds_c->uuid_type = cmds_c_uuid.type;
    p_cmds_c->conn_handle                 = BLE_CONN_HANDLE_INVALID;
    p_cmds_c->handles.header_handle = BLE_GATT_HANDLE_INVALID;
    p_cmds_c->handles.header_cccd_handle      = BLE_GATT_HANDLE_INVALID;
    p_cmds_c->handles.data_handle = BLE_GATT_HANDLE_INVALID;
    p_cmds_c->handles.data_cccd_handle      = BLE_GATT_HANDLE_INVALID;
    p_cmds_c->handles.result_handle = BLE_GATT_HANDLE_INVALID;
    p_cmds_c->handles.result_cccd_handle      = BLE_GATT_HANDLE_INVALID;
    
    return ble_db_discovery_evt_register(&cmds_c_uuid);
}

