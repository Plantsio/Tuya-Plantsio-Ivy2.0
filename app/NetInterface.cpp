//
// Created by Huwensong on 2022/11/23.
//

#include "NetInterface.h"
#include "Network/IvyNet.h"
#include "Engine/IvyEngine.h"
#include "Anim/drivers/LvglDriver.h"
#include "common/Sys.h"
#include "Body/Skin.h"
#include "IOT.h"
#include "tool/OTA.h"
#include "Network/plantsio_api.h"
#include "common/Boot.h"
#include "esp_wifi.h"
#include "UIProvTip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "tuya_iot_wifi_api.h"
#include "tuya_svc_timer_task.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "Anim/ui/tools.h"

#include "tuya_devos_netlink.h"


#define WIFI_CONNECT_MAXNUM_RETRY   6
#define WIFI_AP_FIND_MAXNUM_RETRY   16

#define WIFI_RSSI_THRESHOLD         -70

#define ARCH_OS_MS2TICK(ms)      ((ms)/portTICK_PERIOD_MS)
#define ARCH_OS_TICK2MS(tick)    ((tick)*portTICK_PERIOD_MS)

#define ARCH_OS_TIMER_OPT_BLOCK_MS_MAX        10*1000

#define ARCH_OS_WAIT_FOREVER                (0xFFFFFFFF)
#define ARCH_OS_NO_WAIT                    (0)
#define ARCH_OS_WAIT_MS2TICK(ms)            \
    ( ((ms) == ARCH_OS_WAIT_FOREVER) ? ARCH_OS_WAIT_FOREVER : (((ms)/portTICK_PERIOD_MS)+((((ms)%portTICK_PERIOD_MS)+portTICK_PERIOD_MS-1)/portTICK_PERIOD_MS)) )

#define IP_WAIT_TIME                1000         //ms

NetInterface::NetInterface() {
}

NetInterface &NetInterface::instance() {
    static std::shared_ptr<NetInterface> instance(new NetInterface());
    return *instance;
}

void NetInterface::net_prov_start() {
    IOT::instance().begin();
}

bool NetInterface::net_is_provisioned() {
    return !in_provision;
}

void NetInterface::net_prov_deinit() {
}

void NetInterface::net_prov_reset() {
    log_i("soft rebooting");
    LvglDriver::instance().set_locked(false);
    IvyAnim::instance().set_current_drv(IvyAnim::none);
//        IvyAnim::instance().exit();
    log_i("closing SD MMC");
    SD_MMC.end();
    Bck::instance().fade_to_end(false, true, 300);
    Screen::instance().fillScreen(TFT_BLACK);
    log_i("restarting");
    vTaskDelay(100);
    tuya_iot_wf_gw_reset();
}

void NetInterface::net_event_register() {
    WiFi.onEvent([this](system_event_id_t event, system_event_info_t info) { net_event_handler(event, info); });
}

