#include "cen.h"
#include "ble_advertising.h"

#define NRF_LOG_MODULE_NAME "[cen]"

static uint32_t cccd_configure(uint16_t conn_handle, uint16_t cccd_handle, bool enable);

static uint32_t cen_noti_enable(cen_t *p_cen, uint16_t *handle);

static uint32_t cen_value_update(cen_t *p_cen, uint16_t *handle, uint8_t *p_string, uint16_t length);

uint32_t cen_header_update(cen_t *p_cen, p_header *header) {
    uint8_t buff[20];
    memcpy(buff, header, sizeof(p_header));
    return cen_value_update(p_cen, &p_cen->hdlrs.header_hdlr, buff, sizeof(p_header));
}

uint32_t cen_data_1_update(cen_t *p_cen, p_data *data) {
    uint8_t buff[20];
    memcpy(buff, &data->p_data[0], sizeof(buff));
    return cen_value_update(p_cen, &p_cen->hdlrs.data_1_hdlr, buff, sizeof(buff));
}

uint32_t cen_data_2_update(cen_t *p_cen, p_data *data) {
    uint8_t buff[20];
    memcpy(buff, &data->p_data[20], sizeof(buff));
    return cen_value_update(p_cen, &p_cen->hdlrs.data_2_hdlr, buff, sizeof(buff));
}

static void next_pkt_chk(void) {
    PKT.tx_p.tx_que[PKT.tx_p.proc_cnt] = CEN_TXP_QUEUE_UNAVAILABLE;
    PKT.tx_p.proc_cnt++;
    if (PKT.tx_p.tx_que[PKT.tx_p.proc_cnt] == CEN_TXP_QUEUE_UNAVAILABLE) {
        PKT.tx_p.proc = false;
    }
}

static void pkt_send_err(p_pkt *txp, ble_gap_addr_t *target_addr) {
    ble_gap_addr_t *cmp_addr = get_node(&txp->header.target.node,1,0);
    uint8_t tgt_idx = get_addr_idx(target_addr->addr);
    if (!memcmp(cmp_addr->addr, target_addr->addr, BLE_GAP_ADDR_LEN)) {
        pkt_build(CEN_SEND_TARGET_ERROR,&APP.net.node.peer[tgt_idx].id);
    } else {
        pkt_build(CEN_SEND_ROUTE_ERROR,&APP.net.node.peer[tgt_idx].id);
    }
}

ble_gap_addr_t* retrieve_send_addr(p_pkt *txp) {
    uint8_t tgt_node = txp->header.target.node;    
    if(tgt_node !=0 &&txp->header.path[0] != 0){
        for(int i=0;i<MAX_DEPTH_CNT;i++){
            if(APP.dev.my_id == txp->header.path[i]){
                tgt_node = txp->header.path[i+1];
                break;
            }
        }
    }

    ble_gap_addr_t *target_addr = retrieve_send(&tgt_node,1,0);
    
    if (!target_addr && txp->header.type == PKT_TYPE_NET_SCAN_REQ){
        target_addr = get_node(txp->data.p_data,0,1);
    }
    
    return target_addr;
}

