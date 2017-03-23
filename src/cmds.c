#include "cmds.h"

int8_t gap_disc_addr_check(uint8_t *p_data){
    for(int i=0;i<app_state.net.disc.count;i++){
        if(!memcmp(app_state.net.disc.data[i].peer_addr.addr,p_data, BLE_GAP_ADDR_LEN)){
            NRF_LOG_DEBUG("ADDR FOUND!\r\n");
            return i;
        }
    }
    NRF_LOG_DEBUG("ADDR NOT FOUND!\r\n");
    return GAP_DISC_ADDR_NOT_FOUND;
}

void packet_route()
{
}
 
void packet_interpret(uint8_t packet_no)
{
    uint32_t              err_code;
    NRF_LOG_DEBUG("PACKET INTERPRET!\r\n");

    p_packet *ppacket = &(app_state.packet.packet[packet_no]);
   
    switch(ppacket->header.type)
    {
        case BLE_CMD_SVC_PACKET_TYPE_NETWORK_SCAN_REQUEST:
            if(app_state.dev.id == 0)
            {
                NRF_LOG_DEBUG("Device ID not set!\r\n");

                if(memcmp(app_state.dev.p_addr.addr,ppacket->data.p_data, BLE_GAP_ADDR_LEN))
                {
                NRF_LOG_ERROR("SET Device ID FIRST!!\r\n");
                }
                else
                {
                app_state.dev.id = ppacket->header.target.node; //ID SETTING
                NRF_LOG_INFO("Device ID SET : %d !!\r\n",app_state.dev.id);
                    if(app_state.net.established == APP_NET_ESTABLISHED_FALSE)
                    {
                        NRF_LOG_INFO("Network Not Established.\r\n");
                        scan_start(); //NET SCAN INIT
                    }
                }
            }
            else if(app_state.dev.id != ppacket->header.target.node)
            {
                NRF_LOG_DEBUG("PACKET ROUTE!\r\n");

              int8_t result = gap_disc_addr_check(ppacket->data.p_data);
                if(result != GAP_DISC_ADDR_NOT_FOUND){
                    err_code = sd_ble_gap_connect(&app_state.net.disc.data[result].peer_addr,
                              &m_scan_params,
                              &m_connection_param);
                    APP_ERROR_CHECK(err_code);
                  
                    NRF_LOG_DEBUG("CONNECTED TO PERIPHERAL!\r\n");  
                }
                
            }
            else if(app_state.dev.id == ppacket->header.target.node)
            {
              app_state.net.established = APP_NET_ESTABLISHED_FALSE;
              NRF_LOG_INFO("Network Re-Scan Initialized.\r\n");
              scan_start();
            }
            break;
        
        case BLE_CMD_SVC_PACKET_TYPE_NETWORK_SCAN_RESPONSE: 
            break;
        
        default:
            break;
    }

}
void packet_count(uint8_t *packet_type)
{
    *packet_type = *packet_type+1;
    NRF_LOG_DEBUG("PACKET COUNT : %d HEADER : %d DATA : %d \r\n",
    app_state.packet.packet_count, 
    app_state.packet.header_count,
    app_state.packet.data_count);

    if(app_state.packet.packet_count<*packet_type)
    {
        app_state.packet.packet_count++;
    }
    else if(app_state.packet.packet_count>=*packet_type)
    {
        packet_interpret(*packet_type -1);
    }
}


void header_parser(uint8_t * data_buffer)
{
    p_header *pheader = &(app_state.packet.packet[app_state.packet.header_count].header);
    
    NRF_LOG_DEBUG("Header : %02x%02x%02x%02x%02x%02x\n",
    data_buffer[0],data_buffer[1],data_buffer[2],data_buffer[3],data_buffer[4],data_buffer[5]);

    pheader->type = data_buffer[0];
    pheader->index.now = data_buffer[1];
    pheader->index.total = data_buffer[2];
    pheader->source.node = data_buffer[3];
    pheader->source.sensor = data_buffer[4];
    pheader->target.node = data_buffer[5];
    pheader->target.sensor = data_buffer[6];
    NRF_LOG_DEBUG("Header received\r\n");
    NRF_LOG_DEBUG("Header TYPE : %02x\r\n",pheader->type);
    NRF_LOG_DEBUG("Header INDEX : %02x / %02x\r\n",pheader->index.now , pheader->index.total);
    NRF_LOG_DEBUG("Header SOURCE : %02x - %02x\r\n",pheader->source.node , pheader->source.sensor);
    NRF_LOG_DEBUG("Header TARGET : %02x - %02x\r\n",pheader->target.node , pheader->target.sensor);
    
    packet_count(&app_state.packet.header_count);

}

