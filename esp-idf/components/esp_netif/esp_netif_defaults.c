/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_netif.h"
#include "esp_wifi_default.h"
#if CONFIG_ETH_ENABLED
#include "esp_eth.h"
#endif

//
// Purpose of this module is to provide
//  - general esp-netif definitions of default objects for STA, AP, ETH
//  - default init / create functions for basic default interfaces
//



//
// Default configuration of common interfaces, such as STA, AP, ETH
//
const esp_netif_inherent_config_t _g_esp_netif_inherent_sta_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();

#ifdef CONFIG_ESP_WIFI_SOFTAP_SUPPORT
const esp_netif_inherent_config_t _g_esp_netif_inherent_ap_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_AP();
#endif

const esp_netif_inherent_config_t _g_esp_netif_inherent_eth_config = ESP_NETIF_INHERENT_DEFAULT_ETH();

const esp_netif_inherent_config_t _g_esp_netif_inherent_ppp_config = ESP_NETIF_INHERENT_DEFAULT_PPP();

const esp_netif_inherent_config_t _g_esp_netif_inherent_slip_config = ESP_NETIF_INHERENT_DEFAULT_SLIP();

const esp_netif_inherent_config_t _g_esp_netif_inherent_openthread_config = ESP_NETIF_INHERENT_DEFAULT_OPENTHREAD();

const esp_netif_ip_info_t _g_esp_netif_soft_ap_ip = {
        .ip = { .addr = ESP_IP4TOADDR( 192, 168, 176, 1) },
        .gw = { .addr = ESP_IP4TOADDR( 192, 168, 176, 1) },
        .netmask = { .addr = ESP_IP4TOADDR( 255, 255, 255, 0) },
};
