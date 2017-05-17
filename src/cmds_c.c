#include "cmds_c.h"

static uint32_t cccd_configure(uint16_t conn_handle, uint16_t cccd_handle, bool enable);
static uint32_t ble_cmds_c_notif_enable(ble_cmds_c_t * p_cmds_c, uint16_t * handle);
static uint32_t cmds_c_value_update(ble_cmds_c_t * p_cmds_c, uint16_t* handle, uint8_t * p_string, uint16_t length);

uint32_t cmds_c_header_update(ble_cmds_c_t* p_cmds_c, p_header* header)
{
    uint8_t buff[20];
    memcpy(buff, header,sizeof(p_header));
    return cmds_c_value_update(p_cmds_c,&p_cmds_c->handles.header_handle,buff,sizeof(p_header));
}

uint32_t cmds_c_data_1_update(ble_cmds_c_t* p_cmds_c, p_data* data)
{
    uint8_t buff[20];
    memcpy(buff, &data->p_data[0],sizeof(buff));
    return cmds_c_value_update(p_cmds_c,&p_cmds_c->handles.data_1_handle,buff,sizeof(buff));
}

uint32_t cmds_c_data_2_update(ble_cmds_c_t* p_cmds_c, p_data* data)
{
    uint8_t buff[20];
    memcpy(buff, &data->p_data[20],sizeof(buff));
    return cmds_c_value_update(p_cmds_c,&p_cmds_c->handles.data_2_handle,buff,sizeof(buff));
}


void pkt_send(ble_cmds_c_t* p_cmds_c)
{
    uint32_t err_code;
    
    if(APP.tx_p.process){
        p_pkt* txp = &APP.tx_p.pkt[APP.tx_p.process_cnt];
   
        if (p_cmds_c->conn_handle == BLE_CONN_HANDLE_INVALID)
        {
            ble_gap_addr_t* target_addr = app_disc_id_check(&txp->header.target.node);
            if(target_addr){
                LOG_I("WAIT FOR PERIPHERAL - TARGET %s\r\n",STR_PUSH(target_addr->addr,1));
                err_code = sd_ble_gap_connect(target_addr,&m_scan_params,&m_connection_param);
                ERR_CHK("Connection Request Failed");
                return;
            }
            LOG_E("TARGET NOT FOUND\r\n");
            return;
        }
        
        if(!p_cmds_c->handles.assigned){
            LOG_I("WAIT FOR HANDLE ASSIGNED\r\n");
            APP.tx_p.process = false;
            return;
        }

        if(!p_cmds_c->notification){
            err_code = ble_cmds_c_notif_enable(p_cmds_c,&p_cmds_c->handles.result_cccd_handle);
            ERR_CHK("Noti Enable Failed");
            return;
        }
  
        if(!p_cmds_c->state.idle){
            LOG_D("Wait For IDLE\r\n");
            return;
        }

        if(p_cmds_c->state.send){
            LOG_D("Wait For Response\r\n");
            return;
        }
        
        else if(!p_cmds_c->state.send){
            p_cmds_c->state.send = true;
            
            if(!p_cmds_c->state.header){
                err_code = cmds_c_header_update(p_cmds_c,&txp->header);
                ERR_CHK("Header Update Failed");
                LOG_I("Header UPDATE SUCCESS\r\n");
                return;
            }
            
            if(!p_cmds_c->state.data_1){
                err_code = cmds_c_data_1_update(p_cmds_c,&txp->data);
                ERR_CHK("Data 1 Update Failed");
                LOG_I("Data 1 UPDATE SUCCESS\r\n");
                return;
            }
            
            if(txp->header.index.total >= 2) {
							if(!p_cmds_c->state.data_2){
									err_code = cmds_c_data_2_update(p_cmds_c,&txp->data);
									ERR_CHK("Data 2 Update Failed");
									LOG_I("Data 2 UPDATE SUCCESS\r\n");
									return;
							}
					}
        }
        
        if(!p_cmds_c->state.interpret){
            LOG_I("WAIT FOR PACKET INTERPRETING\r\n");
            nrf_delay_ms(100);
            return;
        }
        
        else if(p_cmds_c->state.interpret){
            APP.tx_p.tx_queue[APP.tx_p.process_cnt] = CMDS_C_TXP_QUEUE_UNAVAILABLE;
            APP.tx_p.process_cnt++;
            if(APP.tx_p.tx_queue[APP.tx_p.process_cnt] == CMDS_C_TXP_QUEUE_UNAVAILABLE){
                APP.tx_p.process = false;
            }
        }
    }

}

