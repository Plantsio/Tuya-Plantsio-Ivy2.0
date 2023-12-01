/**
 * @file tkl_wifi.c
 * @brief wifi操作接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi_types.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "esp_netif_defaults.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
#include "tkl_system.h"
#include "tkl_memory.h"
#include "tkl_wifi.h"
#include "tkl_output.h"

static NW_IP_S wifi_sta_ip_addrs;
static WF_STATION_STAT_E wifi_sta_conn_status = WSS_IDLE;
static WIFI_EVENT_CB tkl_event_cb = NULL;
static WF_WK_MD_E g_wifi_work_mode = WWM_UNKNOWN;
static SNIFFER_CALLBACK tuya_sniffer_cb = NULL;
static WIFI_REV_MGNT_CB mgnt_recv_cb = NULL;

static wifi_country_t cc_tbls[] = {
    {.cc = "CN", .schan = 1, .nchan = 13, .policy = WIFI_COUNTRY_POLICY_MANUAL },
    {.cc = "US", .schan = 1, .nchan = 11, .policy = WIFI_COUNTRY_POLICY_MANUAL },
    {.cc = "JP", .schan = 1, .nchan = 14, .policy = WIFI_COUNTRY_POLICY_MANUAL },
    {.cc = "EU", .schan = 1, .nchan = 11, .policy = WIFI_COUNTRY_POLICY_MANUAL },
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define TKL_WIFI_STA_IP_ADDRS_INIT(ip) memset(&(ip), 0, sizeof(ip))

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#endif

#ifndef MACSTR
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define STORAGE_NAMESPACE "tuya.storage"

esp_netif_t *netif_sta = NULL;
esp_netif_t *netif_ap = NULL;
//static EventGroupHandle_t s_wifi_event_group;
//unsigned char s_wifi_reconn_retries = 0;
unsigned char s_wifi_sta_conne_evt_state = 0xFF;
unsigned char s_wifi_macaddr[6] = { 0 };

#define DBG_TAG "TKL_WIFI"

static int wifi_sniffer_start(wifi_promiscuous_cb_t cb, wifi_promiscuous_filter_t *filter)
{
    esp_wifi_set_promiscuous_filter(filter);
    esp_wifi_set_promiscuous_rx_cb(cb);
    esp_wifi_set_promiscuous(true);
    return OPRT_OK;
}

static int wifi_sniffer_stop(void)
{
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
    return OPRT_OK;
}
#if 0
static void wifi_promiscuous_mgnt_cb(uint8_t *data, uint16_t len, wifi_promiscuous_pkt_type_t type)
{
    assert(0);
    assert(NULL != data);
    if (mgnt_recv_cb) {
        mgnt_recv_cb(data, len);
    }
}
#endif
static void sniffer_local_cb(const uint8_t *buf, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *sniffer_pkt = (wifi_promiscuous_pkt_t *)buf;
    uint8_t *data = sniffer_pkt->payload;
    unsigned int len = sniffer_pkt->rx_ctrl.sig_len;

    if (tuya_sniffer_cb) {
        //ESP_LOGI(DBG_TAG,"%s: sniffer frame data %p len %u rssi %d", __func__, data, len, sniffer_pkt->rx_ctrl.rssi);
        tuya_sniffer_cb(data, len, sniffer_pkt->rx_ctrl.rssi);
    }

    // if (type == WIFI_PKT_MGMT) {
	//     wifi_promiscuous_mgnt_cb(data, len, type);
    // }
}

static void tkl_wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    unsigned short ssid_num = 0;
    esp_err_t ret = ESP_OK;    
    char *ip, *mask, *gw;
    ip_event_got_ip_t *ip_event;
    wifi_event_sta_disconnected_t* disconnected;
    //wifi_event_ap_staconnected_t* ap_event = NULL;
    //ESP_LOGI(TAG, "%s: event_base %d event_id %ld", __func__, (int)event_base, event_id);
 
    if (WIFI_EVENT == event_base && WIFI_EVENT_STA_START == event_id) {
        //ESP_LOGI(DBG_TAG, "%s: recv event WIFI_EVENT_STA_START", __func__);
        ret = esp_wifi_connect();
        if (ESP_OK != ret) {
            ESP_LOGE(DBG_TAG, "%s: call esp_wifi_connect failed(ret=0x%x)", __func__, ret);
        } else {
            wifi_sta_conn_status = WSS_CONNECTING;
        }
    } else if (WIFI_EVENT == event_base && WIFI_EVENT_STA_CONNECTED == event_id) {
        //ESP_LOGI(DBG_TAG, "%s: recv event WIFI_EVENT_STA_CONNECTED", __func__);
        wifi_sta_conn_status = WSS_CONN_SUCCESS;
    } else if (WIFI_EVENT == event_base && WIFI_EVENT_STA_DISCONNECTED == event_id) {
        disconnected = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGI(DBG_TAG, "%s: WIFI_EVENT_STA_DISCONNECTED Disconnect reason : %d", __func__, disconnected->reason);

        switch (disconnected->reason) {
        case WIFI_REASON_NO_AP_FOUND:
            wifi_sta_conn_status = WSS_NO_AP_FOUND;
            break;
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            wifi_sta_conn_status = WSS_PASSWD_WRONG;
            break;
        default:
            wifi_sta_conn_status = WSS_CONN_FAIL;
            break;
        }
        
        TKL_WIFI_STA_IP_ADDRS_INIT(wifi_sta_ip_addrs);

        if (s_wifi_sta_conne_evt_state == WFE_CONNECTED) {
            //ESP_LOGI(DBG_TAG, "%s: send WFE_DISCONNECTED event", __func__);
            //esp_wifi_stop() ???
            if (tkl_event_cb) {
                tkl_event_cb(WFE_DISCONNECTED, NULL);
            }
            s_wifi_sta_conne_evt_state = WFE_DISCONNECTED;
        } else {
            if (tkl_event_cb) {
                tkl_event_cb(WFE_CONNECT_FAILED, NULL);
            }
            s_wifi_sta_conne_evt_state = WFE_CONNECT_FAILED;
        }
        //ESP_LOGI(DBG_TAG, "%s:%d s_wifi_reconn_retries %d", __func__, __LINE__, s_wifi_reconn_retries);
        //if (s_wifi_reconn_retries < 5) {
        //    esp_wifi_connect();
            //ESP_LOGE(DBG_TAG, "%s:%d esp_wifi_connect ret = %d", __func__, __LINE__, ret);
        //    s_wifi_reconn_retries++;
        //}
    } else if (WIFI_EVENT == event_base && WIFI_EVENT_SCAN_DONE == event_id) {
        esp_wifi_scan_get_ap_num(&ssid_num);
        if (!ssid_num) {
            wifi_sta_conn_status = WSS_NO_AP_FOUND;
            TKL_WIFI_STA_IP_ADDRS_INIT(wifi_sta_ip_addrs);
            if (tkl_event_cb && s_wifi_sta_conne_evt_state != WFE_CONNECT_FAILED) {
                tkl_event_cb(WFE_CONNECT_FAILED, NULL);
            }
        }
        //ESP_LOGI(DBG_TAG, "%s: recv event WIFI_EVENT_SCAN_DONE ssid_num %d", __func__, ssid_num);
    } else if (WIFI_EVENT == event_base && WIFI_EVENT_STA_AUTHMODE_CHANGE == event_id) {    
        //ESP_LOGI(DBG_TAG, "%s: recv event WIFI_EVENT_STA_AUTHMODE_CHANGE", __func__);
        if (tkl_event_cb && s_wifi_sta_conne_evt_state != WFE_DISCONNECTED) {
            tkl_event_cb(WFE_DISCONNECTED, NULL);
            s_wifi_sta_conne_evt_state = WFE_DISCONNECTED;
            wifi_sta_conn_status = WSS_CONN_FAIL;
            TKL_WIFI_STA_IP_ADDRS_INIT(wifi_sta_ip_addrs);
        }
    //} else if (WIFI_EVENT == event_base && WIFI_EVENT_STA_STOP == event_id) { 
    //    ESP_LOGI(DBG_TAG, "%s: recv event WIFI_EVENT_STA_STOP", __func__);
    } else if (WIFI_EVENT == event_base && WIFI_EVENT_STA_BSS_RSSI_LOW == event_id) {    
        //ESP_LOGI(DBG_TAG, "%s: recv event WIFI_EVENT_STA_BSS_RSSI_LOW", __func__);
    } else if (WIFI_EVENT == event_base && WIFI_EVENT_ACTION_TX_STATUS == event_id) {
        //ESP_LOGI(DBG_TAG, "%s: recv event WIFI_EVENT_ACTION_TX_STATUS", __func__);
    } else if (WIFI_EVENT == event_base && WIFI_EVENT_ROC_DONE == event_id) {
        //ESP_LOGI(DBG_TAG, "%s: recv event WIFI_EVENT_ROC_DONE", __func__);
    } else if (WIFI_EVENT == event_base && WIFI_EVENT_STA_BEACON_TIMEOUT == event_id) {
        //ESP_LOGI(DBG_TAG, "%s: recv event WIFI_EVENT_STA_BEACON_TIMEOUT", __func__);
        if (tkl_event_cb && s_wifi_sta_conne_evt_state != WFE_DISCONNECTED) {
            tkl_event_cb(WFE_DISCONNECTED, NULL);
            s_wifi_sta_conne_evt_state = WFE_DISCONNECTED;
            wifi_sta_conn_status = WSS_CONN_FAIL;
            TKL_WIFI_STA_IP_ADDRS_INIT(wifi_sta_ip_addrs);
        }
    //} else if (WIFI_EVENT == event_base && WIFI_EVENT_CONNECTIONLESS_MODULE_WAKE_INTERVAL_START == event_id) {      
        //ESP_LOGI(DBG_TAG, "%s: recv event WIFI_EVENT_CONNECTIONLESS_MODULE_WAKE_INTERVAL_START", __func__);
    } else if (IP_EVENT == event_base && IP_EVENT_STA_GOT_IP == event_id) {
        ip_event = (ip_event_got_ip_t *)event_data;

        ip = esp_ip4addr_ntoa(&ip_event->ip_info.ip, wifi_sta_ip_addrs.ip, sizeof(wifi_sta_ip_addrs.ip));
        mask = esp_ip4addr_ntoa(&ip_event->ip_info.netmask, wifi_sta_ip_addrs.mask, sizeof(wifi_sta_ip_addrs.mask));
        gw = esp_ip4addr_ntoa(&ip_event->ip_info.gw, wifi_sta_ip_addrs.gw, sizeof(wifi_sta_ip_addrs.gw));
        //ESP_LOGI(DBG_TAG, "%s: IP_EVENT_STA_GOT_IP ip %s mask %s gw %s", __func__, ip, mask, gw);
        //xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        if (ip && mask && gw && tkl_event_cb) {
            wifi_sta_conn_status = WSS_GOT_IP;
            //ESP_LOGI(DBG_TAG, "%s: send WFE_CONNECTED event", __func__);
            tkl_event_cb(WFE_CONNECTED, NULL);
            s_wifi_sta_conne_evt_state = WFE_CONNECTED;
        }
        //s_wifi_reconn_retries = 0;
    } else if (WIFI_EVENT == event_base && WIFI_EVENT_AP_STACONNECTED == event_id) {
        //ESP_LOGI(DBG_TAG, "%s: WIFI_EVENT_AP_STACONNECTED", __func__);
        //ap_event = (wifi_event_ap_staconnected_t *)event_data;
    } else if (WIFI_EVENT == event_base && WIFI_EVENT_AP_STADISCONNECTED == event_id) {
        //ESP_LOGI(DBG_TAG, "%s: WIFI_EVENT_AP_STADISCONNECTED", __func__);
        //ap_event = (wifi_event_ap_staconnected_t *)event_data;
    } else {
        //ESP_LOGI(DBG_TAG, "%s: unsupport event_id %d", __func__, event_id);
    }
}

OPERATE_RET _tkl_wifi_all_ap_scan(SCHAR_T *ssid, AP_IF_S **ap_ary, UINT_T *num)
{
    int i, total;
    uint16_t numbers = 0;
    AP_IF_S* ap_items = NULL;
    wifi_scan_config_t scan_cfg;
    wifi_ap_record_t *esp_scan_results;

    assert(NULL != ap_ary);
    assert(NULL != num);
    
    *num = 0;
    *ap_ary = NULL;

    memset(&scan_cfg, 0, sizeof(scan_cfg));
    scan_cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;

    if (ESP_OK != esp_wifi_scan_start(&scan_cfg, TRUE)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_scan_start failed", __func__);
        return OPRT_COM_ERROR;
    }

    if (ESP_OK != esp_wifi_scan_get_ap_num(&numbers)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_scan_get_ap_num failed", __func__);
        return OPRT_COM_ERROR;
    }

    esp_scan_results = (wifi_ap_record_t *)tkl_system_malloc(numbers * sizeof(wifi_ap_record_t));
    if (NULL == esp_scan_results) {
        //ESP_LOGE(DBG_TAG, "%s: malloc memory failed", __func__);
        return OPRT_COM_ERROR;
    }

    memset(esp_scan_results, 0, numbers * sizeof(wifi_ap_record_t));
    if (ESP_OK != esp_wifi_scan_get_ap_records(&numbers, esp_scan_results)) {
        tkl_system_free(esp_scan_results);
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_scan_get_ap_records failed", __func__);
        return OPRT_COM_ERROR;
    }

    ap_items = (AP_IF_S *)tkl_system_malloc(numbers * sizeof(AP_IF_S));
    if (NULL == ap_items) {
        tkl_system_free(esp_scan_results);
        //ESP_LOGE(DBG_TAG, "%s: malloc memory for ap info failed", __func__);
        return OPRT_COM_ERROR;
    }

    memset(ap_items, 0, numbers * sizeof(AP_IF_S));
    for (i = 0, total = 0; i < numbers; i++) {
        if (ssid && !strncmp((char *)ssid, (char *)esp_scan_results[i].ssid, strlen((char *)ssid))) {
            continue;
        }
        ap_items[i].channel = esp_scan_results[i].primary;
        ap_items[i].rssi = esp_scan_results[i].rssi;
        ap_items[i].security = esp_scan_results[i].authmode;
        ap_items[i].s_len = strlen((char *)esp_scan_results[i].ssid);
        memcpy(ap_items[i].ssid, esp_scan_results[i].ssid, ap_items[i].s_len);
        memcpy(ap_items[i].bssid, esp_scan_results[i].bssid, 6);
        total++;
        //ESP_LOGE(DBG_TAG, "%s: ssid %s bssid "MACSTR" chan %d sec %d rssi %d",
        //    __func__, ap_items[i].ssid, MAC2STR(ap_items[i].bssid), ap_items[i].channel, 
        //    ap_items[i].security, ap_items[i].rssi);
    }

    *ap_ary = ap_items;
    *num = total;

    tkl_system_free(esp_scan_results);
    return OPRT_OK;
}

/**
 * @brief set wifi interface work channel
 * 
 * @param[in]       chan        the channel to set
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_set_cur_channel(CONST UCHAR_T chan)
{
    if (ESP_OK != esp_wifi_set_channel(chan, WIFI_SECOND_CHAN_NONE)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_set_channel set channel %d failed", __func__, chan);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

/**
 * @brief get wifi interface work channel
 * 
 * @param[out]      chan        the channel wifi works
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_get_cur_channel(UCHAR_T *chan)
{
    wifi_second_chan_t second;
    if (ESP_OK != esp_wifi_get_channel(chan, &second)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_get_channel failed", __func__);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

/**
 * @brief enable / disable wifi sniffer mode.
 *        if wifi sniffer mode is enabled, wifi recv from
 *        packages from the air, and user shoud send these
 *        packages to tuya-sdk with callback <cb>.
 * 
 * @param[in]       en          enable or disable
 * @param[in]       cb          notify callback
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_set_sniffer(CONST BOOL_T en, CONST SNIFFER_CALLBACK cb)
{
    if(en && (NULL == cb)) {
        return OPRT_INVALID_PARM;
    }
 
    wifi_promiscuous_filter_t filter;
    memset(&filter, 0, sizeof(filter));
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA | WIFI_PROMIS_FILTER_MASK_MGMT;

    //ESP_LOGI(DBG_TAG, "%s: set sniffer en %d cb %p", __func__, en, cb);
    if (en) {
        tuya_sniffer_cb = cb;
        wifi_sniffer_start((wifi_promiscuous_cb_t)sniffer_local_cb, &filter);
    } else {
        //if (NULL != mgnt_recv_cb) {
            wifi_sniffer_stop();
        //}
        tuya_sniffer_cb = NULL;
    }

    return OPRT_OK;
}

/**
 * @brief get wifi mac info.when wifi works in
 *        ap+station mode, wifi has two macs.
 * 
 * @param[in]       wf          wifi function type
 * @param[out]      mac         the mac info
 * @return  OPRT_OK: success  Other: fail
 */
 
