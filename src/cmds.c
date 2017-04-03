#include "cmds.h"

static void app_disc_id_update(p_packet* rxp)
{
    int8_t result = app_disc_addr_check(rxp->data.p_data);
    if(result >= 0)
    {
        NRF_LOG_INFO("ADDR %s ID %d -> %d !!\r\n",STR_PUSH(app_state.net.disc.peer[result].p_addr.addr,1),app_state.net.disc.peer[result].id,rxp->header.target.node);
        app_state.net.disc.peer[result].id = rxp->header.target.node;
    }
    else{
        NRF_LOG_ERROR("[FATAL]Can't set ID. ADDR NOT FOUND!!");
    }
}

void packet_interpret(ble_cmds_t * p_cmds, ble_evt_t * p_ble_evt)
{
    uint32_t err_code;

    if(app_state.rx_p.process){
        //NRF_LOG_DEBUG("[P3] - PACKET INTERPRET\r\n");
        
        p_packet *rxp = &(app_state.rx_p.packet[app_state.rx_p.process_count]);
       
           
        uint8_t buff1[7];
        memcpy(buff1, &rxp->header,sizeof(buff1));
        
        uint8_t buff2[20];
        memcpy(buff2, &rxp->data,sizeof(buff2));

        NRF_LOG_DEBUG("[%d]th PACKET INTERPRET\r\n",app_state.rx_p.process_count);
        NRF_LOG_DEBUG(" Header : %.14s\r\n", STR_PUSH(buff1,0));
        NRF_LOG_DEBUG(" DATA : %.28s\r\n", STR_PUSH(buff2,0));
        
        switch(rxp->header.type)
        {
            case CMDS_PACKET_TYPE_NETWORK_SCAN_REQUEST:
                if(app_state.dev.my_id == 0)
                {
                    NRF_LOG_DEBUG("Device ID not set!\r\n");

                    if(memcmp(app_state.dev.my_addr.addr,rxp->data.p_data, BLE_GAP_ADDR_LEN))
                    {
                        NRF_LOG_ERROR("SET Device ID FIRST!!\r\n");
                    }
                    else
                    {
                        app_state.dev.my_id = rxp->header.target.node; //ID SETTING
                        NRF_LOG_INFO("Device ID SET : %d !!\r\n",app_state.dev.my_id);
                        
                        NRF_LOG_INFO("[MOD] Network Not Discovered.\r\n");
                        scan_start();
                    }
                }
                else if(app_state.dev.my_id == rxp->header.target.node)
                {
                    app_state.net.discovered = APP_NET_DISCOVERED_FALSE;
                    NRF_LOG_INFO("Network Re-Scan Initialized.\r\n");
                    
                    scan_start();
                }
                else if(app_state.dev.my_id != rxp->header.target.node)
                {
                    NRF_LOG_DEBUG("PACKET ROUTE!\r\n");
                    app_disc_id_update(rxp);
                    
                    if(!app_state.net.discovered){
                        NRF_LOG_ERROR("Network not discovered\r\n");
                        break;
                    }
                    
                    packet_build(CMDS_C_BUILD_PACKET_ROUTE);
                }
                break;
            
            case CMDS_PACKET_TYPE_NETWORK_SCAN_RESPONSE:
                if(!app_state.net.discovered){
                    NRF_LOG_ERROR("Network not discovered\r\n");
                    break;
                }
                
                NRF_LOG_DEBUG("PACKET ROUTE!\r\n");
                packet_build(CMDS_C_BUILD_PACKET_ROUTE);
                break;
            
            default:
                break;
        }
        
        err_code = sd_ble_gap_disconnect(p_cmds->conn_handle,BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        memset(&p_cmds->notification, 0, sizeof(ble_cmds_notification_t));
        app_state.rx_p.process = false;
    }
}
static void packet_count(uint8_t *packet_type)
{
    *packet_type = *packet_type+1;
    NRF_LOG_DEBUG("PACKET COUNT : %d HEADER : %d DATA : %d \r\n",
    app_state.rx_p.packet_count, 
    app_state.rx_p.header_count,
    app_state.rx_p.data_count);

    if(app_state.rx_p.packet_count<*packet_type)
    {
        app_state.rx_p.packet_count++;
    }
    else if(app_state.rx_p.packet_count>=*packet_type)
    {
        NRF_LOG_DEBUG("PACKET INTERPRET START \r\n");
        app_state.rx_p.process = true;
        app_state.rx_p.process_count = *packet_type -1;
    }
}


static void header_parser(ble_gatts_value_t * rx_data)
{
    p_header *pheader = &(app_state.rx_p.packet[app_state.rx_p.header_count].header);
    
    NRF_LOG_DEBUG("Header : %s\r\n",VSTR_PUSH(rx_data->p_value,rx_data->len,0));

    pheader->type = rx_data->p_value[0];
    pheader->index.now = rx_data->p_value[1];
    pheader->index.total = rx_data->p_value[2];
    pheader->source.node = rx_data->p_value[3];
    pheader->source.sensor = rx_data->p_value[4];
    pheader->target.node = rx_data->p_value[5];
    pheader->target.sensor = rx_data->p_value[6];
    NRF_LOG_DEBUG("Header TYPE : %02x\r\n",pheader->type);
    NRF_LOG_DEBUG("Header INDEX : %02x / %02x\r\n",pheader->index.now , pheader->index.total);
    NRF_LOG_DEBUG("Header SOURCE : %02x - %02x\r\n",pheader->source.node , pheader->source.sensor);
    NRF_LOG_DEBUG("Header TARGET : %02x - %02x\r\n",pheader->target.node , pheader->target.sensor);
    
    packet_count(&app_state.rx_p.header_count);
}

static void data_parser(ble_gatts_value_t *rx_data)
{
    uint8_t *pdata = app_state.rx_p.packet[app_state.rx_p.data_count].data.p_data;
    memcpy(pdata, rx_data->p_value, rx_data->len);
    
    NRF_LOG_DEBUG("Data : %s\r\n",VSTR_PUSH(rx_data->p_value,rx_data->len,0));
    
    packet_count(&app_state.rx_p.data_count);
}

static void cmds_notification_enable(ble_cmds_t * p_cmds, bool *notification_type)
{
    *notification_type = true;
    p_cmds->notification.all=p_cmds->notification.header && p_cmds->notification.data && p_cmds->notification.result;
    if(p_cmds->notification.all){
        NRF_LOG_DEBUG("PERIPHERAL NOTIFICATION ALL ENABLED!!\r\n");
    }
}

static void gatts_value_get(ble_cmds_t * p_cmds, uint16_t handle, ble_gatts_value_t* rx_data)
{
    sd_ble_gatts_value_get(p_cmds->conn_handle, handle, rx_data);
    NRF_LOG_INFO("[R] Handle %#06x Value : %s \r\n", handle, VSTR_PUSH(rx_data->p_value,rx_data->len,0));
}

static void cmds_result_update(ble_cmds_t * p_cmds, uint8_t result_type)
{
    uint32_t err_code;
    uint8_t result[CMDS_RESULT_MAX_LEN] = {result_type};

    err_code = cmds_value_update(p_cmds,&p_cmds->result_handles,result, sizeof(result));
    APP_ERROR_CHECK(err_code);
    NRF_LOG_INFO("Result : %s \r\n", STR_PUSH(result,0));
}

static void on_write(ble_cmds_t * p_cmds, ble_evt_t * p_ble_evt)
{
    ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    // Decclare buffer variable to hold received data. The data can only be 32 bit long.
    uint8_t data_buffer[20];
    // Pupulate ble_gatts_value_t structure to hold received data and metadata.
    ble_gatts_value_t rx_data;
    rx_data.len = sizeof(data_buffer);
    rx_data.offset = 0;
    rx_data.p_value = data_buffer;
    
    // Check if write event is performed on our characteristic or the CCCD
    if(p_evt_write->handle == p_cmds->header_handles.value_handle)
    {
        gatts_value_get(p_cmds,p_cmds->header_handles.value_handle,&rx_data);
        header_parser(&rx_data);
        
        cmds_result_update(p_cmds,CMDS_PACKET_RESULT_HEADER_OK);
    }
    else if(p_evt_write->handle == p_cmds->header_handles.cccd_handle)
    {
        gatts_value_get(p_cmds,p_cmds->header_handles.cccd_handle,&rx_data);
        cmds_notification_enable(p_cmds,&p_cmds->notification.header);
    }
    else if(p_evt_write->handle == p_cmds->data_handles.value_handle)
    {
        gatts_value_get(p_cmds,p_cmds->data_handles.value_handle,&rx_data);
        data_parser(&rx_data);

        cmds_result_update(p_cmds,CMDS_PACKET_RESULT_DATA_OK);
    }
    else if(p_evt_write->handle == p_cmds->data_handles.cccd_handle)
    {
        gatts_value_get(p_cmds,p_cmds->data_handles.cccd_handle,&rx_data);
        cmds_notification_enable(p_cmds,&p_cmds->notification.data);
    }
    else if(p_evt_write->handle == p_cmds->result_handles.value_handle)
    {
        gatts_value_get(p_cmds,p_cmds->result_handles.value_handle,&rx_data);
    }
    else if(p_evt_write->handle == p_cmds->result_handles.cccd_handle)
    {
        gatts_value_get(p_cmds,p_cmds->result_handles.cccd_handle,&rx_data);
        cmds_notification_enable(p_cmds,&p_cmds->notification.result);
    }
}

void ble_cmds_on_ble_evt(ble_cmds_t * p_cmds, ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            p_cmds->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            if(p_ble_evt->evt.gap_evt.params.connected.peer_addr.addr[0] != 0){
                app_dev_parent_addr_set(&p_ble_evt->evt.gap_evt.params.connected.peer_addr);
            }
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            p_cmds->conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_cmds, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}


static uint32_t cmd_char_header_add(ble_cmds_t * p_cmds)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          char_uuid;
    ble_gatts_attr_md_t attr_md; 

    char_uuid.type      = p_cmds->uuid_type;
    char_uuid.uuid      = BLE_UUID_CMDS_CHAR_HEADER_UUID;
    
    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read = 1;
    char_md.char_props.write = 1;
    char_md.char_props.notify = 1;
    
    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc       = BLE_GATTS_VLOC_STACK;
    char_md.p_cccd_md           = &cccd_md;
    char_md.char_props.notify   = 1;
    
    memset(&attr_md, 0, sizeof(attr_md));
    attr_md.vloc        = BLE_GATTS_VLOC_STACK;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));    
    attr_char_value.p_uuid      = &char_uuid;
    attr_char_value.p_attr_md   = &attr_md;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len     = CMDS_HEADER_MAX_LEN;
    attr_char_value.init_len = CMDS_HEADER_MAX_LEN;
    
    uint8_t value[CMDS_HEADER_MAX_LEN];
    memset(value, 0, sizeof(value));
    
    attr_char_value.p_value     = value;
    
    return sd_ble_gatts_characteristic_add(p_cmds->service_handle,
                                   &char_md,
                                   &attr_char_value,
                                   &p_cmds->header_handles);
}