//Function name Rename
void data_builder(uint8_t *p_data)
{
    uint8_t p_idx= 0;
		uint8_t unit = BLE_GAP_ADDR_LEN+sizeof(uint8_t);
	
    for(int i=0;i<APP.net.disc.count;i++){
        memcpy(&p_data[p_idx],APP.net.disc.peer[i].p_addr.addr,BLE_GAP_ADDR_LEN);
        
        uint8_t u_rssi = -APP.net.disc.peer[i].rssi;
        memcpy(&p_data[p_idx+BLE_GAP_ADDR_LEN],&u_rssi,sizeof(uint8_t));
        
        p_idx= p_idx+unit;
    }
		LOG_I("PACKET BUILD %s, \r\n",VSTR_PUSH(p_data,APP.net.disc.count*unit,0));
}

void pkt_build(uint8_t build_cmd)
{
    LOG_I("PACKET BUILD TXP : %d, RXP : %d, \r\n",APP.tx_p.pkt_cnt,APP.rx_p.process_cnt);
    p_pkt* txp = &APP.tx_p.pkt[APP.tx_p.pkt_cnt];
    p_pkt* rxp = &APP.rx_p.pkt[APP.rx_p.process_cnt];

    switch(build_cmd)
    {
        case CMDS_C_BUILD_SCAN_RESULT:
            txp->header.type = CMDS_PKT_TYPE_NET_SCAN_RESPONSE;
            txp->header.source.node = APP.dev.my_id;
            txp->header.source.sensor = 0;
            txp->header.target.node = APP.dev.root_id;
            txp->header.target.sensor = 0;
            txp->header.index.now = 1;
            txp->header.index.total = (int) ceil((float)APP.net.disc.count*8/CMDS_DATA_MAX_LEN);
            
            data_builder(txp->data.p_data);
            break;

        case CMDS_C_BUILD_PACKET_ROUTE:
            memcpy(&txp->header,&rxp->header,CMDS_HEADER_MAX_LEN);
            memcpy(&txp->data,&rxp->data,MAX_DATA_LEN);
            break;
        
        default:
            break;
    }
    APP.tx_p.process = true;
    APP.tx_p.tx_queue[APP.tx_p.queue_index] = APP.tx_p.pkt_cnt;
    APP.tx_p.pkt_cnt++;
    APP.tx_p.queue_index++;
}

void ble_cmds_c_on_db_disc_evt(ble_cmds_c_t * p_cmds_c, ble_db_discovery_evt_t * p_evt)
{
    ble_gatt_db_char_t * p_chars = p_evt->params.discovered_db.charateristics;

    // Check if the CMDS was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == BLE_UUID_CMDS &&
        p_evt->params.discovered_db.srv_uuid.type == p_cmds_c->uuid_type)
    {
        uint32_t i;
      
          ble_cmds_c_handles_t* hdlr = &p_cmds_c->handles;

        for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            switch (p_chars[i].characteristic.uuid.uuid)
            {
                case BLE_UUID_CMDS_CHAR_HEADER_UUID:
                    hdlr->header_handle         = p_chars[i].characteristic.handle_value;
                    break;

                case BLE_UUID_CMDS_CHAR_DATA_1_UUID:
                    hdlr->data_1_handle           = p_chars[i].characteristic.handle_value;
                    break;

                case BLE_UUID_CMDS_CHAR_DATA_2_UUID:
                    hdlr->data_2_handle           = p_chars[i].characteristic.handle_value;
                    break;

                case BLE_UUID_CMDS_CHAR_RESULT_UUID:    
                    hdlr->result_handle         = p_chars[i].characteristic.handle_value;
                    hdlr->result_cccd_handle    = p_chars[i].cccd_handle;
                    break;
                
                default:
                    break;
            }
        }
        
        LOG_D("HEADER :%02x, DATA_1 :%02x, DATA_2 :%02x\r\n",hdlr->header_handle, hdlr->data_1_handle, hdlr->data_2_handle);
        LOG_D("RESULT :%02x , CCCD : %02x\r\n",hdlr->result_handle, hdlr->result_cccd_handle);
        if(hdlr->header_handle&&hdlr->data_1_handle&&hdlr->data_2_handle&&hdlr->result_handle&&hdlr->result_cccd_handle){
            LOG_D("ALL HANDLER ASSIGNED\r\n");
            hdlr->assigned=true;
            APP.tx_p.process = true;
        }
    }
}
//FOR NOTIFICATION ENABLE
static void on_write_rsp(ble_cmds_c_t * p_cmds_c, const ble_evt_t * p_ble_evt)
{
    if(p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS &&
       p_ble_evt->evt.gattc_evt.params.write_rsp.handle == p_cmds_c->handles.result_cccd_handle)
    {
        p_cmds_c->notification = true;
        LOG_D("PERIPHERAL's NOTIFICATION ENABLED!!\r\n");
    }
}