void pkt_send(cen_t *p_cen) {
    static uint32_t req_cnt=0;
    uint32_t err_code;

//    LOG_I("txp, rxp,%d,%d \r\n",PKT.tx_p.proc_cnt, PKT.rx_p.proc_cnt);
    if (PKT.tx_p.proc) {
        nrf_delay_ms(100);
        
        p_pkt *txp = &PKT.tx_p.pkt[PKT.tx_p.proc_cnt];
        ble_gap_addr_t *target_addr;

        if (p_cen->conn_handle == BLE_CONN_HANDLE_INVALID) {
            target_addr = retrieve_send_addr(txp);
            
            if(req_cnt >= CEN_MAX_REQ_CNT) { //if app.net.set = true
                req_cnt=0;
                err_code = sd_ble_gap_connect_cancel();
                ERR_CHK("Connection Request Failed");
                pkt_send_err(txp,target_addr);
                return;
            }
            
            if (target_addr) {
                LOG_I("WAIT FOR PER - TARGET %s Type : %d TRIAL %dth\r\n", STR_PUSH(target_addr->addr, 1),target_addr->addr_type,req_cnt);
                err_code = sd_ble_gap_connect(target_addr, &m_scan_params, &m_connection_param);
                ERR_CHK("Connection Request Failed");
                req_cnt++;
                return;
            }

            LOG_E("TARGET NOT FOUND\r\n");
            return;
        }

        if (!p_cen->hdlrs.assigned) {
            LOG_D("PACKET TXPh : %s, \r\n", VSTR_PUSH((uint8_t *) &txp->header, HEADER_LEN, 0));
            LOG_D("PACKET TXPd : %s, \r\n", VSTR_PUSH((uint8_t *) &txp->data, DATA_LEN, 0));
            LOG_I("WAIT FOR HANDLE ASSIGNED\r\n");
            PKT.tx_p.proc = false;
            req_cnt = 0;
            return;
        }

        if (!p_cen->notification) {
            LOG_D("ENABLING NOTIFICATION \r\n");
            err_code = cen_noti_enable(p_cen, &p_cen->hdlrs.result_cccd_hdlr);
            ERR_CHK("Noti Enable Failed");
            PKT.tx_p.proc = false;
            return;
        }

        if (!p_cen->state.idle) {
            LOG_D("Wait For IDLE\r\n");
            PKT.tx_p.proc = false;
            return;
        }

        if (p_cen->state.send) {
            LOG_D("Wait For Response\r\n");
            PKT.tx_p.proc = false;
            return;
        } else if (!p_cen->state.send) {
            p_cen->state.send = true;

            if (!p_cen->state.header) {
                err_code = cen_header_update(p_cen, &txp->header);
                ERR_CHK("Header Update Failed");
                LOG_I("Header UPDATE SUCCESS\r\n");
                return;
            }

            if (!p_cen->state.data_1) {
                err_code = cen_data_1_update(p_cen, &txp->data);
                ERR_CHK("Data 1 Update Failed");
                LOG_I("Data 1 UPDATE SUCCESS\r\n");
                return;
            }

            if (txp->header.idx_tot >= 2) {
                if (!p_cen->state.data_2) {
                    err_code = cen_data_2_update(p_cen, &txp->data);
                    ERR_CHK("Data 2 Update Failed");
                    LOG_I("Data 2 UPDATE SUCCESS\r\n");
                    return;
                }
            }
        }

        if (!p_cen->state.interpret) {
            LOG_I("WAIT FOR PACKET INTERPRETING\r\n");
            return;
        } else if (p_cen->state.interpret) {
            next_pkt_chk();
            
            (void) sd_ble_gap_adv_stop();
            
            nrf_delay_ms(100);
            
            uint32_t err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
            APP_ERROR_CHECK(err_code);
            PKT.tx_p.proc_done = true;
            app_fds_save();

            nrf_delay_ms(100);
            
            LOG_I("CENTRAL CLOSING CONNECTION\r\n");
            err_code = sd_ble_gap_disconnect(p_cen->conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            
        }
    }
}

void scan_res_builder(uint8_t *p_data) {
    uint8_t p_idx = 0;
    uint8_t unit = BLE_GAP_ADDR_LEN + sizeof(uint8_t);

    for (int i = 0; i < APP.net.node.cnt; i++) {
        memcpy(&p_data[p_idx], APP.net.node.peer[i].p_addr.addr, BLE_GAP_ADDR_LEN);

        uint8_t u_rssi = -APP.net.node.peer[i].rssi;
        memcpy(&p_data[p_idx + BLE_GAP_ADDR_LEN], &u_rssi, sizeof(uint8_t));

        p_idx = p_idx + unit;
    }
    LOG_I("SCAN_RESPONSE BUILD %s, \r\n", VSTR_PUSH(p_data, APP.net.node.cnt * unit, 0));
}

void pkt_base(p_pkt *txp, uint8_t build_type) {
    txp->header.type = build_type;
    txp->header.err_type = 0;
    txp->header.idx_tot = 1;
    txp->header.source.node = APP.dev.my_id;
    txp->header.source.sensor = 0;
    txp->header.target.node = APP.dev.root_id;
    txp->header.target.sensor = 0;
}

void pkt_err_base(p_pkt *txp, uint8_t build_type) {
    txp->header.type ++; // Req => Res Packet
    txp->header.err_type = build_type;
    txp->header.source = txp->header.target;
    txp->header.target.node = APP.dev.root_id;
    txp->header.target.sensor = 0;
}

void pkt_build(uint8_t build_type, uint8_t *p_data) {
    p_pkt *txp = &PKT.tx_p.pkt[PKT.tx_p.pkt_cnt];
    p_pkt *rxp = &PKT.rx_p.pkt[PKT.rx_p.proc_cnt];

    LOG_I("PACKET BUILD TXP : %d, RXP : %d, \r\n", PKT.tx_p.pkt_cnt, PKT.rx_p.proc_cnt);
    
    switch(build_type){
        case CEN_BUILD_PACKET_ROUTE:
            memcpy(&txp->header, &rxp->header, HEADER_LEN);
            memcpy(&txp->data, &rxp->data, MAX_PKT_DATA_LEN);
            break;
        
        case CEN_SEND_TARGET_ERROR:
        case CEN_SEND_ROUTE_ERROR:
            memcpy(&(txp-1)->header, &(rxp-1)->header, HEADER_LEN); //Prev RxP Packet
            pkt_err_base(txp-1, build_type);
            memset(&(txp-1)->data, 0, sizeof(txp->data));
            (txp-1)->data.p_data[0]=APP.dev.my_id;
            (txp-1)->data.p_data[1]=47; //Ascii-Seperator '/'
            (txp-1)->data.p_data[2]=p_data[0];
            (txp-1)->data.p_data[3]=47; //Ascii-Seperator '/'
            LOG_D("BUILD TXPh : %s, \r\n", VSTR_PUSH((uint8_t *) &(txp-1)->header, HEADER_LEN, 0));
            LOG_D("BUILD TXPd : %s, \r\n", VSTR_PUSH((uint8_t *) &(txp-1)->data, DATA_LEN, 0));
            return; // Must Return (stop seq.) for Overwritng Idx
        
        default:
            pkt_base(txp, build_type);
            uint8_t data_len = 0;
            
            switch (build_type) {
                case PKT_TYPE_NET_SCAN_RES:{
                    data_len = (int) ceil((float) APP.net.node.cnt * 8 / DATA_LEN);
                    txp->header.idx_tot = (data_len == 0) ? 1 : data_len;
                    scan_res_builder(txp->data.p_data);
                }break;
                
                
                case PKT_TYPE_SNSR_STATE_REQ:{
                    txp->data.p_data[0] = PKT_DATA_SUCCESS;

                }break;
                case PKT_TYPE_SNSR_DATA_RES: {
                }break;
                case PKT_TYPE_SNSR_ACT_REQ: {
                }break;
                
                case PKT_TYPE_NODE_STAT_REQ:{
                }break;
                
                case PKT_TYPE_NET_JOIN_REQ:{
                }break;
                case PKT_TYPE_SNSR_CMD_RES:
                case PKT_TYPE_NODE_LED_RES:
                case PKT_TYPE_NODE_BTN_PRESS_REQ:
                case PKT_TYPE_NET_UPDATE_RES:
                case PKT_TYPE_NET_ACK_RES:
                    txp->data.p_data[0] = PKT_DATA_SUCCESS;
                    break;
                case PKT_TYPE_SCAN_TGT_RES:
                    memcpy(&txp->data.p_data,p_data,7); //RSSI OR 0
                default:
                    break;
            }
            break;
                
    }
    PKT.tx_p.proc = true;
    PKT.tx_p.tx_que[PKT.tx_p.que_idx] = PKT.tx_p.pkt_cnt;
    PKT.tx_p.pkt_cnt++;
    PKT.tx_p.que_idx++;
}

void cen_on_db_disc_evt(cen_t *p_cen, ble_db_discovery_evt_t *p_evt) {
    ble_gatt_db_char_t *p_chars = p_evt->params.discovered_db.charateristics;

    // Check if the CMDS was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == CMDS_UUID &&
        p_evt->params.discovered_db.srv_uuid.type == p_cen->uuid_type) {
        uint32_t i;

        cen_handlers_t *hdlr = &p_cen->hdlrs;
        LOG_D("Discovered Evt Count : %d\r\n", p_evt->params.discovered_db.char_count);
        for (i = 0; i < p_evt->params.discovered_db.char_count; i++) {
            switch (p_chars[i].characteristic.uuid.uuid) {
                case CMDS_HEADER_UUID:
                    hdlr->header_hdlr = p_chars[i].characteristic.handle_value;
                    break;

                case CMDS_DATA_1_UUID:
                    hdlr->data_1_hdlr = p_chars[i].characteristic.handle_value;
                    break;

                case CMDS_DATA_2_UUID:
                    hdlr->data_2_hdlr = p_chars[i].characteristic.handle_value;
                    break;

                case CMDS_RESULT_UUID:
                    hdlr->result_hdlr = p_chars[i].characteristic.handle_value;
                    hdlr->result_cccd_hdlr = p_chars[i].cccd_handle;
                    break;

                default:
                    break;
            }
        }

        LOG_D("HEADER :%02x, DATA_1 :%02x, DATA_2 :%02x\r\n", hdlr->header_hdlr, hdlr->data_1_hdlr, hdlr->data_2_hdlr);
        LOG_D("RESULT :%02x , CCCD : %02x\r\n", hdlr->result_hdlr, hdlr->result_cccd_hdlr);
        if (hdlr->header_hdlr && hdlr->data_1_hdlr && hdlr->data_2_hdlr && hdlr->result_hdlr &&
            hdlr->result_cccd_hdlr) {
//            LOG_D("ALL HANDLER ASSIGNED\r\n");
            hdlr->assigned = true;
            PKT.tx_p.proc = true;
        }
    }
}

