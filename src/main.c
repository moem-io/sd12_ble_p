#include "nordic_common.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_db_discovery.h"
#include "boards.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "peer_manager.h"
#include "ble_conn_state.h"

#include "util.h"

#include "Fuel_Gauge.h"
#include "LED.h"
#include "Button.h"

#include "Detect.h"

#include "Sensor_Communication.h"

#define DEBUG

//#define TEST_LED
//#define TEST_BUZZER
//#define TEST_IR
//#define TEST_TH

#define NRF_LOG_MODULE_NAME "APP"

#define IS_SRVC_CHANGED_CHARACT_PRESENT 1                                           /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/

#if (NRF_SD_BLE_API_VERSION == 3)
#define NRF_BLE_MAX_MTU_SIZE            GATT_MTU_SIZE_DEFAULT                       /**< MTU size used in the softdevice enabling and to reply to a BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST event. */
#endif

#define APP_FEATURE_NOT_SUPPORTED       BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2        /**< Reply when unsupported features are requested. */

#define CENTRAL_LINK_COUNT              1
#define PERIPHERAL_LINK_COUNT           1

#define CENTRAL_SCANNING_LED        BSP_BOARD_LED_0
#define CENTRAL_CONNECTED_LED       BSP_BOARD_LED_1

#define DEVICE_NAME_PREFIX                     "Mx"
#define APP_ADV_INTERVAL                300                                         /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS      0                                         /**< The advertising timeout in units of seconds. */

#define APP_TIMER_PRESCALER             0                                           /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE         4                                           /**< Size of timer operation queues. */

#define MIN_CONN_INTERVAL     (uint16_t) MSEC_TO_UNITS(7.5, UNIT_1_25_MS)   /**< Determines minimum connection interval in milliseconds. */
#define MAX_CONN_INTERVAL     (uint16_t) MSEC_TO_UNITS(30, UNIT_1_25_MS)    /**< Determines maximum connection interval in milliseconds. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds). */

//PERIPHERAL SETTINGS
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                  0                                           /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS              0                                           /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                        /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size. */

//CENTRAL SETTINGS
#define SCAN_INTERVAL               0x00A0                                        /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                 0x0050                                        /**< Determines scan window in units of 0.625 millisecond. */
#define SCAN_TIMEOUT                0x000A

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define FINAL

uint8_t flagLED;
uint8_t file_flag;

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;                            /**< Handle of the current connection. */

app_condition APP;
app_packet PKT;

gap_disc tmp_disc; //for SCAN TARGET
uint8_t tmp_addr[BLE_GAP_ADDR_LEN];
int tmp_base_rssi[MAX_DISC_QUEUE];

static per_t m_per_s;
static cen_t m_cen_s;
static ble_db_discovery_t m_ble_db_discovery;             /**< Instance of database discovery module. Must be passed to all db_discovert API calls */

APP_TIMER_DEF(m_single_timer);
#define NET_DISC_TIMER_INTERVAL     APP_TIMER_TICKS(10000, APP_TIMER_PRESCALER) // 1000 ms intervals

__IO flagButton callBackButton 				= {.flag = false };
__IO flagDetect flagCharger 			= {.flag = false };
__IO flagDetect flagLowBattery = {.flag = false };
__IO flagDetect flagSensor 			= {.flag = false };
__IO flagUART	flagCommunication 					= { .flag = false };

__IO flagPacket flagCommand_Sensor = {.flag = false};
__IO flagPacket flagCommand_Node = {.flag = false };

volatile bool tgt_scan = false;

const ble_gap_conn_params_t m_connection_param =
    {
        (uint16_t) MIN_CONN_INTERVAL,  // Minimum connection
        (uint16_t) MAX_CONN_INTERVAL,  // Maximum connection
        (uint16_t) SLAVE_LATENCY,            // Slave latency
        (uint16_t) CONN_SUP_TIMEOUT       // Supervision time-out
    };

const ble_gap_scan_params_t m_scan_params =
    {
        .active   = 1,
        .interval = SCAN_INTERVAL,
        .window   = SCAN_WINDOW,
        .timeout  = SCAN_TIMEOUT,
#if (NRF_SD_BLE_API_VERSION == 2)
    .selective   = 0,
    .p_whitelist = NULL,
#endif
#if (NRF_SD_BLE_API_VERSION == 3)
        .use_whitelist = 0,
#endif
    };

