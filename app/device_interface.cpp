//
// Created by Huwensong on 2022/12/27.
//

#include "device_interface.h"

//esp-idf
#include "esp_bt.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "nvs_flash.h"

//arduino
#include "SD_MMC.h"

//tuya sdk
#include "uni_log.h"
#include "tuya_iot_com_api.h"
#include "tuya_cloud_error_code.h"
#include "gw_intf.h"
#include "tuya_gpio.h"
#include "tuya_key.h"
#include "ws_db_gw.h"
#include "bt_netcfg.h"

//ivy
#include "Anim/drivers/LvglDriver.h"
#include "tool/OTA.h"
#include "Body/IvyBody.h"

//app
#include "IOT.h"
#include "NetInterface.h"
#include "partition_table.h"
#include "UIProvTip.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
#define APP_RAW_PRINT_DEBUG 1
#define _TUYA_DEVICE_GLOBAL
#define PATH_OTA_FILE                   "/ota/firmware.bin"

/***********************************************************
***********************typedef define***********************
***********************************************************/

/***********************************************************
***********************variable define**********************
***********************************************************/
ble_netcfg_context_t* ble_netcfg_ctx = NULL;

BOOL_T bt_stop_flag = FALSE;

/***********************************************************
***********************function define**********************
***********************************************************/

/**
* @brief initiation key, led
*
* @return none
*/
STATIC VOID wifi_config_init(VOID)
{
}

/**
* @brief erase application data from flash
*
* @return none
*/
STATIC VOID hw_reset_flash_data(VOID)
{
    /* erase application data from flash */
}

/**
* @brief pre_gpio_test gpio test pre-interface, used to prepare for the gpio test
*
* @return none
*/
VOID mf_user_pre_gpio_test_cb(VOID)
{
}

/**
* @brief configuring the write callback interface
*
* @return none
*/
VOID mf_user_callback(VOID)
{
    hw_reset_flash_data();
}

/**
* @brief configure to enter the production test callback interface
*
* @return none
*/
VOID mf_user_enter_callback(VOID)
{
}

/**
* @brief Finished Product test callbacks
*
* @return OPRT_OK
*/
OPERATE_RET
mf_user_product_test_cb(USHORT_T cmd, UCHAR_T *data, UINT_T len, OUT UCHAR_T **ret_data, OUT USHORT_T *ret_len)
{
    return OPRT_OK;
}

/**
* @brief application initialization prep work
*
* @param[in] none
* @return none
*/
VOID pre_app_init(VOID)
{
}

/**
* @brief device initialization prep work
*
* @param[in] none
* @return none
*/
VOID pre_device_init(VOID)
{
    PR_NOTICE("%s", tuya_iot_get_sdk_info()); /* print SDK information */
    PR_NOTICE("%s:%s", APP_BIN_NAME, DEV_SW_VERSION); /* print the firmware name and version */
    PR_NOTICE("firmware compiled at %s %s", __DATE__, __TIME__); /* print firmware compilation time */

    SetLogManageAttr(TY_LOG_LEVEL_NOTICE);  /* set log level */
}

/**
* @brief application initialization interface
*
* @param[in] none
* @return none
*/
VOID app_init(VOID)
{
    /* WiFi key, len initiation */
    wifi_config_init();
}

/**
* @brief report all dp status
*
* @param[in] none
* @return none
*/
VOID hw_report_all_dp_status(VOID)
{
    //report all dp status
}

/**
* @brief firmware download finish result callback
*
* @param[in] fw: firmware info
* @param[in] download_result: 0 means download succes. other means fail
* @param[in] pri_data: private data
* @return none
*/
VOID upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID_T pri_data)
{
    PR_DEBUG("download  Finish");
    PR_DEBUG("download_result:%d", download_result);
}

/**
* @brief firmware download content process callback
*
* @param[in] fw: firmware info
* @param[in] total_len: firmware total size
* @param[in] offset: offset of this download package
* @param[in] data && len: this download package
* @param[out] remain_len: the size left to process in next cb
* @param[in] pri_data: private data
* @return OPRT_OK: success  Other: fail
*/
OPERATE_RET get_file_data_cb(IN CONST FW_UG_S *fw, IN CONST UINT_T total_len, IN CONST UINT_T offset,
                             IN CONST BYTE_T *data, IN CONST UINT_T len, OUT UINT_T *remain_len, IN PVOID_T pri_data)
{
    PR_DEBUG("Rev File Data");
    PR_DEBUG("Total_len:%d ", total_len);
    PR_DEBUG("Offset:%d Len:%d", offset, len);

    return OPRT_OK;
}