//FOR NOTIFICATION ENABLE
static void on_write_rsp(cen_t *p_cen, const ble_evt_t *p_ble_evt) {
    if (p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS &&
        p_ble_evt->evt.gattc_evt.params.write_rsp.handle == p_cen->hdlrs.result_cccd_hdlr) {
        p_cen->notification = true;
        PKT.tx_p.proc = true;           
        LOG_D("PERIPHERAL's NOTIFICATION ENABLED!!\r\n");
    }
}

static void on_hvx(cen_t *p_cen, const ble_evt_t *p_ble_evt) {
    const ble_gattc_evt_hvx_t *p_evt_hvx = &p_ble_evt->evt.gattc_evt.params.hvx;
    // HVX can only occur from client sending.

    if (p_cen->hdlrs.assigned) {
        if (p_evt_hvx->handle == p_cen->hdlrs.result_hdlr) {
            p_cen->state.send = false;
            PKT.tx_p.proc = true;
            nrf_delay_ms(100);

            uint8_t *p_data = (uint8_t *) p_evt_hvx->data;
            if (p_data[0] == PKT_RSLT_IDLE) {
                p_cen->state.idle = true;
                LOG_D("IDLE OK\r\n");
            } else if (p_data[0] == PKT_RSLT_HEADER_OK) {
                p_cen->state.header = true;
                LOG_D("HEADER OK\r\n");
            } else if (p_data[0] == PKT_RSLT_DATA_1_OK) {
                p_cen->state.data_1 = true;
                LOG_D("DATA_1 OK\r\n");
            } else if (p_data[0] == PKT_RSLT_DATA_2_OK) {
                p_cen->state.data_2 = true;
                LOG_D("DATA_2 OK\r\n");
            } else if (p_data[0] == PKT_RSLT_INTERPRET_OK) {
                p_cen->state.interpret = true;
                LOG_D("INTERPRET OK\r\n");
            }
        }
    }
}