void data_parser(uint8_t *data_buffer){
    NRF_LOG_DEBUG("Data received\r\n");
    uint8_t *pdata = app_state.packet.packet[app_state.packet.data_count].data.p_data;

    memcpy(pdata, data_buffer, sizeof(uint8_t)*20);

    for(int i=4;i>0;i--){
        NRF_LOG_DEBUG("Data : %02x%02x%02x%02x%02x\n",
        pdata[5*i],pdata[5*i-1],pdata[5*i-2],pdata[5*i-3],pdata[5*i-4]);
    }
    
    packet_count(&app_state.packet.data_count);
}


void nrf_log_string(int length, uint8_t *data_buffer)
{
    for(int i = 0; i < length; i++){
        NRF_LOG_INFO("%02x ", data_buffer[i]);
    }
}
static void on_write(ble_cmds_t * p_cmds, ble_evt_t * p_ble_evt)
{
    // Decclare buffer variable to hold received data. The data can only be 32 bit long.
    uint8_t data_buffer[20];
    // Pupulate ble_gatts_value_t structure to hold received data and metadata.
    ble_gatts_value_t rx_data;
    rx_data.len = sizeof(data_buffer);
    rx_data.offset = 0;
    rx_data.p_value = data_buffer;
    
    // Check if write event is performed on our characteristic or the CCCD
    if(p_ble_evt->evt.gatts_evt.params.write.handle == p_cmds->header_handles.value_handle)
    {
        sd_ble_gatts_value_get(p_cmds->conn_handle, p_cmds->header_handles.value_handle, &rx_data);
        NRF_LOG_INFO("Value received on handle %#06x\r\n", p_ble_evt->evt.gatts_evt.params.write.handle);
        nrf_log_string(rx_data.len,data_buffer);
        
        header_parser(data_buffer);
    }
    else if(p_ble_evt->evt.gatts_evt.params.write.handle == p_cmds->header_handles.cccd_handle)
    {
        sd_ble_gatts_value_get(p_cmds->conn_handle, p_cmds->header_handles.cccd_handle, &rx_data);
        NRF_LOG_INFO("Value received on handle %#06x\r\n", p_ble_evt->evt.gatts_evt.params.write.handle);
        nrf_log_string(rx_data.len,data_buffer);
    }
    
    if(p_ble_evt->evt.gatts_evt.params.write.handle == p_cmds->data_handles.value_handle)
    {
        sd_ble_gatts_value_get(p_cmds->conn_handle, p_cmds->data_handles.value_handle, &rx_data);
        NRF_LOG_INFO("Value received on handle %#06x\r\n", p_ble_evt->evt.gatts_evt.params.write.handle);
        nrf_log_string(rx_data.len,data_buffer);
        data_parser(data_buffer);
    }
    else if(p_ble_evt->evt.gatts_evt.params.write.handle == p_cmds->data_handles.cccd_handle)
    {
        sd_ble_gatts_value_get(p_cmds->conn_handle, p_cmds->data_handles.cccd_handle, &rx_data);
        NRF_LOG_INFO("Value received on handle %#06x\r\n", p_ble_evt->evt.gatts_evt.params.write.handle);
        nrf_log_string(rx_data.len,data_buffer);
    }
    
    if(p_ble_evt->evt.gatts_evt.params.write.handle == p_cmds->result_handles.value_handle)
    {
        sd_ble_gatts_value_get(p_cmds->conn_handle, p_cmds->result_handles.value_handle, &rx_data);
        NRF_LOG_INFO("Value received on handle %#06x\r\n", p_ble_evt->evt.gatts_evt.params.write.handle);
        nrf_log_string(rx_data.len,data_buffer);
    }
    else if(p_ble_evt->evt.gatts_evt.params.write.handle == p_cmds->result_handles.cccd_handle)
    {
        sd_ble_gatts_value_get(p_cmds->conn_handle, p_cmds->result_handles.cccd_handle, &rx_data);
        NRF_LOG_INFO("Value received on handle %#06x\r\n", p_ble_evt->evt.gatts_evt.params.write.handle);
        nrf_log_string(rx_data.len,data_buffer);
    }
}

void ble_cmds_on_ble_evt(ble_cmds_t * p_cmds, ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            p_cmds->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
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


static uint32_t cmd_char_header_add(ble_cmds_t * p_cmd_service)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          char_uuid;
    ble_gatts_attr_md_t attr_md; 

    char_uuid.type      = p_cmd_service->uuid_type;
    char_uuid.uuid      = BLE_UUID_CMD_CHAR_HEADER_UUID;
    
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
    attr_char_value.max_len     = 7;
    
    uint8_t value[attr_char_value.max_len];
    memset(&value, 0, sizeof(value));
    
    attr_char_value.p_value     = value;
    
    return sd_ble_gatts_characteristic_add(p_cmd_service->service_handle,
                                   &char_md,
                                   &attr_char_value,
                                   &p_cmd_service->header_handles);
}