static void advertising_start(void);

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name) {
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}
void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info){
	LOG_D("id : %X, pc : %X", id, pc);
	LOG_D("info->line_num : %d, info->p_file_name : %s, info->err_code : %d\r\n", ((error_info_t *)info)->line_num, LOG_PUSH((char *const )((error_info_t *)info)->p_file_name), ((error_info_t *)info)->err_code);

}


/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const *p_evt) {
    ret_code_t err_code;

    switch (p_evt->evt_id) {
        case PM_EVT_BONDED_PEER_CONNECTED: {
            LOG_I("Connected to a previously bonded device.\r\n");
        }
            break;

        case PM_EVT_CONN_SEC_SUCCEEDED: {
            LOG_I("Connection secured. Role: %d. conn_handle: %d, Procedure: %d\r\n",
                  ble_conn_state_role(p_evt->conn_handle),
                  p_evt->conn_handle,
                  p_evt->params.conn_sec_succeeded.procedure);
        }
            break;

        case PM_EVT_CONN_SEC_FAILED: {
            /* Often, when securing fails, it shouldn't be restarted, for security reasons.
             * Other times, it can be restarted directly.
             * Sometimes it can be restarted, but only after changing some Security Parameters.
             * Sometimes, it cannot be restarted until the link is disconnected and reconnected.
             * Sometimes it is impossible, to secure the link, or the peer device does not support it.
             * How to handle this error is highly application dependent. */
        }
            break;

        case PM_EVT_CONN_SEC_CONFIG_REQ: {
            // Reject pairing request from an already bonded peer.
            pm_conn_sec_config_t conn_sec_config = {.allow_repairing = false};
            pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
        }
            break;

        case PM_EVT_STORAGE_FULL: {
            // Run garbage collection on the flash.
            err_code = fds_gc();
            if (err_code == FDS_ERR_BUSY || err_code == FDS_ERR_NO_SPACE_IN_QUEUES) {
                // Retry.
            } else {
                APP_ERROR_CHECK(err_code);
            }
        }
            break;

        case PM_EVT_PEERS_DELETE_SUCCEEDED: {
            advertising_start();
        }
            break;

        case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED: {
            // The local database has likely changed, send service changed indications.
            pm_local_database_has_changed();
        }
            break;

        case PM_EVT_PEER_DATA_UPDATE_FAILED: {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peer_data_update_failed.error);
        }
            break;

        case PM_EVT_PEER_DELETE_FAILED: {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
        }
            break;

        case PM_EVT_PEERS_DELETE_FAILED: {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
        }
            break;

        case PM_EVT_ERROR_UNEXPECTED: {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
        }
            break;

        case PM_EVT_CONN_SEC_START:
        case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
        case PM_EVT_PEER_DELETE_SUCCEEDED:
        case PM_EVT_LOCAL_DB_CACHE_APPLIED:
        case PM_EVT_SERVICE_CHANGED_IND_SENT:
        case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
        default:
            break;
    }
}


static void timer_timeout_handler(void *p_context) {
    // // // LOG_D("Timer Timeout!!\r\n");
    APP.timer.status = APP_TIMER_STATUS_DISABLED;
    APP.timer.timeout = APP_TIMER_TIMEOUT_TRUE;
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static uint32_t timers_init(void) {
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);

    return app_timer_create(&m_single_timer, APP_TIMER_MODE_SINGLE_SHOT, timer_timeout_handler);
}

static void gap_params_init(void) {
    uint32_t err_code;
    ble_gap_conn_params_t gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) APP.dev.name,
                                          strlen(APP.dev.name));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