int tkl_wifi_get_mac(const WF_IF_E wf, NW_MAC_S *mac)
{
    assert(NULL != mac);

    if (ESP_OK != esp_wifi_get_mac(WF_AP == wf ? ESP_IF_WIFI_AP : ESP_IF_WIFI_STA, mac->mac)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_get_mac failed(wf=%d)", __func__, wf);
        return OPRT_COM_ERROR;
    }

    //ESP_LOGI(DBG_TAG, "%s: wf %d mac="MACSTR, __func__, wf, MAC2STR(mac->mac));
    return OPRT_OK;
}

/**
 * @brief set wifi mac info.when wifi works in
 *        ap+station mode, wifi has two macs.
 * 
 * @param[in]       wf          wifi function type
 * @param[in]       mac         the mac info
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_set_mac(CONST WF_IF_E wf, CONST NW_MAC_S *mac)
{
    int ret;
    nvs_handle_t nvs_handle;
    assert(NULL != mac);
    //ESP_LOGI(DBG_TAG, "%s: wf %d mac="MACSTR, __func__, wf, MAC2STR(mac->mac));

    ret = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ESP_OK != ret) {
        //ESP_LOGE(DBG_TAG, "%s: call nvs_open failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    ret = nvs_set_blob(nvs_handle, "sta.mac", mac->mac, sizeof(mac->mac));
    if (ESP_OK != ret) {
        //ESP_LOGE(DBG_TAG, "%s: call nvs_set_blob failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    ret = nvs_commit(nvs_handle);
    if (ESP_OK != ret) {
        //ESP_LOGE(DBG_TAG, "%s: call nvs_commit failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    nvs_close(nvs_handle);

    ret = esp_wifi_set_mode((WF_AP == wf) ? WIFI_MODE_AP : WIFI_MODE_STA);
    if (ESP_OK != ret) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_set_mode failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    ret = esp_base_mac_addr_set((const unsigned char*)mac->mac);
    if (ESP_OK != ret) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_base_mac_addr_set failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    ret = esp_wifi_set_mac((WF_AP == wf) ? ESP_IF_WIFI_AP : ESP_IF_WIFI_STA, mac->mac);
    if (ESP_OK != ret) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_set_mac failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief set wifi work mode
 * 
 * @param[in]       mode        wifi work mode
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_set_work_mode(CONST WF_WK_MD_E mode)
{
    wifi_mode_t esp_wifi_mode = WIFI_MODE_MAX;

    assert(mode < WWM_UNKNOWN);
    //ESP_LOGI(DBG_TAG, "%s: mode %d", __func__, mode);

    if (mode == g_wifi_work_mode) {
        return OPRT_OK;
    }

    switch(mode) {
    case WWM_POWERDOWN:
        esp_wifi_mode = WIFI_MODE_NULL;
        break;
    case WWM_SNIFFER:
        esp_wifi_mode = WIFI_MODE_APSTA;
        break;
    case WWM_STATION:
        esp_wifi_mode = WIFI_MODE_STA;
        break;
    case WWM_SOFTAP:
        esp_wifi_mode = WIFI_MODE_AP;
        break;
    case WWM_STATIONAP:
        esp_wifi_mode = WIFI_MODE_APSTA;
        break;
    default:
        return OPRT_COM_ERROR;                    
    }

    if (WWM_SNIFFER != mode && ESP_OK != esp_wifi_set_mode(esp_wifi_mode)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_set_mode set mode %d failed", __func__, mode);
        return OPRT_COM_ERROR;
    }

    if (WWM_POWERDOWN == mode) {
        // if (ESP_OK != esp_wifi_stop()) {
        //     ESP_LOGE(DBG_TAG, "%s: call esp_wifi_stop failed", __func__);
        //     return OPRT_COM_ERROR;
        // }
    } else {
        // if (ESP_OK != esp_wifi_start()) {
        //     ESP_LOGE(DBG_TAG, "%s: call esp_wifi_start failed", __func__);
        //     return OPRT_COM_ERROR;
        // }
    }

    g_wifi_work_mode = mode;
    return OPRT_OK;
}

/**
 * @brief get wifi work mode
 * 
 * @param[out]      mode        wifi work mode
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_get_work_mode(WF_WK_MD_E *mode)
{
    wifi_mode_t esp_wifi_mode = WIFI_MODE_MAX;
    assert(NULL != mode);

    if (ESP_OK != esp_wifi_get_mode(&esp_wifi_mode)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_get_mode failed", __func__);
        return OPRT_COM_ERROR;
    }

    //ESP_LOGI(DBG_TAG, "%s: esp_wifi_mode %d g_wifi_work_mode %d", __func__, esp_wifi_mode, g_wifi_work_mode);
    *mode = g_wifi_work_mode;
    return OPRT_OK;
}

/**
 * @brief set wifi country code
 * 
 * @param[in]       ccode  country code
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_set_country_code(CONST COUNTRY_CODE_E ccode)
{
    int i;
    char country[32] = { 0 };
    wifi_country_t *ptr = NULL;

    //ESP_LOGI(DBG_TAG, "%s: ccode %d", __func__, ccode);
    
    switch (ccode) {
    case COUNTRY_CODE_CN:
    {
        strncpy(country, "CN", sizeof(country));
        break;
    }
    case COUNTRY_CODE_US:
    {
        strncpy(country, "US", sizeof(country));
        break;
    }
    case COUNTRY_CODE_JP:
    {
        strncpy(country, "JP", sizeof(country));
        break;
    }
    case COUNTRY_CODE_EU:
    {
        strncpy(country, "AL", sizeof(country));
        break;
    }
    default:
        return OPRT_NOT_SUPPORTED;
    }

    for (i = 0; i < ARRAY_SIZE(cc_tbls); i++) {
        ptr = &cc_tbls[i];
        if (!strncmp(ptr->cc, country, strlen(ptr->cc))) {
            break;
        }
    }

    if (i == ARRAY_SIZE(cc_tbls)) {
        return OPRT_NOT_SUPPORTED;
    }

    if (ESP_OK != esp_wifi_set_country(ptr)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_set_country failed", __func__);
        return OPRT_COM_ERROR;
    }
 
    return OPRT_OK;
}

/**
 * @brief send wifi management
 * 
 * @param[in]       buf         pointer to buffer
 * @param[in]       len         length of buffer
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_send_mgnt(CONST UCHAR_T *buf, CONST UINT_T len)
{
    esp_err_t ret;
    wifi_interface_t vif_idx = WIFI_IF_STA;
    wifi_mode_t esp_wifi_mode = WIFI_MODE_MAX;

    assert(len);
    assert(NULL != buf);

    if (ESP_OK != esp_wifi_get_mode(&esp_wifi_mode)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_get_mode failed", __func__);
        return OPRT_COM_ERROR;
    }

    if (WIFI_MODE_AP == esp_wifi_mode) {
        vif_idx = WIFI_IF_AP;
    } else if (WIFI_MODE_STA == esp_wifi_mode) {
        vif_idx = WIFI_IF_STA;
    } else if (WIFI_MODE_APSTA == esp_wifi_mode) {
        vif_idx = WIFI_IF_STA;
    } else {
        //ESP_LOGE(DBG_TAG, "%s: invalid esp_wifi_mode %d", __func__, esp_wifi_mode);
        return OPRT_COM_ERROR; 
    }

    ret = esp_wifi_80211_tx(vif_idx, (const void *)buf, len, false);
    if (ESP_OK != ret) {
        //ESP_LOGI(DBG_TAG, "%s: call esp_wifi_80211_tx failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief register receive wifi management callback
 * 
 * @param[in]       enable
 * @param[in]       recv_cb     receive callback
 * @return  OPRT_OK: success  Other: fail
 */