static uint32_t cmd_char_data_add(ble_cmds_t * p_cmds)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_gatts_attr_md_t attr_md;
    ble_uuid_t          char_uuid;
    
    char_uuid.type      = p_cmds->uuid_type;
    char_uuid.uuid      = BLE_UUID_CMDS_CHAR_DATA_UUID;
    
    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read = 1;
    char_md.char_props.write = 1;
    char_md.char_props.notify = 1;

    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc       = BLE_GATTS_VLOC_STACK;
    char_md.p_cccd_md           = &cccd_md;
    char_md.char_props.notify   = 1;
    
    memset(&attr_md, 0, sizeof(attr_md));
    attr_md.vloc        = BLE_GATTS_VLOC_STACK;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));    
    attr_char_value.p_uuid      = &char_uuid;
    attr_char_value.p_attr_md   = &attr_md;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len     = CMDS_DATA_MAX_LEN;
    attr_char_value.init_len     = CMDS_DATA_MAX_LEN;

    uint8_t value[CMDS_DATA_MAX_LEN];
    memset(&value, 0, sizeof(value));
    
    attr_char_value.p_value     = value;
    
    
    return sd_ble_gatts_characteristic_add(p_cmds->service_handle,
                                   &char_md,
                                   &attr_char_value,
                                   &p_cmds->data_handles);
}

