#include "per.h"

static void per_result_update(per_t *p_per, uint8_t result_type);

static void app_disc_id_update(p_pkt *rxp) {
    int8_t result = app_disc_addr_check(rxp->data.p_data);
    if (result >= 0) {
        LOG_I("ADDR %s ID %d -> %d !!\r\n", STR_PUSH(APP.net.disc.peer[result].p_addr.addr, 1),
              APP.net.disc.peer[result].id, rxp->header.target.node);
        APP.net.disc.peer[result].id = rxp->header.target.node;
    } else {
        LOG_E("[FATAL]Can't set ID. ADDR NOT FOUND!!");
    }
}

void pkt_interpret(per_t *p_per, ble_evt_t *p_ble_evt) {
    uint32_t err_code;

    if (APP.rx_p.proc) {
        p_pkt *rxp = &(APP.rx_p.pkt[APP.rx_p.proc_cnt]);

        uint8_t buff1[7];
        memcpy(buff1, &rxp->header, sizeof(buff1));

        uint8_t buff2[20];
        memcpy(buff2, &rxp->data, sizeof(buff2));

        LOG_D("[%d]th PACKET INTERPRET\r\n", APP.rx_p.proc_cnt);
        LOG_D(" Header : %.14s\r\n", STR_PUSH(buff1, 0));
        LOG_D(" DATA : %.28s\r\n", STR_PUSH(buff2, 0));

        switch (rxp->header.type) {
            case PKT_TYPE_NET_SCAN_REQUEST:
                if (APP.dev.my_id == 0) {
                    LOG_D("Device ID not set!\r\n");
                    app_dev_parent_set(&APP.dev.connected_central);

                    if (memcmp(APP.dev.my_addr.addr, rxp->data.p_data, BLE_GAP_ADDR_LEN)) {
                        LOG_E("SET Device ID FIRST!!\r\n");
                    } else {
                        APP.dev.my_id = rxp->header.target.node; //ID SETTING
                        LOG_I("Device ID SET : %d !!\r\n", APP.dev.my_id);

                        LOG_I("[MOD] Network Not Discovered.\r\n");
                        scan_start();
                    }
                } else if (APP.dev.my_id == rxp->header.target.node) {
                    APP.net.discovered = APP_NET_DISCOVERED_FALSE;
                    LOG_I("Network Re-Scan Initialized.\r\n");

                    scan_start();
                } else if (APP.dev.my_id != rxp->header.target.node) {
                    LOG_D("PACKET ROUTE!\r\n");
                    app_disc_id_update(rxp);

                    if (!APP.net.discovered) {
                        LOG_E("Network not discovered\r\n");
                        break;
                    }

                    pkt_build(CEN_BUILD_PACKET_ROUTE);
                }
                break;

            case PKT_TYPE_NET_SCAN_RESPONSE:
                if (!APP.net.discovered) {
                    LOG_E("Network not discovered\r\n");
                    break;
                }

                LOG_D("PACKET ROUTE!\r\n");
                pkt_build(CEN_BUILD_PACKET_ROUTE);
                break;

            default:
                break;
        }

        per_result_update(p_per, PKT_RSLT_INTERPRET_OK);
        APP.rx_p.proc = false;
        nrf_delay_ms(100);
        err_code = sd_ble_gap_disconnect(p_per->conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);

    }
}

static void data_cnt_chk(uint8_t *pkt_type, uint8_t cnt) {
    p_header *pheader = &(APP.rx_p.pkt[APP.rx_p.header_cnt - 1].header);
    LOG_D("PACKET HEADER INDEX TOTAL : %d NOW : %d \r\n", pheader->index.total, cnt);

    if (pheader->index.total == cnt) {
        APP.rx_p.data_cnt++;
        APP.rx_p.pkt_cnt++;
        APP.rx_p.proc = true;
    }
}