OPERATE_RET tkl_wifi_register_recv_mgnt_callback(CONST BOOL_T enable, CONST WIFI_REV_MGNT_CB recv_cb)
{
    if (enable) {
        mgnt_recv_cb = recv_cb;
		
        wifi_promiscuous_filter_t filter;
        memset(&filter, 0, sizeof(filter));
        filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA | WIFI_PROMIS_FILTER_MASK_MGMT;

        wifi_sniffer_start((wifi_promiscuous_cb_t)sniffer_local_cb, &filter);
    } else {
        if (NULL != tuya_sniffer_cb) {
            wifi_sniffer_stop();
        }
        mgnt_recv_cb = NULL;
    }
    
    return OPRT_OK;
}

/**
 * @brief set wifi lowerpower mode
 * 
 * @param[in]       en
 * @param[in]       dtim
 * @return  OPRT_OK: success  Other: fail
 */
int tkl_wifi_set_lp_mode(const BOOL_T en, const unsigned char dtim)
{
    return OPRT_OK;
}

/**
 * @brief get wifi rf param exist or not
 * 
 * @param[in]       none
 * @return  true: rf param exist  Other: fail
 */
BOOL_T tkl_wifi_set_rf_calibrated(VOID_T)
{
    return TRUE;
}

OPERATE_RET tkl_wifi_init(WIFI_EVENT_CB cb)
{
    size_t size;
    esp_err_t ret;
    nvs_handle_t nvs_handle = 0;
    unsigned char base_mac[6] = { 0 };
    esp_netif_config_t cfg_sta;
    esp_netif_config_t cfg_ap;
    esp_netif_inherent_config_t netif_cfg;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    assert(NULL != cb);
    //ESP_LOGI(DBG_TAG, "==%s: cb %p==", __func__, cb);

    if (ESP_OK == (ret = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle))) {
        //ESP_LOGI(DBG_TAG, "%s: call nvs_open sucess", __func__);
        size = sizeof(base_mac);
        if (ESP_OK == (ret = nvs_get_blob(nvs_handle, "sta.mac", base_mac, &size))) {
            //ESP_LOGI(DBG_TAG, "%s: call nvs_get_blob sucess macaddr "MACSTR, __func__, MAC2STR(base_mac));
        } else {
            //ESP_LOGI(DBG_TAG, "%s: call nvs_get_blob failed(ret=%d)", __func__, ret);
            esp_efuse_mac_get_default(base_mac);
        }
    } else {
        //ESP_LOGE(DBG_TAG, "%s: call nvs_open failed(ret=%d)", __func__, ret);
        esp_efuse_mac_get_default(base_mac);
    }

    //ESP_LOGE(DBG_TAG, "%s:%d set base mac address "MACSTR" nvs_handle %lu", __func__, __LINE__, MAC2STR(base_mac), nvs_handle);
    ret = esp_base_mac_addr_set(base_mac);
    if (ESP_OK != ret) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_base_mac_addr_set failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    nvs_close(nvs_handle);

    if (ESP_OK != esp_netif_init()) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_netif_init failed", __func__);
        return OPRT_COM_ERROR;
    }

    //ESP_LOGI(DBG_TAG, "%s:%d nvs_enable %d", __func__, __LINE__, cfg.nvs_enable);

    if (ESP_OK != esp_wifi_init(&cfg)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_init failed", __func__);
        return OPRT_COM_ERROR;
    }

    if (ESP_OK != esp_wifi_restore()) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_restore failed", __func__);
    }

    if (ESP_OK != esp_wifi_set_storage(WIFI_STORAGE_RAM)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_set_storage failed", __func__);
    }

    memset(&cfg_sta, 0, sizeof(cfg_sta));
    memcpy(&netif_cfg, ESP_NETIF_BASE_DEFAULT_WIFI_STA, sizeof(netif_cfg));
    cfg_sta.base = &netif_cfg;
    cfg_sta.stack = ESP_NETIF_NETSTACK_DEFAULT_WIFI_STA;

    if (NULL == netif_sta) {
        //ESP_LOGI(DBG_TAG, "%s: ====before call esp_netif_new====", __func__);
        netif_sta = esp_netif_new(&cfg_sta);
        if (NULL == netif_sta) {
            //ESP_LOGE(DBG_TAG, "%s: call esp_netif_new failed", __func__);
            return OPRT_COM_ERROR;
        }
        //ESP_LOGI(DBG_TAG, "%s: ====end call esp_netif_new==== netif_sta %p", __func__, netif_sta);
    }

    //netif_sta->route_prio = 128;
    if (ESP_OK != esp_netif_attach_wifi_station(netif_sta)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_netif_attach_wifi_station failed", __func__);
        return OPRT_COM_ERROR;
    }

    memset(&cfg_ap, 0, sizeof(cfg_ap));
    memcpy(&netif_cfg, ESP_NETIF_BASE_DEFAULT_WIFI_AP, sizeof(netif_cfg));
    cfg_ap.base = &netif_cfg;
    cfg_ap.stack = ESP_NETIF_NETSTACK_DEFAULT_WIFI_AP;

    if (NULL == netif_ap) {
        netif_ap = esp_netif_new(&cfg_ap);
        if (NULL == netif_ap) {
            //ESP_LOGE(DBG_TAG, "%s: call esp_netif_new failed", __func__);
            return OPRT_COM_ERROR;
        }
    }

    if (ESP_OK != esp_netif_attach_wifi_ap(netif_ap)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_netif_attach_wifi_ap failed", __func__);
        return OPRT_COM_ERROR;
    }

    if (ESP_OK != esp_event_loop_create_default()) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_event_loop_create_default failed", __func__);
        return OPRT_COM_ERROR;
    }
 
    if (ESP_OK != esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &tkl_wifi_event_handler, NULL)) {
        //ESP_LOGE(DBG_TAG, "%s: register WIFI_EVENT_STA_START failed", __func__);
        return OPRT_COM_ERROR;
    }
    if (ESP_OK != esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_STOP, &tkl_wifi_event_handler, NULL)) {
        //ESP_LOGE(DBG_TAG, "%s: register WIFI_EVENT_STA_CONNECTED failed", __func__);
        return OPRT_COM_ERROR;
    }
    if (ESP_OK != esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &tkl_wifi_event_handler, NULL)) {
        //ESP_LOGE(DBG_TAG, "%s: register WIFI_EVENT_STA_CONNECTED failed", __func__);
        return OPRT_COM_ERROR;
    }
    if (ESP_OK != esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &tkl_wifi_event_handler, NULL)) {
        //ESP_LOGE(DBG_TAG, "%s: register WIFI_EVENT_STA_CONNECTED failed", __func__);
        return OPRT_COM_ERROR;
    }
    if (ESP_OK != esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_AUTHMODE_CHANGE, &tkl_wifi_event_handler, NULL)) {
        //ESP_LOGE(DBG_TAG, "%s: register WIFI_EVENT_STA_CONNECTED failed", __func__);
        return OPRT_COM_ERROR;
    }
    if (ESP_OK != esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_BSS_RSSI_LOW, &tkl_wifi_event_handler, NULL)) {
        //ESP_LOGE(DBG_TAG, "%s: register WIFI_EVENT_STA_CONNECTED failed", __func__);
        return OPRT_COM_ERROR;
    }
    if (ESP_OK != esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_ACTION_TX_STATUS, &tkl_wifi_event_handler, NULL)) {
        //ESP_LOGE(DBG_TAG, "%s: register WIFI_EVENT_STA_CONNECTED failed", __func__);
        return OPRT_COM_ERROR;
    }
    if (ESP_OK != esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_ROC_DONE, &tkl_wifi_event_handler, NULL)) {
        //ESP_LOGE(DBG_TAG, "%s: register WIFI_EVENT_STA_CONNECTED failed", __func__);
        return OPRT_COM_ERROR;
    }
    if (ESP_OK != esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_BEACON_TIMEOUT, &tkl_wifi_event_handler, NULL)) {
        //ESP_LOGE(DBG_TAG, "%s: register WIFI_EVENT_STA_CONNECTED failed", __func__);
        return OPRT_COM_ERROR;
    }
    //if (ESP_OK != esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_CONNECTIONLESS_MODULE_WAKE_INTERVAL_START, &tkl_wifi_event_handler, NULL)) {
        //ESP_LOGE(DBG_TAG, "%s: register WIFI_EVENT_STA_CONNECTED failed", __func__);
        //return OPRT_COM_ERROR;
    //}
  
    if (ESP_OK != esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &tkl_wifi_event_handler, NULL)) {
        //ESP_LOGE(DBG_TAG, "%s: register WIFI_EVENT_STA_DISCONNECTED failed", __func__);
        return OPRT_COM_ERROR;
    }
    if (ESP_OK != esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &tkl_wifi_event_handler, NULL)) {
        //ESP_LOGE(DBG_TAG, "%s: register IP_EVENT_STA_GOT_IP failed", __func__);
        return OPRT_COM_ERROR;
    }
  
    TKL_WIFI_STA_IP_ADDRS_INIT(wifi_sta_ip_addrs);
    tkl_event_cb = cb;

    return OPRT_OK;
}

