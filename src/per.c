#include "per.h"


#define NRF_LOG_MODULE_NAME "[per]"

extern uint8_t tmp_addr[BLE_GAP_ADDR_LEN];

extern volatile bool flagLED;
extern volatile bool tgt_scan;

static void  per_value_reset(per_t *p_per);
static uint32_t per_char_reset(per_t *p_per, ble_gatts_char_handles_t *data_handle, uint8_t *p_string, uint16_t length);
static void per_result_update(per_t *p_per, uint8_t result_type);


void pkt_interpret(per_t *p_per) {
    if (PKT.rx_p.proc) {
        p_pkt *rxp = &(PKT.rx_p.pkt[PKT.rx_p.proc_cnt]);

        uint8_t buff1[HEADER_LEN];
        memcpy(buff1, &rxp->header, sizeof(buff1));

        uint8_t buff2[MAX_PKT_DATA_LEN];
        memcpy(buff2, &rxp->data, sizeof(buff2));

        LOG_D("[%d]th PACKET INTERPRET\r\n", PKT.rx_p.proc_cnt);
        LOG_D(" Header : %.24s\r\n", STR_PUSH(buff1, 0));
        LOG_D(" DATA : %.28s\r\n", STR_PUSH(buff2, 0));
        
        update_node(rxp);
        LOG_D("PACKET %d!\r\n",rxp->header.target.node);
        
        if(APP.dev.my_id != 0 && APP.dev.my_id != rxp->header.target.node) {
            LOG_D("PACKET ROUTE!\r\n");
            if (!APP.net.discovered) {
                LOG_E("Network not discovered\r\n");
                return;
            }
            pkt_build(CEN_BUILD_PACKET_ROUTE,0);
        } else {
            app_dev_parent_set(&APP.dev.conn_cen);

            switch (rxp->header.type) {
                case PKT_TYPE_NET_SCAN_REQ:{
                    if (APP.dev.my_id == 0) {
                        if (memcmp(APP.dev.my_addr.addr, rxp->data.p_data, BLE_GAP_ADDR_LEN)) {
                            LOG_E("WRONG Device ADDR!!\r\n");
                            break;
                        } else {
                            APP.dev.my_id = rxp->header.target.node; //ID SETTING
                            LOG_I("Device ID SET : %d !!\r\n", APP.dev.my_id);
                        }
                    } else if (APP.dev.my_id == rxp->header.target.node) {
                        APP.net.discovered = APP_NET_DISCOVERED_FALSE;
                        LOG_I("Network Re-Scan Initialized.\r\n");
                    }
                    scan_start();
                }break;
                    
                case PKT_TYPE_SNSR_STATE_RES:{
                    
                }break;
                    
                case PKT_TYPE_SNSR_DATA_REQ:{
                }break;

                case PKT_TYPE_SNSR_ACT_RES:{
                }break;
                case PKT_TYPE_SNSR_CMD_REQ:{
                }break;
                case PKT_TYPE_NODE_STAT_REQ:{
                }break;                   
                case PKT_TYPE_NODE_LED_REQ:{
                    LOG_D("LED Request \r\n");
                    flagLED = true; // PKT_BUILD -> MAIN LOOP
                    pkt_build(PKT_TYPE_NODE_LED_RES,0);
                }break;
                case PKT_TYPE_NODE_BTN_PRESS_RES:{
                    LOG_D("OK\r\n");
                } break;
                case PKT_TYPE_NET_PATH_UPDATE_REQ:{
                    if (!APP.net.discovered) {
                        LOG_E("Network not discovered\r\n");
                        break;
                    }
    ///////////////////////////////////////////////////////////
                    LOG_D("PATH UPDATING!\r\n");
                    pkt_build(PKT_TYPE_NET_PATH_UPDATE_RES,0);
                }break;

                case PKT_TYPE_NET_ACK_REQ:{
                    if (!APP.net.discovered) {
                        LOG_E("Network not discovered\r\n");
                        break;
                    }
    ///////////////////////////////////////////////////////////
                    LOG_D("ACK REQUEST!\r\n");
                    pkt_build(PKT_TYPE_NET_ACK_RES,0);
                }break;
                
                case PKT_TYPE_NET_JOIN_RES:{
                }break;
                case PKT_TYPE_SCAN_TGT_REQ:{
                    memcpy(tmp_addr, rxp->data.p_data, BLE_GAP_ADDR_LEN);
                    tgt_scan = true;
                    scan_start();
                }break;
                default:
                    break;
            }
        }
        app_fds_save();
        per_value_reset(p_per);
        per_result_update(p_per, PKT_RSLT_INTERPRET_OK);
        PKT.rx_p.proc_cnt++;
        PKT.rx_p.proc = false;
        
        nrf_delay_ms(100);
        
    }
}

