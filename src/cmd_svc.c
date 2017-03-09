#include "cmd_svc.h"


void cmd_service_init(ble_cmd_svc_t * p_cmd_service)
{
    uint32_t   err_code;
    ble_uuid_t        service_uuid;
    ble_uuid128_t     base_uuid = BLE_UUID_CMD_SVC_BASE_UUID;
    
    service_uuid.uuid = BLE_UUID_CMD_SERVICE;
    
    err_code = sd_ble_uuid_vs_add(&base_uuid, &service_uuid.type);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &service_uuid,
                                        &p_cmd_service->service_handle);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("Exectuing our_service_init().\n");
    NRF_LOG_INFO("Service UUID: 0x%#04x\n", service_uuid.uuid); // Print service UUID should match definition BLE_UUID_OUR_SERVICE
    NRF_LOG_INFO("Service UUID type: 0x%#02x\n", service_uuid.type); // Print UUID type. Should match BLE_UUID_TYPE_VENDOR_BEGIN. Search for BLE_UUID_TYPES in ble_types.h for more info
    NRF_LOG_INFO("Service handle: 0x%#04x\n", p_cmd_service->service_handle); // Print out the service handle. Should match service handle shown in MCP under Attribute values
}