OPERATE_RET tkl_wifi_ioctl(WF_IOCTL_CMD_E cmd,  VOID *args)
{
    return OPRT_NOT_SUPPORTED;
}

//#if !defined(TUYA_HOSTAPD_SUPPORT) || (TUYA_HOSTAPD_SUPPORT == 0)
/**
 * @brief get wifi bssid
 * 
 * @param[out]      mac         uplink mac
 * @return  OPRT_OK: success  Other: fail
 */
int tkl_wifi_get_bssid(unsigned char *mac)
{
    assert(NULL != mac);
    return OPRT_OK;
}

OPERATE_RET tkl_wifi_station_connect(CONST SCHAR_T *ssid, CONST SCHAR_T *passwd)
{
    wifi_config_t wifi_cfg;

    assert(NULL != ssid);
    assert(NULL != passwd);

    if (ESP_OK != esp_wifi_stop()) {
        //ESP_LOGI(DBG_TAG, "%s: call esp_wifi_stop failed", __func__);
        //return OPRT_COM_ERROR;
    }

    memset(&wifi_cfg, 0, sizeof(wifi_cfg));
    wifi_cfg.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    strncpy((char *)wifi_cfg.sta.ssid, (char *)ssid, strlen((char *)ssid));
    if (passwd && passwd[0] != '\0') {
        strncpy((char *)wifi_cfg.sta.password, (char *)passwd, strlen((char *)passwd));
    }
    //wifi_cfg.sta.btm_enabled = 1;
    wifi_cfg.sta.listen_interval = 10;
    wifi_cfg.sta.pmf_cfg.capable = 1;
    //wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    //wifi_cfg.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    if (ESP_OK != esp_wifi_set_default_wifi_sta_handlers()) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_set_default_wifi_sta_handlers failed", __func__);
        return OPRT_COM_ERROR;
    }

    if (ESP_OK != esp_wifi_set_mode(WIFI_MODE_STA)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_set_mode failed", __func__);
        return OPRT_COM_ERROR;
    }
    if (ESP_OK != esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_set_config failed", __func__);
        return OPRT_COM_ERROR;
    }
    if (ESP_OK != esp_wifi_start()) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_start failed", __func__);
        return OPRT_COM_ERROR;
    }