static void data_cnt_chk(uint8_t *pkt_type, uint8_t cnt) {
    p_header *pheader = &(PKT.rx_p.pkt[PKT.rx_p.header_cnt - 1].header);
    LOG_D("PACKET DATA CHAR TOTAL : %d NOW : %d \r\n", pheader->idx_tot, cnt);

    if (pheader->idx_tot == cnt) {
        PKT.rx_p.data_cnt++;
        PKT.rx_p.pkt_cnt++;
        PKT.rx_p.proc = true;
    }
}


static void header_parser(ble_gatts_value_t *rx_data) {
    p_header *pheader = &(PKT.rx_p.pkt[PKT.rx_p.header_cnt].header);
  
    pheader->type = rx_data->p_value[0];
    pheader->err_type = rx_data->p_value[1];
    pheader->idx_tot = rx_data->p_value[2];
    pheader->source.node = rx_data->p_value[3];
    pheader->source.sensor = rx_data->p_value[4];
    pheader->target.node = rx_data->p_value[5];
    pheader->target.sensor = rx_data->p_value[6];
    memcpy(&pheader->path,&rx_data->p_value[7],MAX_DEPTH_CNT);
//    LOG_D("Header TYPE : %02x INDEX : %02x / %02x\r\n", pheader->type, pheader->err_type, pheader->idx_tot);
//    LOG_D("Header SOURCE : %02x - %02x\r\n", pheader->source.node, pheader->source.sensor);
//    LOG_D("Header TARGET : %02x - %02x\r\n", pheader->target.node, pheader->target.sensor);

    PKT.rx_p.header_cnt++;
}

static void data_1_parser(ble_gatts_value_t *rx_data) {
    uint8_t *pdata = PKT.rx_p.pkt[PKT.rx_p.data_cnt].data.p_data;
    memcpy(&pdata[0], rx_data->p_value, rx_data->len);

//    LOG_D("Data 1: %s\r\n", VSTR_PUSH(pdata, 20, 0));

    data_cnt_chk(&PKT.rx_p.data_cnt, 1);
}


static void data_2_parser(ble_gatts_value_t *rx_data) {
    uint8_t *pdata = PKT.rx_p.pkt[PKT.rx_p.data_cnt].data.p_data;
    memcpy(&pdata[20], rx_data->p_value, rx_data->len);

//    LOG_D("Data 2: %s\r\n", VSTR_PUSH(pdata, 40, 0));

    data_cnt_chk(&PKT.rx_p.data_cnt, 2);
}


static void gatts_value_get(per_t *p_per, uint16_t handle, ble_gatts_value_t *rx_data) {
    sd_ble_gatts_value_get(p_per->conn_handle, handle, rx_data);
    LOG_I("[rxP %d]th Handle %#06x Value : %s \r\n", PKT.rx_p.data_cnt, handle, VSTR_PUSH(rx_data->p_value, rx_data->len, 0));
}