static uint32_t cmd_char_data_add(ble_cmds_t * p_cmd_service)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_gatts_attr_md_t attr_md;
    ble_uuid_t          char_uuid;
    
    char_uuid.type      = p_cmd_service->uuid_type;
    char_uuid.uuid      = BLE_UUID_CMD_CHAR_DATA_UUID;
    
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
    attr_char_value.max_len     = 20;

    uint8_t value[attr_char_value.max_len];
    memset(&value, 0, sizeof(value));
    
    attr_char_value.p_value     = value;
    
    
    return sd_ble_gatts_characteristic_add(p_cmd_service->service_handle,
                                   &char_md,
                                   &attr_char_value,
                                   &p_cmd_service->data_handles);
}

static uint32_t cmd_char_result_add(ble_cmds_t * p_cmd_service)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_gatts_attr_md_t attr_md;
    ble_uuid_t          char_uuid;
    
    char_uuid.type      = p_cmd_service->uuid_type;
    char_uuid.uuid      = BLE_UUID_CMD_CHAR_RESULT_UUID;
    
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
    attr_char_value.max_len     = 1;
    attr_char_value.init_len    = 1;
    uint8_t value[4]            = {0x00};
    attr_char_value.p_value     = value;
    
    return sd_ble_gatts_characteristic_add(p_cmd_service->service_handle,
                                   &char_md,
                                   &attr_char_value,
                                   &p_cmd_service->result_handles);
}

uint32_t cmd_service_init(ble_cmds_t * p_cmd_service)
{
    uint32_t   err_code;
    ble_uuid_t        service_uuid;
    ble_uuid128_t     base_uuid = BLE_UUID_CMD_SVC_BASE_UUID;

    
    err_code = sd_ble_uuid_vs_add(&base_uuid, &service_uuid.type);
    APP_ERROR_CHECK(err_code);
    
    p_cmd_service->conn_handle             = BLE_CONN_HANDLE_INVALID;
    p_cmd_service->is_notification_enabled = false;
    p_cmd_service->uuid_type = service_uuid.type;
    
    service_uuid.uuid = BLE_UUID_CMD_SVC;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &service_uuid,
                                        &p_cmd_service->service_handle);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("Service UUID: 0x%04x\r\n", service_uuid.uuid); // Print service UUID should match definition BLE_UUID_CMD_SVC
    NRF_LOG_INFO("Service handle: 0x%04x\r\n", p_cmd_service->service_handle); // Print out the service handle. Should match service handle shown in MCP under Attribute values
    
    err_code = cmd_char_header_add(p_cmd_service);
    APP_ERROR_CHECK(err_code);

    err_code = cmd_char_data_add(p_cmd_service);
    APP_ERROR_CHECK(err_code);

    err_code = cmd_char_result_add(p_cmd_service);
    APP_ERROR_CHECK(err_code);
    
    return NRF_SUCCESS;
}


uint32_t cmd_header_char_update(ble_cmds_t *p_cmd_service, uint8_t * p_string, uint16_t length)
{
    if ((p_cmd_service->conn_handle == BLE_CONN_HANDLE_INVALID) || (!p_cmd_service->is_notification_enabled))
    {
        return NRF_ERROR_INVALID_STATE;
    }
    
     if (length > BLE_CMD_SVC_MAX_DATA_LEN)
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    
    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = p_cmd_service->header_handles.value_handle;
    hvx_params.offset = 0;
    hvx_params.p_len  = &length;
    hvx_params.p_data = p_string;  
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;

    return sd_ble_gatts_hvx(p_cmd_service->conn_handle, &hvx_params);
}

uint32_t cmd_header_data_update(ble_cmds_t *p_cmd_service, uint8_t * p_string, uint16_t length)
{
    if ((p_cmd_service->conn_handle == BLE_CONN_HANDLE_INVALID) || (!p_cmd_service->is_notification_enabled))
    {
        return NRF_ERROR_INVALID_STATE;
    }
    
     if (length > BLE_CMD_SVC_MAX_DATA_LEN)
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    
    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = p_cmd_service->data_handles.value_handle;
    hvx_params.offset = 0;
    hvx_params.p_len  = &length;
    hvx_params.p_data = p_string;  
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;

    return sd_ble_gatts_hvx(p_cmd_service->conn_handle, &hvx_params);
}

uint32_t cmd_header_result_update(ble_cmds_t *p_cmd_service, uint8_t * p_string, uint16_t length)
{
    if ((p_cmd_service->conn_handle == BLE_CONN_HANDLE_INVALID) || (!p_cmd_service->is_notification_enabled))
    {
        return NRF_ERROR_INVALID_STATE;
    }
    
     if (length > BLE_CMD_SVC_RESULT_CHAR_MAX_DATA_LEN)
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    
    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = p_cmd_service->result_handles.value_handle;
    hvx_params.offset = 0;
    hvx_params.p_len  = &length;
    hvx_params.p_data = p_string;  
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;

    return sd_ble_gatts_hvx(p_cmd_service->conn_handle, &hvx_params);
}