static uint32_t cmd_char_result_add(ble_cmds_t * p_cmds)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_gatts_attr_md_t attr_md;
    ble_uuid_t          char_uuid;
    
    char_uuid.type      = p_cmds->uuid_type;
    char_uuid.uuid      = BLE_UUID_CMDS_CHAR_RESULT_UUID;
    
    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read = 1;
    char_md.char_props.write = 1;
    char_md.char_props.notify = 1;
    
    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc       = BLE_GATTS_VLOC_STACK;
    char_md.p_cccd_md           = &cccd_md;
    char_md.char_props.notify   = 1;
    
    memset(&attr_md, 0, sizeof(attr_md));
    attr_md.vloc        = BLE_GATTS_VLOC_STACK;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));    
    attr_char_value.p_uuid      = &char_uuid;
    attr_char_value.p_attr_md   = &attr_md;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len     = CMDS_RESULT_MAX_LEN;
    attr_char_value.init_len    = CMDS_RESULT_MAX_LEN;
    uint8_t value[CMDS_RESULT_MAX_LEN] = {CMDS_PACKET_RESULT_IDLE};
    attr_char_value.p_value     = value;
    
    return sd_ble_gatts_characteristic_add(p_cmds->service_handle,
                                   &char_md,
                                   &attr_char_value,
                                   &p_cmds->result_handles);
}