/**
* @brief ota inform callback
*
* @param[in] fw: firmware info
* @return
*/
INT_T gw_ug_inform_cb(IN CONST FW_UG_S *fw)
{
    printf("gw_ug_inform_cb is called\n");
    PR_DEBUG("Rev GW Upgrade Info");
    PR_DEBUG("fw->fw_url:%s", fw->fw_url);
    PR_DEBUG("fw->sw_ver:%s", fw->sw_ver);
    PR_DEBUG("fw->file_size:%d", fw->file_size);

    return tuya_iot_upgrade_gw(fw, get_file_data_cb, upgrade_notify_cb, NULL);
}

/**
* @brief called after reset device or app remove device
*
* @param[in] type: gateway reset type
* @return none
* @others reset factory clear flash data
*/
VOID gw_reset_cb(IN CONST GW_RESET_TYPE_E type)
{
    PR_DEBUG("gw_reset_cb type:%d", type);

    if (GW_REMOTE_RESET_FACTORY == type)
    {
        hw_reset_flash_data();

    }
    else
    {
        PR_DEBUG("type is GW_REMOTE_RESET_FACTORY");
    }
}

/**
* @brief called after the cloud sends data of type bool, value, enum, string or fault
*
* @param[in] dp: obj dp info
* @return none
*/
VOID dev_obj_dp_cb(IN CONST TY_RECV_OBJ_DP_S *dp)
{
    bool ret;
    PR_DEBUG("value data dpid:%d", dp->dps->dpid);
    for (UCHAR_T i = 0;i < dp->dps_cnt;i++)
    {
        switch (dp->dps[i].type)
        {
            case PROP_VALUE:
                ret = IOT::instance().dp_handler((Dp)dp->dps[i].dpid,dp->dps[i].value.dp_value);
                if (!ret)
                    return;
                break;
            case PROP_BOOL:
                ret = IOT::instance().dp_handler((Dp)dp->dps[i].dpid,dp->dps[i].value.dp_bool);
                if (!ret)
                    return;
                break;
            default:
                break;
        }

    }
    dev_report_dp_json_async(get_gw_cntl()->gw_if.id, dp->dps, dp->dps_cnt);
}

/**
* @brief called after the cloud sends data of type raw
*
* @param[in] dp: raw dp info
* @return none
*/
VOID dev_raw_dp_cb(IN CONST TY_RECV_RAW_DP_S *dp)
{
    log_d("raw data dpid:%d", dp->dpid);
    log_d("recv len:%d", dp->len);

    INT_T i = 0;
    for (i = 0;i < dp->len;i++)
    {
        log_d("%d ", dp->data[i]);
    }
}

/**
* @brief query device dp status
*
* @param[in] dp_qry: query info
* @return none
*/
STATIC VOID dev_dp_query_cb(IN CONST TY_DP_QUERY_S *dp_qry)
{
    log_d("Recv DP Query Cmd");

    hw_report_all_dp_status();

}

/**
* @brief This function is called when the state of the device connection has changed
*
* @param[in] stat: curr network status
* @return none
*/
VOID wf_nw_status_cb(IN CONST GW_WIFI_NW_STAT_E stat)
{
    PR_NOTICE("wf_nw_status_cb, wifi_status:%d", stat);
    NetInterface::instance().net_status_change_handler(stat);
}

/**
* @brief device activation status change callback
*
* @param[in] status: current status
* @return none
*/
VOID status_changed_cb(IN CONST GW_STATUS_E status)
{
    PR_NOTICE("status_changed_cb is status:%d", status);
}

