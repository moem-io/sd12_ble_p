#include "util_app.h"


#define NRF_LOG_MODULE_NAME "[util]"

uint8_t analyze_data(uint8_t *p_data, uint8_t size) {
    uint8_t end_data[DATA_LEN] = {0x00,};
    int unit_size = MAX_PKT_DATA_LEN /  size;
    int i =0;

    for(i=0; i < unit_size; i++){
        if(!memcmp(&p_data[i*size],end_data,size)){
            break;
        }
    }
    
    return i;
}


//TODO: [BUG] only 128 idx can be retreived. 
int8_t get_addr_idx(uint8_t *p_data){
    for (int8_t i = 0; i < APP.net.node.cnt; i++) {
        if (!memcmp(APP.net.node.peer[i].p_addr.addr, p_data, BLE_GAP_ADDR_LEN)) {
            LOG_D("ADDR FOUND!\r\n");
            return i;
        }
    }
    LOG_D("ADDR NOT FOUND!\r\n");
    return NODE_ADDR_NOT_FOUND;
}


int8_t get_id_idx(uint8_t *id) {
    if (*id == 0) { // MAY NOT BE USED.
        return NODE_ROOT_FOUND;
    }
    for (int i = 0; i < APP.net.node.cnt; i++) {
        if (!memcmp(&APP.net.node.peer[i].id, id, sizeof(uint8_t))) {
//            LOG_D("ID FOUND! idx :%d \r\n",i);
            return i;
        }
    }
    LOG_D("[GET ID] %d NOT FOUND!\r\n",*id);
    return NODE_ID_NOT_FOUND;
}


ble_gap_addr_t *get_node(uint8_t *p_data, bool id, bool addr) {
    int idx = (id) ? get_id_idx(p_data): get_addr_idx(p_data);
    
    if(idx == NODE_ID_NOT_FOUND){
        return NULL;
    } else if(idx == NODE_ROOT_FOUND) {
        return &APP.dev.parent;
    } else {
        return &APP.net.node.peer[idx].p_addr;
    }
}


ble_gap_addr_t *retrieve_send(uint8_t *p_data, bool id, bool addr) {
    gap_data* peer = APP.net.node.peer;
    int idx = (id) ? get_id_idx(p_data): get_addr_idx(p_data);
    
    if(idx == NODE_ID_NOT_FOUND){
        return NULL;
    } else if(idx == NODE_ROOT_FOUND) {
        return &APP.dev.parent;
    } else {
        if (peer[idx].disc){
            return &peer[idx].p_addr;
        } else {
            return retrieve_send(&peer[idx].path[0],1,0);
        }
    }
}


//just for scan response
bool add_node(uint8_t *n_addr, uint8_t *src_id) {
    gap_data* peer = APP.net.node.peer;
    int8_t conn_idx = 0; 

    LOG_D("Node CNT : %d \r\n",APP.net.node.cnt);
    if(get_addr_idx(n_addr) != NODE_ADDR_NOT_FOUND){
        LOG_I("Already Found!");
        return false;
    }
    
    if(!memcmp(n_addr, APP.dev.my_addr.addr, BLE_GAP_ADDR_LEN)){
        int8_t node_idx = get_id_idx(src_id);
        if(node_idx) {
            peer[node_idx].disc = true;
            LOG_I("Found by One-Side. \r\n");
        }
        return false;
    } 
    
    LOG_I("Node ADD!\r\n");
    ble_gap_addr_t new_addr;
    memset(&new_addr,0,sizeof(new_addr));
    new_addr.addr_type = BLE_GAP_ADDR_TYPE_PUBLIC; // TODO: MAYBE this might be wrong.

    memcpy(new_addr.addr,n_addr,BLE_GAP_ADDR_LEN);
    peer[APP.net.node.cnt].p_addr = new_addr;

    conn_idx = get_addr_idx(APP.dev.conn_cen.addr);
    peer[APP.net.node.cnt].path[0]= peer[conn_idx].id;
    
    LOG_D("Addr set : %s, PATH : %s \r\n", STR_PUSH(peer[APP.net.node.cnt].p_addr.addr, 1), STR_PUSH(peer[APP.net.node.cnt].path, 0));
    
    APP.net.node.cnt++;
    return true;
}