uint32_t cmds_init(ble_cmds_t * p_cmds)
{
    uint32_t   err_code;
    ble_uuid_t        cmds_uuid;
    ble_uuid128_t     base_uuid = BLE_UUID_CMDS_BASE_UUID;

    err_code = sd_ble_uuid_vs_add(&base_uuid, &cmds_uuid.type);
    APP_ERROR_CHECK(err_code);
    
    p_cmds->conn_handle             = BLE_CONN_HANDLE_INVALID;
    p_cmds->notification.all = false;
    p_cmds->uuid_type = cmds_uuid.type;
    
    cmds_uuid.uuid = BLE_UUID_CMDS;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &cmds_uuid,
                                        &p_cmds->service_handle);
    APP_ERROR_CHECK(err_code);

    err_code = cmd_char_header_add(p_cmds);
    APP_ERROR_CHECK(err_code);

    err_code = cmd_char_data_add(p_cmds);
    APP_ERROR_CHECK(err_code);

    err_code = cmd_char_result_add(p_cmds);
    APP_ERROR_CHECK(err_code);
    
    return NRF_SUCCESS;
}


uint32_t cmds_value_update(ble_cmds_t *p_cmds,ble_gatts_char_handles_t* data_handle, uint8_t * p_string, uint16_t length)
{
    if ((p_cmds->conn_handle == BLE_CONN_HANDLE_INVALID) || (!p_cmds->notification.all))
    {
        NRF_LOG_ERROR("Check Noti or Conn State!!\r\n");
        return NRF_ERROR_INVALID_STATE;
    }
    
     if (length > CMDS_MAX_DATA_LEN)
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    
    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = data_handle->value_handle;
    hvx_params.offset = 0;
    hvx_params.p_len  = &length;
    hvx_params.p_data = p_string;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;

    return sd_ble_gatts_hvx(p_cmds->conn_handle, &hvx_params);
}