static void header_parser(ble_gatts_value_t *rx_data) {
    p_header *pheader = &(APP.rx_p.pkt[APP.rx_p.header_cnt].header);

    LOG_D("Header : %s\r\n", VSTR_PUSH(rx_data->p_value, rx_data->len, 0));

    pheader->type = rx_data->p_value[0];
    pheader->index.now = rx_data->p_value[1];
    pheader->index.total = rx_data->p_value[2];
    pheader->source.node = rx_data->p_value[3];
    pheader->source.sensor = rx_data->p_value[4];
    pheader->target.node = rx_data->p_value[5];
    pheader->target.sensor = rx_data->p_value[6];
    LOG_D("Header TYPE : %02x\r\n", pheader->type);
    LOG_D("Header INDEX : %02x / %02x\r\n", pheader->index.now, pheader->index.total);
    LOG_D("Header SOURCE : %02x - %02x\r\n", pheader->source.node, pheader->source.sensor);
    LOG_D("Header TARGET : %02x - %02x\r\n", pheader->target.node, pheader->target.sensor);

    APP.rx_p.header_cnt++;
}

static void data_1_parser(ble_gatts_value_t *rx_data) {
    uint8_t *pdata = APP.rx_p.pkt[APP.rx_p.data_cnt].data.p_data;
    memcpy(&pdata[0], rx_data->p_value, rx_data->len);

    LOG_D("Data 1: %s\r\n", VSTR_PUSH(pdata, 20, 0));

    data_cnt_chk(&APP.rx_p.data_cnt, 1);
}


static void data_2_parser(ble_gatts_value_t *rx_data) {
    uint8_t *pdata = APP.rx_p.pkt[APP.rx_p.data_cnt].data.p_data;
    memcpy(&pdata[20], rx_data->p_value, rx_data->len);

    LOG_D("Data 2: %s\r\n", VSTR_PUSH(pdata, 40, 0));

    data_cnt_chk(&APP.rx_p.data_cnt, 2);
}


static void gatts_value_get(per_t *p_per, uint16_t handle, ble_gatts_value_t *rx_data) {
    sd_ble_gatts_value_get(p_per->conn_handle, handle, rx_data);
    LOG_I("[R] Handle %#06x Value : %s \r\n", handle, VSTR_PUSH(rx_data->p_value, rx_data->len, 0));
}

static void per_result_update(per_t *p_per, uint8_t result_type) {
    uint32_t err_code;
    uint8_t result[MAX_RESULT_LEN] = {result_type};

    err_code = per_value_update(p_per, &p_per->result_hdlrs, result, sizeof(result));
    APP_ERROR_CHECK(err_code);
    LOG_I("Result : %s \r\n", STR_PUSH(result, 0));
}

static void on_write(per_t *p_per, ble_evt_t *p_ble_evt) {
    ble_gatts_evt_write_t *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    // Decclare buffer variable to hold received data. The data can only be 32 bit long.
    uint8_t data_buffer[20];
    // Pupulate ble_gatts_value_t structure to hold received data and metadata.
    ble_gatts_value_t rx_data;
    rx_data.len = sizeof(data_buffer);
    rx_data.offset = 0;
    rx_data.p_value = data_buffer;

    // Check if write event is performed on our characteristic or the CCCD
    if (p_evt_write->handle == p_per->header_hdlrs.value_handle) {
        gatts_value_get(p_per, p_per->header_hdlrs.value_handle, &rx_data);
        header_parser(&rx_data);

        per_result_update(p_per, PKT_RSLT_HEADER_OK);
    } else if (p_evt_write->handle == p_per->data_1_hdlrs.value_handle) {
        gatts_value_get(p_per, p_per->data_1_hdlrs.value_handle, &rx_data);
        data_1_parser(&rx_data);

        per_result_update(p_per, PKT_RSLT_DATA_1_OK);
    } else if (p_evt_write->handle == p_per->data_2_hdlrs.value_handle) {
        gatts_value_get(p_per, p_per->data_2_hdlrs.value_handle, &rx_data);
        data_2_parser(&rx_data);

        per_result_update(p_per, PKT_RSLT_DATA_2_OK);
    } else if (p_evt_write->handle == p_per->result_hdlrs.value_handle) {
        gatts_value_get(p_per, p_per->result_hdlrs.value_handle, &rx_data);
    } else if (p_evt_write->handle == p_per->result_hdlrs.cccd_handle) {
        gatts_value_get(p_per, p_per->result_hdlrs.cccd_handle, &rx_data);
        p_per->notification = true;
        LOG_D("NOTIFICATION ENABLED BY CENTRAL!!\r\n");

        per_result_update(p_per, PKT_RSLT_IDLE);
    }
}

