/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

//ESP-IDF
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_efuse.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_console.h"
#include "hal/uart_types.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/task.h"

//Arduino
#include "Arduino.h"
#include "SD_MMC.h"

//TuyaOS
#include "tuya_error_code.h"
#include "tuya_iot_wifi_api.h"
#include "ws_db_gw.h"
#include "base_event.h"
#include "tkl_system.h"
#include "uni_log.h"
#include "tuya_iot_com_api.h"

#include "IvyAnim.h"


//#define I2S_DOUT    13
//#define I2S_BCLK    40
//#define I2S_LRC     41

#define I2S_DOUT    40
#define I2S_BCLK    13
#define I2S_LRC     14

//#define SD_D0       48
//#define SD_CMD      47
//#define SD_CLK      45

void test_tuysos()
{
    OPERATE_RET rt = OPRT_OK;

    //初始化TuyaOS
    TY_INIT_PARAMS_S init_params = {0};
    init_params.init_db = FALSE;
    TUYA_CALL_ERR_LOG(tuya_iot_init_params(NULL,&init_params));


    //wifi授权信息
    WF_GW_PROD_INFO_S prod_info = {0};
    TUYA_CALL_ERR_LOG(tuya_iot_set_wf_gw_prod_info(&prod_info));

    //注册功能回调函数
    TY_IOT_CBS_S iot_cbs = {0};
    TUYA_CALL_ERR_LOG(tuya_iot_wf_soc_dev_init_param(GWCM_OLD, WF_START_SMART_FIRST, &iot_cbs, "PRODUCT_ID", "PRODUCT_ID",
                                                     "1.0.0"));


    //注册网络状态回调
    TUYA_CALL_ERR_LOG(tuya_iot_reg_get_wf_nw_stat_cb_params(NULL,1));


    //启动wifi
    esp_wifi_start();
}

void file_read_task(void *param)
{
    File file = SD_MMC.open("/test.mp3");

    if (file.available())
    {
        int32_t file_length = file.size();

        while (file_length --)
        {
            uint8_t  c = file.read();
            log_e("read one byte = %c",c);
            vTaskDelay(200);
        }

        file.close();
    }
    else
    {
        log_e("Failed to open file");
    }


}
static IvyAnim anim;

void setup()
{
    anim.init_file_sys();
    anim.bind_assets("calorie");
    anim.play();
}

void loop()
{
}

//extern "C" void app_main(void)
//{
//
//    Audio audio;
//    //init sd
//}

