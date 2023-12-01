//
// Created by Huwensong on 2022/11/23.
//

#ifndef MI_PLANTS_IVY_NETINTERFACE_H
#define MI_PLANTS_IVY_NETINTERFACE_H

#include "WiFi.h"
#include "tool/system.h"

// sdk includes
#include "device_interface.h"
#include "tuya_os_adapt_wifi.h"

class NetInterface {
public:
    static NetInterface &instance();

    NetInterface();

    NetInterface(const NetInterface &) = delete;

    NetInterface(NetInterface &&) = delete;

    NetInterface &operator=(const NetInterface &) = delete;

    NetInterface &operator=(NetInterface &&) = delete;

public:
    typedef enum {
        prov_no_need,
        prov_init,
        prov_ble_connected,
        prov_wifi_connected,
        prov_activated,
        prov_err_start,
        prov_wifi_conn_fail,
        prov_pwd_error,
        prov_ap_not_found,
        prov_rssi_low,
    } prov_status_t;

    void net_prov_start();

    bool net_is_provisioned();

    void net_prov_deinit();

    void net_prov_reset();

    void net_event_register();

    void net_event_handler(system_event_id_t event, system_event_info_t info);

    void net_status_change_handler(IN CONST GW_WIFI_NW_STAT_E stat);

    bool net_sync_geo_info();

    bool net_connected();

    void disable_wifi();

    void enable_wifi();

    void set_in_provision(bool in);

    uint16_t get_ap_num();

    AP_IF_S *get_ap_list();

    prov_status_t get_prov_status(){
        return m_prov_status;
    }

    void set_prov_status(prov_status_t status){
        log_d("xxxxxxxxxxxxxx set to %d", status);
        m_prov_status = status;
    }

private:
    void show_provision_tip();

    void ip_monitor_callback();

    static void ip_monitor_timer_wrapper(TimerHandle_t xTimer);

    int ip_monitor_timer_create(const char *name, uint32_t ms);

    int ip_monitor_timer_change(uint32_t ms);

    int ip_monitor_timer_delete();

    void ui_text_tip(const char *main, const char *sub);

private:
    bool in_provision = false;

    prov_status_t m_prov_status = prov_no_need;

    bool ap_not_found = false;

    TimerHandle_t m_ip_monitor_timer;

    uint16_t m_ap_num = 0;

    AP_IF_S *m_ap_list = nullptr;

};

#endif //MI_PLANTS_IVY_NETINTERFACE_H
