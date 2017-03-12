#include "cmd_svc.h"

static uint32_t cmd_char_header_add(ble_cmd_svc_t * p_cmd_service)
{
    uint32_t   err_code = 0; // Variable to hold return codes from library and softdevice functions
    
    // CMD_JOB: Step 2.A, Add a custom characteristic UUID
    ble_uuid_t          char_uuid;
    ble_uuid128_t       base_uuid = BLE_UUID_CMD_SVC_BASE_UUID;
    char_uuid.uuid      = BLE_UUID_CMD_CHAR_HEADER_UUID;
    
    //TODO : PUT FULL UUID VALUE IN BASE_UUID?
    err_code = sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
    APP_ERROR_CHECK(err_code);
    
    // CMD_JOB: Step 2.F Add read/write properties to our characteristic
    ble_gatts_char_md_t char_md;
    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read = 1;
    char_md.char_props.write = 1;
    
    // CMD_JOB: Step 2.B, Configure the attribute metadata
    ble_gatts_attr_md_t attr_md;
    memset(&attr_md, 0, sizeof(attr_md));
    attr_md.vloc        = BLE_GATTS_VLOC_STACK;
    
    // CMD_JOB: Step 2.G, Set read/write security levels to our characteristic
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    
    // CMD_JOB: Step 2.C, Configure the characteristic value attribute
    ble_gatts_attr_t    attr_char_value;
    memset(&attr_char_value, 0, sizeof(attr_char_value));    
    attr_char_value.p_uuid      = &char_uuid;
    attr_char_value.p_attr_md   = &attr_md;

    // CMD_JOB: Step 2.H, Set characteristic length in number of bytes
    attr_char_value.max_len     = 20;
    attr_char_value.init_len    = 4;
    uint8_t value[4]            = {0x12,0x34,0x56,0x78};
    attr_char_value.p_value     = value;
    
    // CMD_JOB: Step 2.E, Add our new characteristic to the service
    err_code = sd_ble_gatts_characteristic_add(p_cmd_service->service_handle,
                                   &char_md,
                                   &attr_char_value,
                                   &p_cmd_service->char_handles);
    APP_ERROR_CHECK(err_code);
    
    NRF_LOG_INFO("Service handle: %x\r\n", p_cmd_service->service_handle);
    NRF_LOG_INFO("Char value handle: %x\r\n", p_cmd_service->char_handles.value_handle);
    NRF_LOG_INFO("Char cccd handle: %x\r\n", p_cmd_service->char_handles.cccd_handle);

    return NRF_SUCCESS;
}

static uint32_t cmd_char_data_add(ble_cmd_svc_t * p_cmd_service)
{
    uint32_t   err_code = 0; // Variable to hold return codes from library and softdevice functions
    
    // CMD_JOB: Step 2.A, Add a custom characteristic UUID
    ble_uuid_t          char_uuid;
    ble_uuid128_t       base_uuid = BLE_UUID_CMD_SVC_BASE_UUID;
    char_uuid.uuid      = BLE_UUID_CMD_CHAR_DATA_UUID;
    
    //TODO : PUT FULL UUID VALUE IN BASE_UUID?
    err_code = sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
    APP_ERROR_CHECK(err_code);
    
    // CMD_JOB: Step 2.F Add read/write properties to our characteristic
    ble_gatts_char_md_t char_md;
    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read = 1;
    char_md.char_props.write = 1;
    
    // CMD_JOB: Step 2.B, Configure the attribute metadata
    ble_gatts_attr_md_t attr_md;
    memset(&attr_md, 0, sizeof(attr_md));
    attr_md.vloc        = BLE_GATTS_VLOC_STACK;
    
    // CMD_JOB: Step 2.G, Set read/write security levels to our characteristic
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    
    // CMD_JOB: Step 2.C, Configure the characteristic value attribute
    ble_gatts_attr_t    attr_char_value;
    memset(&attr_char_value, 0, sizeof(attr_char_value));    
    attr_char_value.p_uuid      = &char_uuid;
    attr_char_value.p_attr_md   = &attr_md;

    // CMD_JOB: Step 2.H, Set characteristic length in number of bytes
    attr_char_value.max_len     = 20;
    attr_char_value.init_len    = 4;
    uint8_t value[4]            = {0x12,0x34,0x56,0x78};
    attr_char_value.p_value     = value;
    
    // CMD_JOB: Step 2.E, Add our new characteristic to the service
    err_code = sd_ble_gatts_characteristic_add(p_cmd_service->service_handle,
                                   &char_md,
                                   &attr_char_value,
                                   &p_cmd_service->char_handles);
    APP_ERROR_CHECK(err_code);
    
    NRF_LOG_INFO("Service handle: %x\r\n", p_cmd_service->service_handle);
    NRF_LOG_INFO("Char value handle: %x\r\n", p_cmd_service->char_handles.value_handle);
    NRF_LOG_INFO("Char cccd handle: %x\r\n", p_cmd_service->char_handles.cccd_handle);

    return NRF_SUCCESS;
}