void per_on_ble_evt(per_t *p_per, ble_evt_t *p_ble_evt) {
    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            p_per->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            memcpy(&APP.dev.connected_central, &p_ble_evt->evt.gap_evt.params.connected.peer_addr,
                   sizeof(ble_gap_addr_t));
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            memset(&APP.dev.connected_central, 0, sizeof(ble_gap_addr_t));
            p_per->notification = false;
            p_per->conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_per, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}


static uint32_t per_char_add(per_t *p_per,int uuid, int len, ble_gatts_char_handles_t *hdlr) {
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t attr_char_value;
    ble_gatts_attr_md_t attr_md;
    ble_uuid_t char_uuid;

    char_uuid.type = p_per->uuid_type;
    char_uuid.uuid = uuid;

    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read = 1;
    char_md.char_props.write = 1;

    if (uuid == CMDS_RESULT_UUID) {
        ble_gatts_attr_md_t cccd_md;

        memset(&cccd_md, 0, sizeof(cccd_md));
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
        cccd_md.vloc = BLE_GATTS_VLOC_STACK;
        char_md.p_cccd_md = &cccd_md;
        char_md.char_props.notify = 1;
    }
    
    memset(&attr_md, 0, sizeof(attr_md));
    attr_md.vloc = BLE_GATTS_VLOC_STACK;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    memset(&attr_char_value, 0, sizeof(attr_char_value));
    attr_char_value.p_uuid = &char_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len = len;
    attr_char_value.init_len = len;

    uint8_t value[len];
    memset(&value, 0, sizeof(value));

    attr_char_value.p_value = value;

    return sd_ble_gatts_characteristic_add(p_per->service_handle, &char_md, &attr_char_value, hdlr);
}


uint32_t per_init(per_t *p_per) {
    uint32_t err_code;
    ble_uuid_t per_uuid;
    ble_uuid128_t base_uuid = CMDS_BASE_UUID;

    err_code = sd_ble_uuid_vs_add(&base_uuid, &per_uuid.type);
    APP_ERROR_CHECK(err_code);

    p_per->conn_handle = BLE_CONN_HANDLE_INVALID;
    p_per->uuid_type = per_uuid.type;

    per_uuid.uuid = CMDS_UUID;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &per_uuid,
                                        &p_per->service_handle);
    APP_ERROR_CHECK(err_code);

    err_code = per_char_add(p_per,CMDS_HEADER_UUID,MAX_HEADER_LEN,&p_per->header_hdlrs);
    APP_ERROR_CHECK(err_code);
    
    err_code = per_char_add(p_per,CMDS_DATA_1_UUID,MAX_DATA_LEN,&p_per->data_1_hdlrs);
    APP_ERROR_CHECK(err_code);

    err_code = per_char_add(p_per,CMDS_DATA_2_UUID,MAX_DATA_LEN,&p_per->data_2_hdlrs);
    APP_ERROR_CHECK(err_code);
    
    err_code = per_char_add(p_per, CMDS_RESULT_UUID, MAX_RESULT_LEN, &p_per->result_hdlrs);
    APP_ERROR_CHECK(err_code);

    return NRF_SUCCESS;
}


uint32_t per_value_update(per_t *p_per, ble_gatts_char_handles_t *data_handle, uint8_t *p_string, uint16_t length) {
    if ((p_per->conn_handle == BLE_CONN_HANDLE_INVALID) || (!p_per->notification)) {
        LOG_E("Check Noti or Conn State!!\r\n");
        return NRF_ERROR_INVALID_STATE;
    }

    if (length > MAX_CHAR_LEN) {
        return NRF_ERROR_INVALID_PARAM;
    }

    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = data_handle->value_handle;
    hvx_params.offset = 0;
    hvx_params.p_len = &length;
    hvx_params.p_data = p_string;
    hvx_params.type = BLE_GATT_HVX_NOTIFICATION;

    return sd_ble_gatts_hvx(p_per->conn_handle, &hvx_params);
}
