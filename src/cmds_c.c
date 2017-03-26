#include "cmds_c.h"


void data_builder(uint8_t *p_data){
    uint8_t p_idx= 0 ;
    for(int i=0;i<app_state.net.disc.count;i++){
        memcpy(&p_data[p_idx],app_state.net.disc.peer[i].p_addr.addr,BLE_GAP_ADDR_LEN);
        
        uint8_t u_rssi = -app_state.net.disc.peer[i].rssi;
        memcpy(&p_data[p_idx+BLE_GAP_ADDR_LEN],&u_rssi,sizeof(uint8_t));
        
        p_idx= p_idx+BLE_GAP_ADDR_LEN+sizeof(uint8_t);
    }
}

void packet_build(uint8_t build_cmd)
{
    p_packet* txp = &app_state.tx_p.packet[app_state.tx_p.packet_count];
    p_packet* rxp = &app_state.rx_p.packet[app_state.rx_p.process_count];
    
    switch(build_cmd)
    {
        case CMDS_C_BUILD_SCAN_RESULT:
            txp->header.type = CMDS_PACKET_TYPE_NETWORK_SCAN_RESPONSE;
            txp->header.source.node = app_state.dev.my_id;
            txp->header.source.sensor = 0;
            txp->header.target.node = app_state.dev.root_id;
            txp->header.target.sensor = 0;
            txp->header.index.now = 1;
            txp->header.index.total = 1;
        
            data_builder(txp->data.p_data);
            break;

        case CMDS_C_BUILD_PACKET_ROUTE:
            memcpy(&txp->header,&rxp->header,sizeof(CMDS_HEADER_MAX_DATA_LEN));
            memcpy(&txp->data,&rxp->data,sizeof(CMDS_DATA_MAX_DATA_LEN));
            break;
        
        default:
            break;
    }
    app_state.tx_p.packet_count ++;
}

static uint32_t cccd_configure(uint16_t conn_handle, uint16_t cccd_handle, bool enable);

void ble_cmds_c_on_db_disc_evt(ble_cmds_c_t * p_cmds_c, ble_db_discovery_evt_t * p_evt)
{
    uint32_t err_code;
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
        
        NRF_LOG_DEBUG("HEADER :%02x , CCCD : %02x",p_cmds_c->handles.header_handle, p_cmds_c->handles.header_cccd_handle);
        NRF_LOG_DEBUG("DATA :%02x , CCCD : %02x",p_cmds_c->handles.data_handle, p_cmds_c->handles.data_cccd_handle);
        NRF_LOG_DEBUG("RESULT :%02x , CCCD : %02x",p_cmds_c->handles.result_handle, p_cmds_c->handles.result_cccd_handle);
        
        err_code = cccd_configure(p_cmds_c->conn_handle,p_cmds_c->handles.header_cccd_handle, true);
        APP_ERROR_CHECK(err_code);
        
        err_code = cccd_configure(p_cmds_c->conn_handle,p_cmds_c->handles.data_cccd_handle, true);
        APP_ERROR_CHECK(err_code);
        
        err_code = cccd_configure(p_cmds_c->conn_handle,p_cmds_c->handles.result_cccd_handle, true);
        APP_ERROR_CHECK(err_code);
        
        NRF_LOG_DEBUG("NOTIFICATION ENABLED\r\n");
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

/**@brief Function for creating a message for writing to the CCCD.
 */
static uint32_t cccd_configure(uint16_t conn_handle, uint16_t cccd_handle, bool enable)
{
    uint8_t buf[BLE_CCCD_VALUE_LEN];

    buf[0] = enable ? BLE_GATT_HVX_NOTIFICATION : 0;
    buf[1] = 0;

    const ble_gattc_write_params_t write_params = {
        .write_op = BLE_GATT_OP_WRITE_REQ,
        .flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
        .handle   = cccd_handle,
        .offset   = 0,
        .len      = sizeof(buf),
        .p_value  = buf
    };

    return sd_ble_gattc_write(conn_handle, &write_params);
}