void NetInterface::net_event_handler(system_event_id_t event, system_event_info_t info) {
    String ssid_scan;
    int32_t rssi_scan = 0;
    uint8_t sec_scan = 0;
    int32_t chan_scan = 0;

    uint8_t *BSSID_scan = nullptr;

    static int s_wifi_connect_retry_num = 0;
    static int s_ap_find_retry_num = 0;

    switch (event) {
        case SYSTEM_EVENT_SCAN_DONE:
            m_ap_num = info.scan_done.number;
            m_ap_list = (AP_IF_S *) malloc(SIZEOF(AP_IF_S) * m_ap_num);
            if (NULL == m_ap_list) {
                log_e("Malloc_error.");
            }
            for (int i = 0;
                 i < m_ap_num;
                 i++) {
                WiFi.getNetworkInfo(i, ssid_scan, sec_scan, rssi_scan, BSSID_scan, chan_scan);
                m_ap_list[i].channel = chan_scan;
                m_ap_list[i].rssi = rssi_scan;
                m_ap_list[i].bssid[0] = BSSID_scan[0];
                m_ap_list[i].bssid[1] = BSSID_scan[1];
                m_ap_list[i].bssid[2] = BSSID_scan[2];
                m_ap_list[i].bssid[3] = BSSID_scan[3];
                m_ap_list[i].bssid[4] = BSSID_scan[4];
                m_ap_list[i].bssid[5] = BSSID_scan[5];
                strncpy((char *) m_ap_list[i].ssid, ssid_scan.c_str(), WIFI_SSID_LEN + 1);
                m_ap_list[i].s_len = ssid_scan.length();
            }

            break;
        case SYSTEM_EVENT_STA_START: {
            log_i("WIFI STA Started");
//            int ret = esp_wifi_set_rssi_threshold(WIFI_RSSI_THRESHOLD);
//
//            if (ret) {
//                log_e("set rssi threshold failed %d", ret);
//            } else {
//                log_i("set roam rssi threshold as %d", WIFI_RSSI_THRESHOLD);
//            }
            break;
        }
        case SYSTEM_EVENT_STA_CONNECTED:
            log_i("WIFI STA CONNECTED");
            set_sta_status(WSS_CONN_SUCCESS);

//            if (!in_provision && ap_not_found) {
//                if (s_wifi_connect_retry_num >= WIFI_CONNECT_MAXNUM_RETRY ||
//                    s_ap_find_retry_num >= WIFI_AP_FIND_MAXNUM_RETRY)
//                    ui_text_tip(LANG(Lang::ui_tuya_tip_connect_succ), "");
//            }
            s_wifi_connect_retry_num = 0;
            s_ap_find_retry_num = 0;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            if (!check_condition(Sys::WAKE))
                break;
            log_i("WIFI Disconnect: ssid = %s,reason = %d", info.disconnected.ssid, info.disconnected.reason);
            set_sta_status(WSS_CONN_FAIL);

            IOT::instance().set_connection_status(false);

            switch (info.disconnected.reason) {
                case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                case WIFI_REASON_HANDSHAKE_TIMEOUT: {
                    // wifi password error. reconfig.
                    if (in_provision) {
                        set_prov_status(prov_pwd_error);
                    }
                    break;
                }
                case WIFI_REASON_NO_AP_FOUND: {
                    if (s_ap_find_retry_num < WIFI_AP_FIND_MAXNUM_RETRY) {
                        esp_wifi_connect();
                        s_ap_find_retry_num++;
                    } else if (s_ap_find_retry_num == WIFI_AP_FIND_MAXNUM_RETRY) {
                        if (in_provision)
                            set_prov_status(prov_ap_not_found);
//                            LvglDriver::instance().get_current_ui<UI::UIProvTip>()->ui_prov_set_status(UI::prov_ap_not_found);
                        else
                            ui_text_tip(LANG(Lang::ui_tuya_tip_connect_fail),
                                        LANG(Lang::ui_tuya_tip_reason_ap_not_found));
                        esp_wifi_connect();
                        s_ap_find_retry_num++;
                    } else {
                        ap_not_found = true;
                    }
                    break;
                }
                default: {
                    if (s_wifi_connect_retry_num < WIFI_CONNECT_MAXNUM_RETRY) {
                        esp_wifi_connect();
                        s_wifi_connect_retry_num++;
                        log_i("retry to connect to the AP: %d", s_wifi_connect_retry_num);
                    } else if (s_wifi_connect_retry_num == WIFI_CONNECT_MAXNUM_RETRY) {
                        if (in_provision)
                            set_prov_status(prov_wifi_conn_fail);
                        else
                            ui_text_tip(LANG(Lang::ui_tuya_tip_connect_fail), LANG(Lang::ui_tuya_tip_fail_reset));
                        esp_wifi_connect();
                        s_wifi_connect_retry_num++;
                    } else {

                    }
                }
                    break;

            }

            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            log_i("Got IP:" IPSTR, IP2STR(&info.got_ip.ip_info.ip));
            set_ip_info(info.got_ip.ip_info);
            set_sta_status(WSS_GOT_IP);
            IOT::instance().report_ip();

            Boot::got_ip_cb();
            Skin::instance().calibrate_all(1000);   /* fixme calibration results needs to be stored in nvs */
            if (in_provision) {
                set_prov_status(prov_wifi_connected);
            }
            break;
        case SYSTEM_EVENT_STA_LOST_IP:
            log_i("Lost IP:");
            break;
        case SYSTEM_EVENT_STA_BSS_RSSI_LOW:
            log_i("RSSI_LOW: rssi = %d, Please closer wifi", info.bss_rssi_low.rssi);
            if (in_provision)
                set_prov_status(prov_rssi_low);
            else
                ui_text_tip(LANG(Lang::ui_tuya_tip_signal_week), LANG(Lang::ui_tuya_tip_reason_week));
            break;
        default:
            break;
    }
}

void NetInterface::net_status_change_handler(IN CONST GW_WIFI_NW_STAT_E stat) {
    switch (stat) {
        case STAT_STA_CONN:
            log_i("STAT_STA_CONN");

            break;
        case STAT_STA_DISC:
            log_i("STAT_STA_DISCONNECT");
            break;
        case STAT_CLOUD_CONN:
        case STAT_AP_CLOUD_CONN:
            log_i("activated");
            if (in_provision) {
                set_prov_status(prov_activated);
            }
            Sys::set_condition(Sys::TIMESTAMP_ACQUIRED);
            Boot::activated_cb();

            IOT::instance().set_connection_status(true);
            IOT::instance().connect_cb();

            NetInterface::instance().set_in_provision(false);
            IvyBody::instance().register_button_tap();

            break;
        default:
            break;
    }
}