void app_cen_evt(cen_t *p_cen, const ble_evt_t *p_ble_evt) {
    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            p_cen->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GATTC_EVT_WRITE_RSP:
//            LOG_D("PERIPHERAL's Write RSP %02x %d!!\r\n", p_ble_evt->evt.gattc_evt.params.write_rsp.handle,
//                  p_ble_evt->evt.gattc_evt.gatt_status);
            on_write_rsp(p_cen, p_ble_evt);
            break;

        case BLE_GATTC_EVT_HVX:
            on_hvx(p_cen, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            if(PKT.tx_p.proc_done == false) { //Packet Send Error Recovery..
                PKT.tx_p.proc = true;
            }
//            LOG_D("RESETTING CENTRAL\r\n");
            memset(&p_cen->state, 0, sizeof(cen_state_t));
            memset(&p_cen->hdlrs, 0, sizeof(cen_handlers_t));
            PKT.tx_p.proc_done = false;
            p_cen->notification = false;
            p_cen->conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        default:
            // No implementation needed.
            break;
    }
}

uint32_t cen_init(cen_t *p_cen) {
    uint32_t err_code;
    ble_uuid_t cmds_uuid;
    ble_uuid128_t cmds_base_uuid = CMDS_BASE_UUID;

    err_code = sd_ble_uuid_vs_add(&cmds_base_uuid, &cmds_uuid.type);
    APP_ERROR_CHECK(err_code);

    cmds_uuid.uuid = CMDS_UUID;

    p_cen->uuid_type = cmds_uuid.type;
    p_cen->conn_handle = BLE_CONN_HANDLE_INVALID;
    memset(&p_cen->hdlrs, BLE_GATT_HANDLE_INVALID, sizeof(cen_handlers_t));

    return ble_db_discovery_evt_register(&cmds_uuid);
}


static uint32_t cen_value_update(cen_t *p_cen, uint16_t *handle, uint8_t *p_string, uint16_t length) {
    if (p_cen->conn_handle == BLE_CONN_HANDLE_INVALID) {
        return NRF_ERROR_INVALID_STATE;
    }
    if (length > MAX_CHAR_LEN) {
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

    return sd_ble_gattc_write(p_cen->conn_handle, &write_params);
}

/**@brief Function for creating a message for writing to the CCCD.
 */
static uint32_t cccd_configure(uint16_t conn_handle, uint16_t cccd_handle, bool enable) {
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

uint32_t cen_noti_enable(cen_t *p_cen, uint16_t *handle) {
    if ((p_cen->conn_handle == BLE_CONN_HANDLE_INVALID) || (*handle == BLE_GATT_HANDLE_INVALID)) {
        return NRF_ERROR_INVALID_STATE;
    }
    return cccd_configure(p_cen->conn_handle, *handle, true);
}