#if 0
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI("TKL_WIFI", "connected to ap SSID:%s password:%s", wifi_cfg.sta.ssid, wifi_cfg.sta.password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI("TKL_WIFI", "Failed to connect to SSID:%s, password:%s", wifi_cfg.sta.ssid, wifi_cfg.sta.password);
    } else {
        ESP_LOGE("TKL_WIFI", "UNEXPECTED EVENT");
    }
#endif

    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    //ESP_LOGI(DBG_TAG, "%s: connected to ap SSID:%s password:%s", __func__, wifi_cfg.sta.ssid, wifi_cfg.sta.password);

    return OPRT_OK;
}

OPERATE_RET tkl_wifi_station_disconnect(VOID_T)
{
    esp_err_t ret;

    ret = esp_wifi_disconnect();
    if (ESP_OK != ret) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_disconnect failed(ret=%d)", __func__, ret);
        //return OPRT_COM_ERROR;
    }

    ret = esp_wifi_stop();
    if (ESP_OK != ret) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_wifi_stop failed(ret=%d)", __func__, ret);
        //return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_wifi_station_get_status(WF_STATION_STAT_E *stat)
{
    *stat = wifi_sta_conn_status;
    return OPRT_OK;
}

OPERATE_RET tkl_wifi_all_ap_scan(AP_IF_S **ap_ary, UINT_T *num)
{
    return _tkl_wifi_all_ap_scan(NULL, ap_ary, num);
}