static uint32_t cmd_char_result_add(ble_cmd_svc_t * p_cmd_service)
{
    uint32_t   err_code = 0; // Variable to hold return codes from library and softdevice functions
    
    // CMD_JOB: Step 2.A, Add a custom characteristic UUID
    ble_uuid_t          char_uuid;
    ble_uuid128_t       base_uuid = BLE_UUID_CMD_SVC_BASE_UUID;
    char_uuid.uuid      = BLE_UUID_CMD_CHAR_RESULT_UUID;
    
    //TODO : PUT FULL UUID VALUE IN BASE_UUID?
    err_code = sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
    APP_ERROR_CHECK(err_code);
    
    // CMD_JOB: Step 2.F Add read/write properties to our characteristic
    ble_gatts_char_md_t char_md;
    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read = 1;
    char_md.char_props.write = 1;
    
    // CMD_JOB: Step 2.B, Configure the attribute metadata
    ble_gatts_attr_md_t attr_md;
    memset(&attr_md, 0, sizeof(attr_md));
    attr_md.vloc        = BLE_GATTS_VLOC_STACK;
    
    // CMD_JOB: Step 2.G, Set read/write security levels to our characteristic
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    
    // CMD_JOB: Step 2.C, Configure the characteristic value attribute
    ble_gatts_attr_t    attr_char_value;
    memset(&attr_char_value, 0, sizeof(attr_char_value));    
    attr_char_value.p_uuid      = &char_uuid;
    attr_char_value.p_attr_md   = &attr_md;

    // CMD_JOB: Step 2.H, Set characteristic length in number of bytes
    attr_char_value.max_len     = 1;
    attr_char_value.init_len    = 1;
    uint8_t value[4]            = {0x00};
    attr_char_value.p_value     = value;
    
    // CMD_JOB: Step 2.E, Add our new characteristic to the service
    err_code = sd_ble_gatts_characteristic_add(p_cmd_service->service_handle,
                                   &char_md,
                                   &attr_char_value,
                                   &p_cmd_service->char_handles);
    APP_ERROR_CHECK(err_code);
    
    NRF_LOG_INFO("Service handle: %x\r\n", p_cmd_service->service_handle);
    NRF_LOG_INFO("Char value handle: %x\r\n", p_cmd_service->char_handles.value_handle);
    NRF_LOG_INFO("Char cccd handle: %x\r\n", p_cmd_service->char_handles.cccd_handle);

    return NRF_SUCCESS;
}


void cmd_service_init(ble_cmd_svc_t * p_cmd_service)
{
    uint32_t   err_code;
    ble_uuid_t        service_uuid;
    ble_uuid128_t     base_uuid = BLE_UUID_CMD_SVC_BASE_UUID;
    
    service_uuid.uuid = BLE_UUID_CMD_SVC;
    
    err_code = sd_ble_uuid_vs_add(&base_uuid, &service_uuid.type);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &service_uuid,
                                        &p_cmd_service->service_handle);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("Exectuing our_service_init().\r\n");
    NRF_LOG_INFO("Service UUID: 0x%04x\r\n", service_uuid.uuid); // Print service UUID should match definition BLE_UUID_CMD_SVC
    NRF_LOG_INFO("Service handle: 0x%04x\r\n", p_cmd_service->service_handle); // Print out the service handle. Should match service handle shown in MCP under Attribute values
    
    cmd_char_header_add(p_cmd_service);
    cmd_char_data_add(p_cmd_service);
    cmd_char_result_add(p_cmd_service);
}