void update_node(p_pkt *rxp) {
    gap_data* peer = APP.net.node.peer;
    
    int8_t res = 0;
    switch (rxp->header.type) {
        case PKT_TYPE_NET_SCAN_REQUEST:
            res = get_addr_idx(rxp->data.p_data);
            if (res >= 0) {
                LOG_I("ADDR %s ID %d -> %d !!\r\n", STR_PUSH(peer[res].p_addr.addr, 1),
                      peer[res].id, rxp->header.target.node);
                peer[res].id = rxp->header.target.node;
            } else if(res == NODE_ADDR_NOT_FOUND) {
                LOG_E("UNDISCOVERED REQUEST\r\n");
            }
            break;
        
        case PKT_TYPE_NET_SCAN_RESPONSE:
             res = analyze_data(rxp->data.p_data,7);
            
            for(int i=0; i<res; i++){
                add_node(&rxp->data.p_data[i*7], &rxp->header.source.node);
            }
            break;
    }
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
* @param[in]   adv_report  Pointer to the advertisement report.
*
* @retval      true if the UUID is present in the advertisement report. Otherwise false
*/
bool is_uuid_present(const ble_uuid_t *p_target_uuid, const ble_gap_evt_adv_report_t *adv_report) {
    uint32_t err_code;
    uint32_t index = 0;
    uint8_t *p_data = (uint8_t *) adv_report->data;
    ble_uuid_t extracted_uuid;

    while (index < adv_report->dlen) {
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



static ret_code_t app_fds_write(void) {
    fds_record_t record;
    fds_record_desc_t record_desc;
    fds_record_chunk_t record_chunk;
    // Set up data.
    record_chunk.p_data = &APP;
    record_chunk.length_words = sizeof(APP)/4+1;
    // Set up record.
    record.file_id = APP_FILE_ID;
    record.key = APP_REC_KEY;
    record.data.p_chunks = &record_chunk;
    record.data.num_chunks = 1;

    ret_code_t ret = fds_record_write(&record_desc, &record);
    if (ret != FDS_SUCCESS) {
        return ret;
    }
//    LOG_D("Writing Record ID = %d \r\n", record_desc.record_id);
    return NRF_SUCCESS;
}

ret_code_t app_fds_read(void) {
    fds_flash_record_t flash_record;
    fds_record_desc_t record_desc;
    fds_find_token_t ftok = {0};//Important, make sure you zero init the ftok token
    uint32_t * data;
    uint32_t err_code;
    
    while (fds_record_find(APP_FILE_ID, APP_REC_KEY, &record_desc, &ftok) == FDS_SUCCESS) {
        err_code = fds_record_open(&record_desc, &flash_record);
        if (err_code != FDS_SUCCESS) {
            return err_code;
        }

        LOG_D("Found Record ID = %d. Copying...\r\n", record_desc.record_id);
        data = (uint32_t *) flash_record.p_data;
        memcpy(&APP, data, sizeof(APP));

        LOG_D("[NET : %d] %s Addr : %s\r\n", APP.net.established ,LOG_PUSH(APP.dev.name), STR_PUSH(APP.dev.my_addr.addr, 1));
        // Access the record through the flash_record structure.
        // Close the record when done.
        err_code = fds_record_close(&record_desc);
        if (err_code != FDS_SUCCESS) {
            return err_code;
        }
    }
        
    return NRF_SUCCESS;
}

static ret_code_t app_fds_find_and_delete(void) {
    fds_record_desc_t record_desc;
    fds_find_token_t ftok;

    ftok.page = 0;
    ftok.p_addr = NULL;
    // Loop and find records with same ID and rec key and mark them as deleted.
    while (fds_record_find(APP_FILE_ID, APP_REC_KEY, &record_desc, &ftok) == FDS_SUCCESS) {
        fds_record_delete(&record_desc);
//        LOG_D("Deleted record ID: %d \r\n", record_desc.record_id);
    }
    // call the garbage collector to empty them, don't need to do this all the time, this is just for demonstration
    ret_code_t ret = fds_gc();
    if (ret != FDS_SUCCESS) {
        return ret;
    }
    return NRF_SUCCESS;
}


void app_fds_save(void) {
    uint32_t err_code;

    err_code = app_fds_find_and_delete();
    ERR_CHK("APP FIND & DEL FAILED\r\n");
    err_code =app_fds_write();
    ERR_CHK("APP WRITE FAILED\r\n");
}