OPERATE_RET tkl_wifi_get_ip(CONST WF_IF_E wf, NW_IP_S *ip)
{
    esp_netif_ip_info_t ip_info;
    assert(NULL != ip);

    if (WF_STATION == wf) {
        if (WSS_GOT_IP != wifi_sta_conn_status) {
            return OPRT_COM_ERROR;
        }
        strncpy(ip->ip, wifi_sta_ip_addrs.ip, sizeof(ip->ip));
        strncpy(ip->mask, wifi_sta_ip_addrs.mask, sizeof(ip->mask));
        strncpy(ip->gw, wifi_sta_ip_addrs.gw, sizeof(ip->gw));
        //ESP_LOGI(DBG_TAG, "%s: wf %d ip %s netmask %s gw %s", __func__, wf, ip->ip, ip->mask, ip->gw);
    } else if (WF_AP == wf && netif_ap && ESP_OK == esp_netif_get_ip_info(netif_ap, &ip_info)) {
        esp_ip4addr_ntoa(&ip_info.ip, ip->ip, sizeof(ip->ip));
        esp_ip4addr_ntoa(&ip_info.netmask, ip->mask, sizeof(ip->mask));
        esp_ip4addr_ntoa(&ip_info.gw, ip->gw, sizeof(ip->gw));
        //ESP_LOGI(DBG_TAG, "%s: wf %d ip %s netmask %s gw %s", __func__, wf, ip->ip, ip->mask, ip->gw);
    } else {
        return OPRT_NOT_SUPPORTED;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_wifi_release_ap(AP_IF_S *ap)
{
    assert(NULL != ap);
    tkl_system_free(ap);
    return OPRT_OK;
}

OPERATE_RET tkl_wifi_start_ap(CONST WF_AP_CFG_IF_S *cfg)
{
    wifi_config_t wifi_cfg;
    wifi_auth_mode_t auth_mode = WIFI_AUTH_OPEN;
    esp_netif_ip_info_t netif_ap_ip_info;

    assert(NULL != cfg);

    memset(&wifi_cfg, 0, sizeof(wifi_cfg));
    strncpy((char *)wifi_cfg.ap.ssid, (char *)cfg->ssid, cfg->s_len);
    if (cfg->p_len && cfg->p_len <= 64) {
        strncpy((char *)wifi_cfg.ap.password, (char *)cfg->passwd, cfg->p_len);
    }
    wifi_cfg.ap.channel = cfg->chan;
    wifi_cfg.ap.beacon_interval = cfg->ms_interval;
    wifi_cfg.ap.max_connection = cfg->max_conn;
    //wifi_cfg.ap.max_connection = 5;
    wifi_cfg.ap.ssid_hidden = cfg->ssid_hidden;
    if (cfg->p_len && cfg->passwd[0] != '\0') {
        switch (cfg->md) {
        case WAAM_OPEN:
            auth_mode = WIFI_AUTH_OPEN;
            break;
        case WAAM_WEP:
            auth_mode = WIFI_AUTH_WEP;
            break;
        case WAAM_WPA_PSK:
            auth_mode = WIFI_AUTH_WPA_PSK;
            break;
        case WAAM_WPA2_PSK:
            auth_mode = WIFI_AUTH_WPA2_PSK;
            break;
        case WAAM_WPA_WPA2_PSK:
            auth_mode = WIFI_AUTH_WPA_WPA2_PSK;
            break;
        case WAAM_WPA_WPA3_SAE:
            auth_mode = WIFI_AUTH_WPA2_WPA3_PSK;
            break;
        default:
            auth_mode = WIFI_AUTH_OPEN;
            break;                         
        }
    }
    wifi_cfg.ap.authmode = auth_mode;
  
    if (ESP_OK != esp_wifi_set_default_wifi_ap_handlers()) {
        //ESP_LOGI(DBG_TAG, "%s: call esp_wifi_set_default_wifi_ap_handlers failed", __func__);
        return OPRT_COM_ERROR;
    }
   
    if (ESP_OK != esp_wifi_set_mode(WIFI_MODE_AP)) {
        //ESP_LOGI(DBG_TAG, "%s: call esp_wifi_set_mode failed", __func__);
        return OPRT_COM_ERROR;
    }
    if (ESP_OK != esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_cfg)) {
        //ESP_LOGI(DBG_TAG, "%s: call esp_wifi_set_config failed", __func__);
        return OPRT_COM_ERROR;
    }
    if (ESP_OK != esp_wifi_start()) {
        //ESP_LOGI(DBG_TAG, "%s: call esp_wifi_start failed", __func__);
        return OPRT_COM_ERROR;
    }

    memset(&netif_ap_ip_info, 0, sizeof(netif_ap_ip_info));
    netif_ap_ip_info.ip.addr = esp_ip4addr_aton(cfg->ip.ip);
    netif_ap_ip_info.netmask.addr = esp_ip4addr_aton(cfg->ip.mask);
    netif_ap_ip_info.gw.addr = esp_ip4addr_aton(cfg->ip.gw);

    //ESP_LOGI(DBG_TAG, "%s: ip %lu netmask %lu gw %lu",
    //    __func__, netif_ap_ip_info.ip.addr, netif_ap_ip_info.netmask.addr, netif_ap_ip_info.gw.addr);

    if (ESP_OK != esp_netif_set_ip_info(netif_ap, &netif_ap_ip_info)) {
        //ESP_LOGI(DBG_TAG, "%s: call esp_netif_set_ip_info failed", __func__);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_wifi_stop_ap(VOID_T)
{
    //esp_err_t ret;
    
    //ESP_LOGI(DBG_TAG,"%s: enter", __func__);

    //ret = esp_wifi_deauth_sta(0);
    //if (ESP_OK != ret) {
    //    ESP_LOGI(DBG_TAG, "%s: call esp_wifi_deauth_sta failed(ret=%d)", __func__, ret);
        //return OPRT_COM_ERROR;
    //}

    //ret = esp_wifi_stop();
    //if (ESP_OK != ret) {
    //    ESP_LOGI(DBG_TAG, "%s: call esp_wifi_stop failed(ret=%d)", __func__, ret);
        //return OPRT_COM_ERROR;
    //}
    
    //if (NULL != netif_ap) {
    //    esp_netif_destroy(netif_ap);
    //    netif_ap = NULL;
    //}
    //if (NULL != netif_ap) {
    //    esp_netif_destroy_default_wifi(netif_ap);
    //    netif_ap = NULL;
    //}
    return OPRT_OK;
}

OPERATE_RET tkl_wifi_get_connected_ap_info(FAST_WF_CONNECTED_AP_INFO_T **fast_ap_info)
{
    // wifi_ap_record_t ap_info;
    // esp_wifi_sta_get_ap_info(&ap_info);
    return OPRT_OK;
}

OPERATE_RET tkl_wifi_scan_ap(CONST SCHAR_T *ssid, AP_IF_S **ap_ary, UINT_T *num)
{
    return _tkl_wifi_all_ap_scan((SCHAR_T *)ssid, ap_ary, num);
}

OPERATE_RET tkl_wifi_station_get_conn_ap_rssi(SCHAR_T *rssi)
{
    wifi_ap_record_t ap_info;

    assert(NULL != rssi);

    memset(&ap_info, 0, sizeof(ap_info));
    if (ESP_OK != esp_wifi_sta_get_ap_info(&ap_info)) {
        //ESP_LOGI(DBG_TAG, "%s: call esp_wifi_sta_get_ap_info failed", __func__);
        return OPRT_COM_ERROR;
    }

    *rssi = ap_info.rssi;
    return OPRT_OK;
}

OPERATE_RET tkl_wifi_station_fast_connect(CONST FAST_WF_CONNECTED_AP_INFO_T *fast_ap_info)
{
    return OPRT_NOT_SUPPORTED;
}