static void on_hvx(ble_cmds_c_t * p_cmds_c, const ble_evt_t * p_ble_evt)
{
    const ble_gattc_evt_hvx_t * p_evt_hvx = &p_ble_evt->evt.gattc_evt.params.hvx;
    // HVX can only occur from client sending.

    if(p_cmds_c->handles.assigned){
        if (p_evt_hvx->handle == p_cmds_c->handles.result_handle)
        {
            p_cmds_c->state.send = false;
            
            uint8_t  * p_data = (uint8_t *) p_evt_hvx->data;
            if(p_data[0] == CMDS_PKT_RSLT_IDLE){
                p_cmds_c->state.idle = true;
                LOG_D("IDLE OK\r\n");
            }
            else if(p_data[0] == CMDS_PKT_RSLT_HEADER_OK){
                p_cmds_c->state.header = true;
                LOG_D("HEADER OK\r\n");
            }
            else if(p_data[0] == CMDS_PKT_RSLT_DATA_1_OK){
                p_cmds_c->state.data_1 = true;
                LOG_D("DATA_1 OK\r\n");
            }
            else if(p_data[0] == CMDS_PKT_RSLT_DATA_2_OK){
                p_cmds_c->state.data_2 = true;
                LOG_D("DATA_2 OK\r\n");
            }
            else if(p_data[0] == CMDS_PKT_RSLT_INTERPRET_OK){
                p_cmds_c->state.interpret = true;
                LOG_D("INTERPRET OK\r\n");
            }
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
        
        case BLE_GATTC_EVT_WRITE_RSP:
            on_write_rsp(p_cmds_c, p_ble_evt);
            break;
        
        case BLE_GATTC_EVT_HVX:
            on_hvx(p_cmds_c, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            memset(&p_cmds_c->state, 0, sizeof(ble_cmds_c_state_t));
            memset(&p_cmds_c->handles, 0, sizeof(ble_cmds_c_handles_t));
            p_cmds_c->notification = false;
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
    p_cmds_c->handles.data_1_handle = BLE_GATT_HANDLE_INVALID;
    p_cmds_c->handles.data_2_handle = BLE_GATT_HANDLE_INVALID;
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

static uint32_t cmds_c_value_update(ble_cmds_c_t * p_cmds_c, uint16_t* handle, uint8_t * p_string, uint16_t length)
{
    if ( p_cmds_c->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_INVALID_STATE;
    }
    if (length > CMDS_DATA_MAX_LEN)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    const ble_gattc_write_params_t write_params = {
        .write_op = BLE_GATT_OP_WRITE_CMD,
        .flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
        .handle   = *handle,
        .offset   = 0,
        .len      = length,
        .p_value  = p_string
    };

    return sd_ble_gattc_write(p_cmds_c->conn_handle, &write_params);
}


uint32_t ble_cmds_c_notif_enable(ble_cmds_c_t * p_cmds_c, uint16_t * handle)
{
    if ( (p_cmds_c->conn_handle == BLE_CONN_HANDLE_INVALID)
       ||(*handle == BLE_GATT_HANDLE_INVALID)
       )
    {
        return NRF_ERROR_INVALID_STATE;
    }
    return cccd_configure(p_cmds_c->conn_handle,*handle, true);
}