static void per_value_reset(per_t *p_per){
    uint32_t err_code;
    
    uint8_t data[DATA_LEN] = {0,};
    err_code = per_char_reset(p_per, &p_per->data_1_hdlrs, data, sizeof(data));
    ERR_CHK("Data 1 Char reset");

    err_code = per_char_reset(p_per, &p_per->data_2_hdlrs, data, sizeof(data));
    ERR_CHK("Data 2 Char reset");
}


static void per_result_update(per_t *p_per, uint8_t result_type) {
    uint32_t err_code;
    uint8_t result[RESULT_LEN] = {result_type};

    err_code = per_char_update(p_per, &p_per->result_hdlrs, result, sizeof(result));
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
//        LOG_D("NOTIFICATION ENABLED BY CENTRAL!!\r\n");

        per_result_update(p_per, PKT_RSLT_IDLE);
    }
}

void app_per_evt(per_t *p_per, ble_evt_t *p_ble_evt) {
    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            p_per->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            memcpy(&APP.dev.conn_cen, &p_ble_evt->evt.gap_evt.params.connected.peer_addr, sizeof(ble_gap_addr_t));
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            LOG_D("RESETTING Peripheral\r\n");
            memset(&APP.dev.conn_cen, 0, sizeof(ble_gap_addr_t));
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

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &per_uuid, &p_per->service_handle);
    ERR_CHK("Service init");

    err_code = per_char_add(p_per,CMDS_HEADER_UUID,HEADER_LEN,&p_per->header_hdlrs);
    ERR_CHK("Header Char init");
    
    err_code = per_char_add(p_per,CMDS_DATA_1_UUID,DATA_LEN,&p_per->data_1_hdlrs);
    ERR_CHK("Data 1 Char init");

    err_code = per_char_add(p_per,CMDS_DATA_2_UUID,DATA_LEN,&p_per->data_2_hdlrs);
    ERR_CHK("Data 2 Char init");
    
    err_code = per_char_add(p_per, CMDS_RESULT_UUID, RESULT_LEN, &p_per->result_hdlrs);
    ERR_CHK("Result Char init");

//    LOG_D("HEADER :%02x, DATA_1 :%02x, DATA_2 :%02x\r\n",p_per->header_hdlrs.value_handle, p_per->data_1_hdlrs.value_handle,p_per->data_2_hdlrs.value_handle);
//    LOG_D("RESULT :%02x , CCCD : %02x\r\n", p_per->result_hdlrs.value_handle, p_per->result_hdlrs.cccd_handle);

    return NRF_SUCCESS;
}


uint32_t per_char_reset(per_t *p_per, ble_gatts_char_handles_t *data_handle, uint8_t *p_string, uint16_t length){
    ble_gatts_value_t p_val;
    memset(&p_val, 0, sizeof(p_val));
        
    p_val.len = length;
    p_val.offset = 0;
    p_val.p_value = p_string;
    
    return sd_ble_gatts_value_set(p_per->conn_handle,data_handle->value_handle, &p_val);
}

uint32_t per_char_update(per_t *p_per, ble_gatts_char_handles_t *data_handle, uint8_t *p_string, uint16_t length) {
    if ((p_per->conn_handle == BLE_CONN_HANDLE_INVALID) || (!p_per->notification)) {
        LOG_E("Check Noti or Conn State!!\r\n");
        return NRF_ERROR_INVALID_STATE;
    }

    if (length > MAX_CHAR_LEN) {
        return NRF_ERROR_INVALID_PARAM;
    }
    
    uint8_t param_type = BLE_GATT_HVX_INDICATION;
    
    if(data_handle->value_handle == p_per->result_hdlrs.value_handle){
        param_type = BLE_GATT_HVX_NOTIFICATION;
    }
    
    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = data_handle->value_handle;
    hvx_params.offset = 0;
    hvx_params.p_len = &length;
    hvx_params.p_data = p_string;
    hvx_params.type = param_type;

    return sd_ble_gatts_hvx(p_per->conn_handle, &hvx_params);
}
