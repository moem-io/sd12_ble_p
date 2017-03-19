#include "cmd_svc.h"

void nrf_log_string(int length, uint8_t *data_buffer)
{
    for(int i = 0; i < length; i++){
        NRF_LOG_INFO("%#01x ", data_buffer[i]);
    }
}
static void on_write(ble_cmd_svc_t * p_cmd_svc, ble_evt_t * p_ble_evt)
{
    // Decclare buffer variable to hold received data. The data can only be 32 bit long.
    uint8_t data_buffer[20];
    // Pupulate ble_gatts_value_t structure to hold received data and metadata.
    ble_gatts_value_t rx_data;
    rx_data.len = sizeof(data_buffer);
    rx_data.offset = 0;
    rx_data.p_value = data_buffer;
    
    // Check if write event is performed on our characteristic or the CCCD
    if(p_ble_evt->evt.gatts_evt.params.write.handle == p_cmd_svc->header_handles.value_handle)
    {
        sd_ble_gatts_value_get(p_cmd_svc->conn_handle, p_cmd_svc->header_handles.value_handle, &rx_data);
        NRF_LOG_INFO("Value received on handle %#06x\r\n", p_ble_evt->evt.gatts_evt.params.write.handle);
        nrf_log_string(rx_data.len,data_buffer);
    }
    else if(p_ble_evt->evt.gatts_evt.params.write.handle == p_cmd_svc->header_handles.cccd_handle)
    {
        sd_ble_gatts_value_get(p_cmd_svc->conn_handle, p_cmd_svc->header_handles.cccd_handle, &rx_data);
        NRF_LOG_INFO("Value received on handle %#06x\r\n", p_ble_evt->evt.gatts_evt.params.write.handle);
        nrf_log_string(rx_data.len,data_buffer);
    }
    
    if(p_ble_evt->evt.gatts_evt.params.write.handle == p_cmd_svc->data_handles.value_handle)
    {
        sd_ble_gatts_value_get(p_cmd_svc->conn_handle, p_cmd_svc->data_handles.value_handle, &rx_data);
        NRF_LOG_INFO("Value received on handle %#06x\r\n", p_ble_evt->evt.gatts_evt.params.write.handle);
        nrf_log_string(rx_data.len,data_buffer);
    }
    else if(p_ble_evt->evt.gatts_evt.params.write.handle == p_cmd_svc->data_handles.cccd_handle)
    {
        sd_ble_gatts_value_get(p_cmd_svc->conn_handle, p_cmd_svc->data_handles.cccd_handle, &rx_data);
        NRF_LOG_INFO("Value received on handle %#06x\r\n", p_ble_evt->evt.gatts_evt.params.write.handle);
        nrf_log_string(rx_data.len,data_buffer);
    }
    
    if(p_ble_evt->evt.gatts_evt.params.write.handle == p_cmd_svc->result_handles.value_handle)
    {
        sd_ble_gatts_value_get(p_cmd_svc->conn_handle, p_cmd_svc->result_handles.value_handle, &rx_data);
        NRF_LOG_INFO("Value received on handle %#06x\r\n", p_ble_evt->evt.gatts_evt.params.write.handle);
        nrf_log_string(rx_data.len,data_buffer);
    }
    else if(p_ble_evt->evt.gatts_evt.params.write.handle == p_cmd_svc->result_handles.cccd_handle)
    {
        sd_ble_gatts_value_get(p_cmd_svc->conn_handle, p_cmd_svc->result_handles.cccd_handle, &rx_data);
        NRF_LOG_INFO("Value received on handle %#06x\r\n", p_ble_evt->evt.gatts_evt.params.write.handle);
        nrf_log_string(rx_data.len,data_buffer);
    }
}

void ble_cmd_svc_on_ble_evt(ble_cmd_svc_t * p_cmd_svc, ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            p_cmd_svc->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            p_cmd_svc->conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_cmd_svc, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}


static uint32_t cmd_char_header_add(ble_cmd_svc_t * p_cmd_service)
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
    attr_char_value.max_len     = 20;

    
    return sd_ble_gatts_characteristic_add(p_cmd_service->service_handle,
                                   &char_md,
                                   &attr_char_value,
                                   &p_cmd_service->header_handles);
}

static uint32_t cmd_char_data_add(ble_cmd_svc_t * p_cmd_service)
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
    attr_char_value.max_len     = 20;

    
    return sd_ble_gatts_characteristic_add(p_cmd_service->service_handle,
                                   &char_md,
                                   &attr_char_value,
                                   &p_cmd_service->data_handles);
}

static uint32_t cmd_char_result_add(ble_cmd_svc_t * p_cmd_service)
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
    attr_char_value.max_len     = 1;
    attr_char_value.init_len    = 1;
    uint8_t value[4]            = {0x00};
    attr_char_value.p_value     = value;
    
    return sd_ble_gatts_characteristic_add(p_cmd_service->service_handle,
                                   &char_md,
                                   &attr_char_value,
                                   &p_cmd_service->result_handles);
}

uint32_t cmd_service_init(ble_cmd_svc_t * p_cmd_service)
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


uint32_t cmd_header_char_update(ble_cmd_svc_t *p_cmd_service, uint8_t * p_string, uint16_t length)
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

uint32_t cmd_header_data_update(ble_cmd_svc_t *p_cmd_service, uint8_t * p_string, uint16_t length)
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

uint32_t cmd_header_result_update(ble_cmd_svc_t *p_cmd_service, uint8_t * p_string, uint16_t length)
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