bool NetInterface::net_sync_geo_info() {
    auto &geo_info = Sys::get_geo_info();
    bool ret = false;
    auto lon = Prop::get<int>(Prop::longitude);
    auto lat = Prop::get<int>(Prop::latitude);
    uint32_t start_t = millis();
    double delta = 0;
    if (!lat && !lon) {
        ret = plantsio::get_geo_info(geo_info);
        delta = millis() - start_t;
    } else {
        geo_info.location.latitude = std::to_string(lat);
        geo_info.location.longitude = std::to_string(lon);
        ret = plantsio::get_geo_info_with_location(geo_info);
    }
    if (ret) {
        /* request successful, at least ip is retrieved */
        if (!geo_info.location.latitude.empty()) {
            /* geo info retrieval success */
            uint32_t timestamp;
            timestamp = (time_t) geo_info.timestamp + (int) delta / (2 * 1000);
            set_time_from_geo_info(geo_info, timestamp);
            Prop::set(Prop::start_sync_time, get_ymd(timestamp), false, false, true);
            if (Prop::get<int>(Prop::tutorial_complete) == 1) {
                /* set to current time */
                Prop::set(Prop::tutorial_complete, timestamp, true, true, true, true);
            }
            log_i("plantsio sync geo info: %s | %s | %s | %s | %d | %d", geo_info.public_ip.c_str(),
                  geo_info.location.city_district.c_str(), geo_info.location.latitude.c_str(),
                  geo_info.location.longitude.c_str(), geo_info.offset, geo_info.dst);
            Sys::set_condition(Sys::TIME_SYNCHRONIZED);
            return true;
        }
        log_w("ip %s is probably not found by cloud server", geo_info.public_ip.c_str());
    } else {
        log_w("plantsio server down");
    }
    return false;
}

bool NetInterface::net_connected() {
    return WiFi.isConnected();
}

void NetInterface::disable_wifi() {
    tuya_devos_netlink_stop();
}

void NetInterface::enable_wifi() {
    tuya_devos_netlink_start();
}

void NetInterface::set_in_provision(bool in) {
    in_provision = in;
    if (in) {
        set_prov_status(prov_init);
    }
}

uint16_t NetInterface::get_ap_num() {
    return m_ap_num;
}

AP_IF_S *NetInterface::get_ap_list() {
    return m_ap_list;
}

void NetInterface::show_provision_tip() {
    log_i("load prov tip");
}

void NetInterface::ip_monitor_callback() {
    esp_wifi_disconnect();
}

void NetInterface::ip_monitor_timer_wrapper(TimerHandle_t xTimer) {
    static_cast<NetInterface *>(pvTimerGetTimerID(xTimer))->ip_monitor_callback();
}

int NetInterface::ip_monitor_timer_create(const char *name, uint32_t ms) {
    uint32_t ticks = ARCH_OS_WAIT_MS2TICK(ms);

    m_ip_monitor_timer = xTimerCreate((const char *const) name, ticks,
                                      pdFALSE, this, ip_monitor_timer_wrapper);
    if (m_ip_monitor_timer == NULL)
        return 0;
    if (pdTRUE != xTimerStart(m_ip_monitor_timer, ticks)) {
        xTimerDelete(m_ip_monitor_timer, ARCH_OS_MS2TICK(ARCH_OS_TIMER_OPT_BLOCK_MS_MAX));
        m_ip_monitor_timer = NULL;
        return 0;
    }
    return 1;
}

int NetInterface::ip_monitor_timer_change(uint32_t ms) {
    uint32_t ticks = ARCH_OS_WAIT_MS2TICK(ms);

    int ret = xTimerChangePeriod(m_ip_monitor_timer, ticks, ARCH_OS_MS2TICK(ARCH_OS_TIMER_OPT_BLOCK_MS_MAX));

    return ret == pdPASS ? 1 : 0;
}

int NetInterface::ip_monitor_timer_delete() {
    int ret;

    ret = xTimerDelete(m_ip_monitor_timer, ARCH_OS_MS2TICK(ARCH_OS_TIMER_OPT_BLOCK_MS_MAX));

    return ret == pdPASS ? 1 : 0;
}

void NetInterface::ui_text_tip(const char *main, const char *sub) {
    if (LvglDriver::instance().get_current_ui_index() == UI::UI_BOOT)
        return;

    if (LvglDriver::instance().get_current_ui_index() == UI::UI_TEXT) {
        LvglDriver::instance().get_current_ui<UI::UIText>()->update(main, sub);
    } else {
        LvglDriver::instance().set_locked(false);
        auto ui = UI::build_ui<UI::UIText>(UI::UI_TEXT);
        LvglDriver::instance().load(ui, false, true, false,
                                    [&] { ui->update(main, sub); });
    }
}

extern "C" void get_ap_list(AP_IF_S **ap_ary, unsigned int *num) {
    *num = NetInterface::instance().get_ap_num();
    *ap_ary = NetInterface::instance().get_ap_list();
}