/**
* @brief device initialization interface
*
* @param[in] none
* @return OPRT_OK: success, others: please refer to tuya error code description document
*/
OPERATE_RET device_init(VOID)
{
    OPERATE_RET op_ret = OPRT_OK;

    TY_IOT_CBS_S wf_cbs = {
            status_changed_cb, \
        gw_ug_inform_cb, \
        gw_reset_cb, \
        dev_obj_dp_cb, \
        dev_raw_dp_cb, \
        dev_dp_query_cb, \
        NULL,
    };

    /* tuya IoT framework initialization */
    op_ret = tuya_iot_wf_soc_dev_init_param(GWCM_OLD, WF_START_SMART_FIRST, &wf_cbs, PRODUCT_ID, PRODUCT_ID,
                                            DEV_SW_VERSION);
    if (OPRT_OK != op_ret)
    {
        PR_ERR("tuya_iot_wf_soc_dev_init_param error,err_num:%d", op_ret);
        return op_ret;
    }

    /* register Wi-Fi connection status change callback function */
    op_ret = tuya_iot_reg_get_wf_nw_stat_cb(wf_nw_status_cb);
    if (OPRT_OK != op_ret)
    {
        PR_ERR("tuya_iot_reg_get_wf_nw_stat_cb is error,err_num:%d", op_ret);
        return op_ret;
    }

    return op_ret;

}

int tuya_dev_init_event_cb(void *param)
{
    GW_WORK_STAT_MAG_S read_gw_wsm;
    OPERATE_RET op_ret;
    op_ret = wd_gw_wsm_read(&read_gw_wsm);
    if(OPRT_OK != op_ret) {
        return op_ret;
    }
    PR_DEBUG("wifi-config status = %d",read_gw_wsm.nc_tp);


    if((Prop::get<bool>(Prop::tutorial_complete) && strlen(read_gw_wsm.ssid)) > 0 && ((read_gw_wsm.nc_tp > GWNS_UNCFG_AP) && (read_gw_wsm.md == GWM_NORMAL))) {
        PR_DEBUG("esp_bt_controller_mem_release [ESP_BT_MODE_BLE]\r\n");
        ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

        //start boot ui
        LvglDriver::instance().load(UI::UI_BOOT, true, true, true);
    }
    else
    {
        NetInterface::instance().set_in_provision(true);
        IvyBody::instance().unregister_button_tap();

        IOT::instance().ensure_pass_provision(true);
        // start prov ui
        LvglDriver::instance().load(UI::UI_PROV_TIP, true, true, true);
    }
    return OPRT_OK;
}

static void ble_netcfg_event_on(netfcg_event_id_t event_id, netcfg_data_t* data, void* user_data)
{
    switch (event_id) {
        case NETCFG_EVENT_BLE_COMPLETED:
            PR_DEBUG("NETCFG_EVENT_BLE_COMPLETED");
            PR_DEBUG("ssid: %s", data->ssid);
            PR_DEBUG("password: %s", data->password);
            PR_DEBUG("token: %s", data->token);

            gw_wifi_user_cfg( data->ssid, data->password, data->token);

            break;

        case NETCFG_EVENT_BLE_DISCONNECTED:
            PR_DEBUG("NETCFG_EVENT_BLE_DISCONNECTED");
            break;

        case NETCFG_EVENT_BLE_STOP:
            PR_DEBUG("NETCFG_EVENT_BLE_STOP");
            break;

        case NETCFG_EVENT_BLE_TIMEOUT:
            PR_DEBUG("NETCFG_EVENT_BLE_TIMEOUT");
            break;

        case NETCFG_EVENT_BLE_FAILED:
            PR_DEBUG("NETCFG_EVENT_BLE_FAILED");
            break;

        default:
            break;
    }
}

extern "C" OPERATE_RET tuya_iot_bt_init(VOID)
{
    GW_CNTL_S *gw_cntl = get_gw_cntl();
    log_i("tuya_iot_bt_init");

    esp_err_t ret;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE("device_interface", "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return OPRT_COM_ERROR;
    }

    ble_netcfg_ctx = (ble_netcfg_context_t *)Malloc(sizeof(ble_netcfg_context_t));
    memset(ble_netcfg_ctx, 0, sizeof(ble_netcfg_context_t));

    ble_netcfg_config_t ble_netcfg =
            {
                    .product_id = gw_cntl->gw_if.product_key,
                    .uuid = gw_cntl->gw_base.uuid,
                    .auth_key = gw_cntl->gw_base.auth_key,
                    .firmware_version = 1,
                    .hardware_version = 0,
                    .user_data = NULL,
                    .event_handler = ble_netcfg_event_on
            };
    ble_netcfg_init(ble_netcfg_ctx, &ble_netcfg);

    ble_netcfg_start(ble_netcfg_ctx);

    bt_stop_flag = FALSE;

    return OPRT_OK;
}

