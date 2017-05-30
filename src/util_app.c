#include "util_app.h"

#define NRF_LOG_MODULE_NAME "[util]"

int8_t app_disc_addr_check(uint8_t *p_data) {
    for (int i = 0; i < APP.net.disc.count; i++) {
        if (!memcmp(APP.net.disc.peer[i].p_addr.addr, p_data, BLE_GAP_ADDR_LEN)) {
            LOG_D("ADDR FOUND!\r\n");
            return i;
        }
    }
    LOG_D("ADDR NOT FOUND!\r\n");
    return GAP_DISC_ADDR_NOT_FOUND;
}


ble_gap_addr_t *app_disc_id_check(uint8_t *id) {
    if (*id == 0) {
        return &APP.dev.parent;
    }
    for (int i = 0; i < APP.net.disc.count; i++) {
        if (!memcmp(&APP.net.disc.peer[i].id, id, sizeof(uint8_t))) {
            LOG_D("ID FOUND!\r\n");
            return &APP.net.disc.peer[i].p_addr;
        }
    }
    LOG_D("ID NOT FOUND!\r\n");
    return GAP_DISC_ID_NOT_FOUND;
}


void app_dev_parent_set(ble_gap_addr_t *addr) {
    if (!APP.dev.parent_set) { //TODO: if parent must be changed?
        memcpy(&APP.dev.parent, addr, sizeof(ble_gap_addr_t));

        LOG_D("Parent Addr set : %s\r\n", STR_PUSH(APP.dev.parent.addr, 1));
        APP.dev.parent_set = true;
    }
}

/**@brief Reads an advertising report and checks if a uuid is present in the service list.
*
* @details The function is able to search for 16-bit, 32-bit and 128-bit service uuids.
*          To see the format of a advertisement pkt, see
*          https://www.bluetooth.org/Technical/AssignedNumbers/generic_access_profile.htm
*
* @param[in]   p_target_uuid The uuid to search fir
* @param[in]   p_adv_report  Pointer to the advertisement report.
*
* @retval      true if the UUID is present in the advertisement report. Otherwise false
*/
bool is_uuid_present(const ble_uuid_t *p_target_uuid, const ble_gap_evt_adv_report_t *p_adv_report) {
    uint32_t err_code;
    uint32_t index = 0;
    uint8_t *p_data = (uint8_t *) p_adv_report->data;
    ble_uuid_t extracted_uuid;

    while (index < p_adv_report->dlen) {
        uint8_t field_length = p_data[index];
        uint8_t field_type = p_data[index + 1];

        if ((field_type == BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE)
            || (field_type == BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE)) {
            for (uint32_t u_index = 0; u_index < (field_length / UUID16_SIZE); u_index++) {
                err_code = sd_ble_uuid_decode(UUID16_SIZE,
                                              &p_data[u_index * UUID16_SIZE + index + 2],
                                              &extracted_uuid);
                if (err_code == NRF_SUCCESS) {
                    if ((extracted_uuid.uuid == p_target_uuid->uuid)
                        && (extracted_uuid.type == p_target_uuid->type)) {
                        return true;
                    }
                }
            }
        } else if ((field_type == BLE_GAP_AD_TYPE_32BIT_SERVICE_UUID_MORE_AVAILABLE)
                   || (field_type == BLE_GAP_AD_TYPE_32BIT_SERVICE_UUID_COMPLETE)) {
            for (uint32_t u_index = 0; u_index < (field_length / UUID32_SIZE); u_index++) {
                err_code = sd_ble_uuid_decode(UUID16_SIZE,
                                              &p_data[u_index * UUID32_SIZE + index + 2],
                                              &extracted_uuid);
                if (err_code == NRF_SUCCESS) {
                    if ((extracted_uuid.uuid == p_target_uuid->uuid)
                        && (extracted_uuid.type == p_target_uuid->type)) {
                        return true;
                    }
                }
            }
        } else if ((field_type == BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE)
                   || (field_type == BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE)) {
            err_code = sd_ble_uuid_decode(UUID128_SIZE,
                                          &p_data[index + 2],
                                          &extracted_uuid);
            if (err_code == NRF_SUCCESS) {
                if ((extracted_uuid.uuid == p_target_uuid->uuid)
                    && (extracted_uuid.type == p_target_uuid->type)) {
                    return true;
                }
            }
        }
        index += field_length + 1;
    }
    return false;
}