static void services_init(void) {
    uint32_t err_code;

    err_code = per_init(&m_per_s);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t *p_evt) {
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED) {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

static void conn_params_error_handler(uint32_t nrf_error) {
    APP_ERROR_HANDLER(nrf_error);
}

static void conn_params_init(void) {
    uint32_t err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail = false;
    cp_init.evt_handler = on_conn_params_evt;
    cp_init.error_handler = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

static void db_disc_handler(ble_db_discovery_evt_t *p_evt) {
    cen_on_db_disc_evt(&m_cen_s, p_evt);
}

static void db_discovery_init(void) {
    ret_code_t err_code = ble_db_discovery_init(db_disc_handler);
    APP_ERROR_CHECK(err_code);
}

void scan_start(void) {
    ret_code_t err_code;

    memset(&tmp_disc, 0, sizeof(tmp_disc));
    memset(tmp_base_rssi, 0, sizeof(tmp_base_rssi));

    (void) sd_ble_gap_scan_stop();

    err_code = sd_ble_gap_scan_start(&m_scan_params);
    // It is okay to ignore this error since we are stopping the scan anyway.
    if (err_code != NRF_ERROR_INVALID_STATE) {
        APP_ERROR_CHECK(err_code);
    }
    LOG_I("Start Scanning\r\n");
}

static void advertising_start(void) {
    uint32_t err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
//static void sleep_mode_enter(void) {
//    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);

//    APP_ERROR_CHECK(err_code);

//    // Prepare wakeup buttons.
//    err_code = bsp_btn_ble_sleep_mode_prepare();
//    APP_ERROR_CHECK(err_code);

//    // Go to system-off mode (this function will not return; wakeup will cause a reset).
//    err_code = sd_power_system_off();
//    APP_ERROR_CHECK(err_code);
//}


//170228 [TODO] : IF NO NODE FOUND??
// TODO: More Structured. Check if Duplicate Exists.
void net_disc(gap_disc *disc, const ble_evt_t *const p_ble_evt) {
    const ble_gap_evt_adv_report_t *p_adv_report = &p_ble_evt->evt.gap_evt.params.adv_report;

    if (disc->cnt < MAX_DISC_QUEUE) {
        if (is_uuid_present(&m_cmds_uuid, p_adv_report)) {
            for (int i = 0; i < disc->cnt; i++) {
                if (!memcmp(disc->peer[i].p_addr.addr, p_adv_report->peer_addr.addr, BLE_GAP_ADDR_LEN)) {
                    if (disc->peer[i].rssi_cnt < MAX_RSSI_CNT) {
                        tmp_base_rssi[i] += p_adv_report->rssi;
                        disc->peer[i].rssi_cnt++;
                        disc->peer[i].rssi = tmp_base_rssi[i] / disc->peer[i].rssi_cnt;
                        LOG_D("count %d : Addr : %s Rssi : %d \r\n",
                              disc->peer[i].rssi_cnt, STR_PUSH(disc->peer[i].p_addr.addr, 1), disc->peer[i].rssi);
                    }
                    return;
                }
            }

            disc->peer[disc->cnt].p_addr = p_adv_report->peer_addr;
            disc->peer[disc->cnt].rssi = p_adv_report->rssi;
            disc->peer[disc->cnt].rssi_cnt = 1;
            disc->peer[disc->cnt].disc = true;
            tmp_base_rssi[disc->cnt] = p_adv_report->rssi;
            disc->cnt += 1;

            for (int i = 0; i < disc->cnt; i++) {
                LOG_I("No %d : Addr : %s Rssi : %d \r\n", i, STR_PUSH(disc->peer[i].p_addr.addr, 1),
                      disc->peer[i].rssi);
            }
        }
    } else {
        LOG_E("MAX_DISC_COUNT OVER!!\r\n");
    }
}

void node_disc_chk() {
    for (int i = 0; i < APP.net.node.cnt; i++) {
        if (APP.net.node.peer[i].rssi_cnt < MIN_DISC_REG_CNT) {
            for (int j = i; j < APP.net.node.cnt; j++) {
                APP.net.node.peer[j] = APP.net.node.peer[j + 1];
                APP.net.node.cnt -= 1;
            }
        }
    }
}

/**@brief Function for handling BLE Stack events concerning central applications.
 *
 * @details This function keeps the connection handles of central applications up-to-date. It
 * parses scanning reports, initiating a connection attempt to peripherals when a target UUID
 * is found, and manages connection parameter update requests. Additionally, it updates the status
 * of LEDs used to report central applications activity.
 *
 * @note        Since this function updates connection handles, @ref BLE_GAP_EVT_DISCONNECTED events
 *              should be dispatched to the target application before invoking this function.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void nrf_cen_evt(const ble_evt_t *const p_ble_evt) {
    const ble_gap_evt_t *const p_gap_evt = &p_ble_evt->evt.gap_evt;
    ret_code_t err_code;

    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED: {
            LOG_I("Central Connected \r\n");

//            bsp_board_led_on(CENTRAL_CONNECTED_LED);
            memset(&m_ble_db_discovery, 0, sizeof(m_ble_db_discovery));
            err_code = ble_db_discovery_start(&m_ble_db_discovery, p_gap_evt->conn_handle);
            APP_ERROR_CHECK(err_code);
        }
            break; // BLE_GAP_EVT_CONNECTED

        case BLE_GAP_EVT_DISCONNECTED: {
            LOG_I("CENTRAL DISCONNECTED (reason: %d)\r\n", p_gap_evt->params.disconnected.reason);

//            bsp_board_led_off(CENTRAL_CONNECTED_LED);
        }
            break; // BLE_GAP_EVT_DISCONNECTED

        case BLE_GAP_EVT_ADV_REPORT:
            if (!tgt_scan) {
                net_disc(&APP.net.node, p_ble_evt);
            } else {
                net_disc(&tmp_disc, p_ble_evt);
            }
            break; // BLE_GAP_ADV_REPORT

        case BLE_GAP_EVT_TIMEOUT: {
            if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN) {
                if (!tgt_scan) {
//                    LOG_I("NET SCANNING TIMEOUT -- %d FOUND!!\r\n", APP.net.node.cnt);
//                    node_disc_chk();
                    LOG_I("NET Discovery Checked! -- %d FOUND!!\r\n", APP.net.node.cnt);
                    APP.net.discovered = APP_NET_DISCOVERED_TRUE;

                    pkt_build(PKT_TYPE_NET_SCAN_RES, 0, 0);
                } else { //TGT SCAN SEQ. (DISC)? RSSI :0;

                    tgt_scan = false;
                    uint8_t tmp_res[BLE_GAP_ADDR_LEN + 1] = {0,}; //1 for RSSI
                    memcpy(tmp_res, tmp_addr, BLE_GAP_ADDR_LEN);

                    for (int i = 0; i < tmp_disc.cnt; i++) {
                        if (!memcmp(tmp_addr, tmp_disc.peer[i].p_addr.addr, BLE_GAP_ADDR_LEN)) {
                            LOG_D("rssi VAlue %d\r\n", tmp_disc.peer[i].rssi);
                            tmp_res[6] = -tmp_disc.peer[i].rssi;
                            break;
                        }
                    }

                    pkt_build(PKT_TYPE_SCAN_TGT_RES, tmp_res, 0);
                }

            } else if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN) {
                LOG_I("Connection Request timed out.\r\n");
            }
        }
            break; // BLE_GAP_EVT_TIMEOUT

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST: {
            // Accept parameters requested by peer.
            err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                                    &p_gap_evt->params.conn_param_update_request.conn_params);
            APP_ERROR_CHECK(err_code);
        }
            break; // BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            LOG_D("GATT Client Timeout.\r\n");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GATTC_EVT_TIMEOUT

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            LOG_D("GATT Server Timeout.\r\n");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GATTS_EVT_TIMEOUT

#if (NRF_SD_BLE_API_VERSION == 3)
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
            err_code = sd_ble_gatts_exchange_mtu_reply(p_ble_evt->evt.gatts_evt.conn_handle,
                                                       NRF_BLE_MAX_MTU_SIZE);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST
#endif

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for handling BLE Stack events involving peripheral applications. Manages the
 * LEDs used to report the status of the peripheral applications.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void nrf_per_evt(ble_evt_t *p_ble_evt) {
    const ble_gap_evt_t *const p_gap_evt = &p_ble_evt->evt.gap_evt;
    ret_code_t err_code;
    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            LOG_I("Peripheral connected\r\n");
            break; //BLE_GAP_EVT_CONNECTED

        case BLE_GAP_EVT_DISCONNECTED:
            LOG_I("PERIPHERAL DISCONNECTED  (reason: %d)\r\n", p_gap_evt->params.disconnected.reason);
            break;//BLE_GAP_EVT_DISCONNECTED

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            LOG_D("GATT Client Timeout.\r\n");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GATTC_EVT_TIMEOUT

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            LOG_D("GATT Server Timeout.\r\n");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GATTS_EVT_TIMEOUT

        case BLE_EVT_USER_MEM_REQUEST:
            err_code = sd_ble_user_mem_reply(p_ble_evt->evt.gap_evt.conn_handle, NULL);
            APP_ERROR_CHECK(err_code);
            break;//BLE_EVT_USER_MEM_REQUEST

        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST: {
            ble_gatts_evt_rw_authorize_request_t req;
            ble_gatts_rw_authorize_reply_params_t auth_reply;

            req = p_ble_evt->evt.gatts_evt.params.authorize_request;

            if (req.type != BLE_GATTS_AUTHORIZE_TYPE_INVALID) {
                if ((req.request.write.op == BLE_GATTS_OP_PREP_WRITE_REQ) ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL)) {
                    if (req.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE) {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
                    } else {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
                    }
                    auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
                    err_code = sd_ble_gatts_rw_authorize_reply(p_ble_evt->evt.gatts_evt.conn_handle,
                                                               &auth_reply);
                    APP_ERROR_CHECK(err_code);
                }
            }
        }
            break; // BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST

#if (NRF_SD_BLE_API_VERSION == 3)
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
            err_code = sd_ble_gatts_exchange_mtu_reply(p_ble_evt->evt.gatts_evt.conn_handle,
                                                       NRF_BLE_MAX_MTU_SIZE);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST
#endif

        default:
            // No implementation needed.
            break;
    }
}


static void on_adv_evt(ble_adv_evt_t ble_adv_evt) {
    switch (ble_adv_evt) {
        case BLE_ADV_EVT_FAST:
            LOG_I("Fast advertising\r\n");
            break;

        case BLE_ADV_EVT_IDLE:
            //sleep_mode_enter();
            break;

        default:
            break;
    }
}

static void ble_evt_dispatch(ble_evt_t *p_ble_evt) {
    uint16_t conn_handle;
    uint16_t role;

//    LOG_D("EVT ID : %d \r\n", p_ble_evt->header.evt_id);
    ble_conn_state_on_ble_evt(p_ble_evt);
    pm_on_ble_evt(p_ble_evt);

    conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
    role = ble_conn_state_role(conn_handle);

    if (role == BLE_GAP_ROLE_PERIPH) {
        nrf_per_evt(p_ble_evt);
        app_per_evt(&m_per_s, p_ble_evt);

        ble_advertising_on_ble_evt(p_ble_evt);
        ble_conn_params_on_ble_evt(p_ble_evt);
    } else if ((role == BLE_GAP_ROLE_CENTRAL)
               || (p_ble_evt->header.evt_id == BLE_GAP_EVT_ADV_REPORT)
               || (p_ble_evt->header.evt_id == BLE_GAP_EVT_TIMEOUT)) {
        nrf_cen_evt(p_ble_evt);
        ble_db_discovery_on_ble_evt(&m_ble_db_discovery, p_ble_evt);
        app_cen_evt(&m_cen_s, p_ble_evt);
    }
    pkt_interpret(&m_per_s);

//    bsp_btn_ble_on_ble_evt(p_ble_evt);
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in] sys_evt  System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt) {
    // Dispatch the system event to the fstorage module, where it will be
    // dispatched to the Flash Data Storage (FDS) module.
    fs_sys_event_handler(sys_evt);

    // Dispatch to the Advertising module last, since it will check if there are any
    // pending flash operations in fstorage. Let fstorage proc system events first,
    // so that it can report correctly to the Advertising module.
    ble_advertising_on_sys_evt(sys_evt);
}


static void ble_stack_init(void) {
    uint32_t err_code;

    nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;

    SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);

    ble_enable_params_t ble_enable_params;
    err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
                                                    PERIPHERAL_LINK_COUNT,
                                                    &ble_enable_params);
    APP_ERROR_CHECK(err_code);

    CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT);

    // Enable BLE stack.
#if (NRF_SD_BLE_API_VERSION == 3)
    ble_enable_params.gatt_enable_params.att_mtu = NRF_BLE_MAX_MTU_SIZE;
#endif
    err_code = softdevice_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the Peer Manager initialization.
 *
 * @param[in] erase_bonds  Indicates whether bonding information should be cleared from
 *                         persistent storage during initialization of the Peer Manager.
 */
static void peer_manager_init(bool erase_bonds) {
    ble_gap_sec_params_t sec_param;
    ret_code_t err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    if (erase_bonds) {
        err_code = pm_peers_delete();
        APP_ERROR_CHECK(err_code);
    }

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond = SEC_PARAM_BOND;
    sec_param.mitm = SEC_PARAM_MITM;
    sec_param.lesc = SEC_PARAM_LESC;
    sec_param.keypress = SEC_PARAM_KEYPRESS;
    sec_param.io_caps = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob = SEC_PARAM_OOB;
    sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc = 1;
    sec_param.kdist_own.id = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void) {
    uint32_t err_code;
    ble_advdata_t advdata;
    ble_advdata_t srdata;
    ble_adv_modes_config_t options;

    static ble_uuid_t m_adv_uuids[] = {{CMDS_UUID, BLE_UUID_TYPE_VENDOR_BEGIN}};

    // Build advertising data struct to pass into @ref ble_advertising_init.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = false;
    advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    advdata.uuids_complete.p_uuids = m_adv_uuids;

    memset(&srdata, 0, sizeof(srdata));
    srdata.name_type = BLE_ADVDATA_FULL_NAME;

    memset(&options, 0, sizeof(options));
    options.ble_adv_fast_enabled = true;
    options.ble_adv_fast_interval = APP_ADV_INTERVAL;
    options.ble_adv_fast_timeout = APP_ADV_TIMEOUT_IN_SECONDS;

    err_code = ble_advertising_init(&advdata, &srdata, &options, on_adv_evt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the Power manager.
 */
static void power_manage(void) {
    uint32_t err_code = sd_app_evt_wait();

    APP_ERROR_CHECK(err_code);
}


static void device_preset() {
    uint32_t err_code;

    uint8_t num_rand_bytes_available;
    uint8_t rand_number;

    err_code = sd_ble_gap_address_get(&APP.dev.my_addr);
    ERR_CHK("GAP ADDR get Fail");

    nrf_delay_ms(100); // wait for random pool to be filled.
    err_code = sd_rand_application_bytes_available_get(&num_rand_bytes_available);
    ERR_CHK("Ramdom init FAIL");

    if (num_rand_bytes_available > 0) {
        err_code = sd_rand_application_vector_get(&rand_number, 1);
        ERR_CHK("Ramdom GET FAIL");
    }

    sprintf(APP.dev.name, "%s%03d", DEVICE_NAME_PREFIX, rand_number);

#ifdef FINAL

    err_code = LED_Init();
    ERR_CHK("LED init Fail");

    err_code = Button_Init();
    ERR_CHK("Button init Fail");

    err_code = Fuel_Gauge_Init();
    ERR_CHK("FG init Fail");

    err_code = Detect_Init();
    ERR_CHK("Detect init Fail");

#endif // FINAL

}


static void app_fds_evt_handler(fds_evt_t const *const p_fds_evt) {
    switch (p_fds_evt->id) {
        case FDS_EVT_INIT:
            if (p_fds_evt->result != FDS_SUCCESS) {
                // // LOG_D("FDS Init Failed!!  %d  \r\n",p_fds_evt->result);
            }
            break;
        case FDS_EVT_WRITE:
            if (p_fds_evt->result == FDS_SUCCESS) {
                file_flag = 1;
            }
            break;
        default:
            break;
    }
}


static ret_code_t app_fds_init(void) {
    ret_code_t ret = fds_register(app_fds_evt_handler);
    if (ret != FDS_SUCCESS) {
        return ret;
    }

    ret = fds_init();
    if (ret != FDS_SUCCESS) {
        return ret;
    }

    return NRF_SUCCESS;
}

void Button_Click_CallBack() {
//TODO: Sequence must change to NVIC
	
	#ifdef TEST_LED
	
	#endif
	#ifdef TEST_BUZZER
	
	#endif
	#ifdef TEST_IR
	
	#endif
	#ifdef TEST_TH
	
	#endif
	
	#if defined(TEST_LED) | defined(TEST_BUZZER) | defined(TEST_IR) | defined(TEST_TH)
		
	#else
		callBackButton.count = (callBackButton.count + 1) % 3;
	#endif
	
	
	callBackButton.flag = true;
}


void Detect_CallBack(uint32_t rising, uint32_t falling) {
    flagSensor.flag = true;
    flagSensor.rising = rising;
    flagSensor.falling = falling;
}

void UART_Receive_CallBack(char *messages) {
    flagCommunication.flag = true;
    strcpy(flagCommunication.bufferPacket, messages);
}

void sensor_check() {
    Packet *packet;
    char data[2];
    Packet_Status err_code;

    if (flagSensor.state == Falling) {
        if (getState_Channel(flagSensor.pin) == Rising || !checkChannel(flagSensor.pin)) {
#ifdef DEBUG
            LOG_I("ID %d is Falling\r\n", flagSensor.pin); //LOG_P??
#endif

            setState_Channel(flagSensor.pin, Falling);
            setSensor_Channel(flagSensor.pin, Sensor_None);

					if(APP.net.established){
            data[0] = Sensor_None;
            pkt_build(PKT_TYPE_SNSR_STATE_REQ, data, flagSensor.pin);
					}

            //Sensor detect
        }
    } else {
        if (getState_Channel(flagSensor.pin) == Falling || checkChannel(flagSensor.pin)) {
            setState_Channel(flagSensor.pin, Rising);

            LED_Control("0000FF");
            setState_Set_Address(flagSensor.pin);

            nrf_delay_ms(1000);
#ifdef DEBUG
            LOG_I("ID %d is Rising\r\n", flagSensor.pin);//LOG_P??
#endif

            while (!flagCommunication.flag);
            flagCommunication.flag = false;

#ifdef DEBUG
            LOG_I("flagCommunication.bufferPacket : %s\r\n", LOG_PUSH(flagCommunication.bufferPacket));
#endif

#ifdef DEBUG
            switch (releasePacket(flagCommunication.bufferPacket)) {
                case Packet_Ok:
                    LOG_I("Packet_Ok\r\n");
								
                    packet = getReceivePacket();
                    sprintf(data, "%c", flagSensor.pin + 'A');
                    Send_Packet_Polling(Set_Address, packet->sensor, packet->id, data);

                    setSensor_Channel(flagSensor.pin, packet->sensor);

										if(APP.net.established){
												pkt_build(PKT_TYPE_SNSR_STATE_REQ, &packet->sensor,flagSensor.pin);
										}
                    break;
                case Packet_NULL:

                    LOG_I("Packet_NULL\r\n");

                    break;
                case Packet_Length_ERROR:

                    LOG_I("Packet_Length_ERROR\r\n");

                    break;
                case Packet_Mode_ERROR:

                    LOG_I("Packet_Mode_ERROR\r\n");

                    break;
                case Packet_Sensor_ERROR:

                    LOG_I("Packet_Sensor_ERROR\r\n");

                    break;
                case Packet_Id_ERROR:

                    LOG_I("Packet_Id_ERROR\r\n");

                    break;
                case Packet_Data_ERROR:

                    LOG_I("Packet_Data_ERROR\r\n");

                    break;
                case Packet_Dropout:

                    LOG_I("Packet_Dropout\r\n");

                    break;
            }
#else
            err_code = releasePacket( flagCommunication.bufferPacket );

            if(err_code == Packet_Ok){
                packet = getReceivePacket();
                sprintf(data, "%c", flagSensor.pin + 'A');
                Send_Packet_Polling(Set_Address, packet->sensor, packet->id, data);

                setSensor_Channel(flagSensor.pin, packet->sensor);
							
								if(APP.net.established){
										pkt_build(PKT_TYPE_SNSR_STATE_REQ, &packet->sensor,flagSensor.pin);
								}
            }
#endif
            // Type : packet->sensor ID: flagSensor.pin
            // Sensor Type Packet Build

            LED_Control((uint8_t *) "000000");
        }
    }
}

/**@brief Function for application main entry.
 */
int main(void) {
    uint32_t err_code;

    Packet *packet;
    char data[10];

    uint8_t buttonCount = 0;

    memset(&APP, 0, sizeof(APP));
    memset(&PKT.tx_p.tx_que, CEN_TXP_QUEUE_UNAVAILABLE, sizeof(PKT.tx_p.tx_que)); //for tx_que index

    // Initialize.
    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    err_code = Sensor_Communication_Init();
    ERR_CHK("Sensor Communication Fail");

    err_code = timers_init();
    ERR_CHK("Timer init Fail");

    ble_stack_init();
    peer_manager_init(false); //fds init.

    err_code = app_fds_init();
    ERR_CHK("FDS init Fail");
    app_fds_read();

    device_preset();
    app_fds_save();

    db_discovery_init();
    err_code = cen_init(&m_cen_s);
    ERR_CHK("Cen init Fail");

    gap_params_init();
    services_init();
    advertising_init();
    conn_params_init();

    nrf_delay_ms(300);
		
    advertising_start();
    APP_ERROR_CHECK(err_code);
   
#ifdef FINAL

#ifdef DEBUG
    if (Fuel_Gauge_Config()) {
        LOG_I("BQ27441 is Worked!! %s \r\n", LOG_PUSH(Fuel_Gauge_getBatteryStatus()));
    } else {
        LOG_I("BQ27441 isn't Worked!!\r\n");
    }
#endif

    LED_Control((uint8_t *) "00FF00");
    nrf_delay_ms(1000);
    LED_Control((uint8_t *) "000000");

    LOG_I("FDS: %d Wd  %s Addr : %s \r\n", sizeof(APP) / 4 + 1, LOG_PUSH(APP.dev.name),
          STR_PUSH(APP.dev.my_addr.addr, 1));

#endif // FINAL

    for (;;) {
        if (flagLED) {
            if (LED_Control((uint8_t *)PKT.rx_p.pkt[PKT.rx_p.proc_cnt - 1].data.p_data)) {
                //  TODO : IF success
            }
            flagLED = false;
						
						nrf_delay_ms(100); //TODO: Maybe conflict with FSM
						pkt_build(PKT_TYPE_NODE_LED_RES,0,0);
						nrf_delay_ms(100);
        }
        
        if(callBackButton.flag){
            if(checkButton() == Falling){
							#ifdef TEST_LED
							buttonCount = (buttonCount + 1) % 8;
							switch(buttonCount){
								case 1:
									sprintf(data, "%s", "0F0000");
								break;
								case 2:
									sprintf(data, "%s", "000F00");
								break;
								case 3:
									sprintf(data, "%s", "00000F");
								break;
								case 4:
									sprintf(data, "%s", "0F0F00");
								break;
								case 5:
									sprintf(data, "%s", "0F000F");
								break;
								case 6:
									sprintf(data, "%s", "000F0F");
								break;
								case 7:
									sprintf(data, "%s", "0F0F0F");
								break;
								case 0:
									sprintf(data, "%s", "000000");
								break;
							}
							#endif
							
							#ifdef TEST_BUZZER
							buttonCount = (buttonCount + 1) % 8;
							sprintf(data, "%d", buttonCount);
							#endif
							
							#ifdef TEST_IR
							buttonCount = (buttonCount + 1) % 4;
							sprintf(data, "%d", buttonCount);
							#endif
							
							#ifdef TEST_TH
							
							data[0] = '\0';
								
							#endif
								
							#if defined(TEST_LED) | defined(TEST_BUZZER) | defined(TEST_IR) | defined(TEST_TH) 
								Send_Packet_Polling(Command, 'R', ID1, data);
								
							#else
								//  TODO : Do it Reset
								if(callBackButton.count == 2){
									NVIC_SystemReset();
								}
							#endif
            }
						
            callBackButton.flag = false;
        }
        
        if(flagSensor.flag){
            checkEdge(&flagSensor);
            
            switch(flagSensor.pin){
                case ID1: case ID2: case ID3: case ID4: case ID5:
                sensor_check();
                break;
                    
                case ID_GPOUT:
                    if(flagSensor.state == Falling){
                        if( FG_isBattery_Low() ){
													LED_Control("0F0000");
												}else{
													LED_Control("000000");
												}
                    }
                break;
                case ID_CHG:
                    if(flagSensor.state == Falling){
                        LED_Control("0F0F00");
                    }else{
                        LED_Control("000000");
                    }
                break;
            }
            
            flagSensor.flag = false;
        }
				
				if( flagCommunication.flag){
					
					#ifdef DEBUG
					LOG_I("flagCommunication.bufferPacket : %s\r\n", LOG_PUSH(flagCommunication.bufferPacket));
					#endif
					
					#ifdef DEBUG
					switch( releasePacket( flagCommunication.bufferPacket ) ){
						case Packet_Ok:
							LOG_I("Packet_Ok\r\n");
						
							packet = getReceivePacket();
							#ifdef DEBUG
							printPacket(packet);
							#endif
                            
						if(APP.net.established){										
								switch(packet->sensor){
									case Th:
										pkt_build(PKT_TYPE_SNSR_DATA_RES,  packet->data, flagSensor.pin);
									break;
									case Pressure:
									case Light:
									case Button:
									case Human:
									case Sound:
										sprintf(data, "%d", packet->data[0]); //TODO: DATA to String.
										pkt_build(PKT_TYPE_SNSR_ACT_REQ,  data, flagSensor.pin);
									default: 
										#ifdef DEBUG
										LOG_I("packet->sensor Error!\r\n");
										#endif
								}
							}
						break;
						case Packet_NULL:
							LOG_I("Packet_NULL\r\n");
						break;
						case Packet_Length_ERROR:
							LOG_I("Packet_Length_ERROR\r\n");
						break;
						case Packet_Mode_ERROR:
							LOG_I("Packet_Mode_ERROR\r\n");
						break;
						case Packet_Sensor_ERROR:
							LOG_I("Packet_Sensor_ERROR\r\n");
						break;
						case Packet_Id_ERROR:
							LOG_I("Packet_Id_ERROR\r\n");
						break;
						case Packet_Data_ERROR:
							LOG_I("Packet_Data_ERROR\r\n");
						break;
						case Packet_Dropout:
							LOG_I("Packet_Dropout\r\n");
						break;
					}
					#endif
					
					flagCommunication.flag = false;
				}
				
				if(flagCommand_Sensor.flag){
					
					Send_Packet_Polling(Command, getSensor_Channel(flagCommand_Sensor.pin), flagCommand_Sensor.pin, flagCommand_Sensor.bufferData);
					flagCommand_Sensor.flag = false;
					
					nrf_delay_ms(100); //TODO: Mabye Conflict with FSM															
					pkt_build(PKT_TYPE_SNSR_CMD_RES, 0, flagCommand_Sensor.pin);
					nrf_delay_ms(100);
				}
        
        pkt_send(&m_cen_s);
				
        if (NRF_LOG_PROCESS() == false) {
            power_manage();
			}
		}
}