extern "C" void tuya_iot_bt_stop(void)
{
    if(!bt_stop_flag) {
        PR_DEBUG("tuya_iot_bt_stop");
        ble_netcfg_stop(ble_netcfg_ctx);
        bt_stop_flag = TRUE;
        Free(ble_netcfg_ctx);
    }
    NetInterface::instance().set_prov_status(NetInterface::prov_ble_connected);
}

bool chang_partition()
{
    log_i("upgrading from local file");
    /* prepare esp ota */
    esp_ota_handle_t ota_handle;
    const esp_partition_t *next_partition = esp_ota_get_next_update_partition(NULL);
    int ret = esp_ota_begin(next_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (ret != ESP_OK) {
        log_e("ota failed to start");
        return false;
    }

    /* ota get_percent */
    File ota_file = SD_MMC.open(PATH_OTA_FILE, FILE_READ);
    size_t total_size = ota_file.size();
    size_t size_read = 0;
//    auto ota_buf = (uint8_t *) malloc(1024);
    auto ota_buf = (uint8_t *) heap_caps_malloc(1024 * 6, MALLOC_CAP_SPIRAM);
    while (true) {
        if (!ota_file.available()) {
            break;
        }
        int len = ota_file.readBytes((char *) ota_buf, 1024 * 6);
        if (len > 0) {
            esp_ota_write(ota_handle, ota_buf, len);
            size_read += len;
            log_v("upgrade progress %d/%d", size_read, total_size);
        }
        /* to counter WDT error */
        vTaskDelay(20);
    }
    ota_file.close();
    free(ota_buf);
    ret = esp_ota_end(ota_handle);
    if (ret != ESP_OK) {
        log_e("ota failed to end");
    } else {
        esp_ota_set_boot_partition(next_partition);
        log_i("ota success");
        return true;
    }
    return false;
}

bool not_find_tuya_patition()
{
   const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                                       ESP_PARTITION_SUBTYPE_ANY, "tuya");

    if (part == NULL)
        return true;
    return false;
}

bool running_in_app1()
{
    const esp_partition_t *boot_partition = esp_ota_get_running_partition();

    if (boot_partition->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1)
    {
        log_i("start from ota_1 partition");
        return true;
    }
    log_i("start from ota_0 partition");
    return false;
}

void update_partition_table()
{
    log_i("update partition table");
    esp_err_t ret = spi_flash_erase_range(CONFIG_PARTITION_TABLE_OFFSET, 0x1000);
    if (ret == ESP_OK)
    {
        log_i("partition table len = %d",sizeof(tuya_partition_table));
        esp_err_t ret = spi_flash_write(CONFIG_PARTITION_TABLE_OFFSET, tuya_partition_table, sizeof(tuya_partition_table));
        log_i("spi flash write ret = %d",ret);
        ESP_ERROR_CHECK(nvs_flash_erase());
        log_i("Success to update partition table,reboot!");
        vTaskDelay(3000);
        esp_restart();

    }
}

void read_patition_table()
{
    File ota_file = SD_MMC.open("/ota/partition-table.bin", FILE_READ);

    uint8_t *table_bin_8bit = (uint8_t *)heap_caps_malloc(TUYA_PARTITION_TABLE_LEN,MALLOC_CAP_SPIRAM);

    ota_file.read(table_bin_8bit,TUYA_PARTITION_TABLE_LEN);

    log_i("table data = %x %x %x %x %x %x",table_bin_8bit[0],table_bin_8bit[1],table_bin_8bit[2],table_bin_8bit[3],table_bin_8bit[4],table_bin_8bit[5]);

    ota_file.close();

    free(table_bin_8bit);
}

void ota_update_partition_table()
{
//    if (chang_partition())
//        esp_restart();

    if (not_find_tuya_patition())   // OTA 过程中分区表不一致更新最新分区表（OTA分区起始地址不变）
    {
        update_partition_table();
//        if (running_in_app1())
//            update_partition_table();
    }
}

void ota_ui_start()
{
    OTA::instance().start();
}

void ota_ui_progress(unsigned int progress)
{
    OTA::instance().set_download_progress(progress);
}

void ota_ui_end(bool success)
{
    OTA::instance().finish(success);
}

