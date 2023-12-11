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

#include "Audio.h"

//#define I2S_DOUT    13
//#define I2S_BCLK    40
//#define I2S_LRC     41

#define I2S_DOUT    40
#define I2S_BCLK    13
#define I2S_LRC     14

#define SD_D0       48
#define SD_CMD      47
#define SD_CLK      45

Audio audio;

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
            file.read();
            log_e("read one byte");
            vTaskDelay(200);
        }

        file.close();
    }
    else
    {
        log_e("Failed to open file");
    }


}

void setup()
{
    Serial.begin(115200);

    SD_MMC.setPins(SD_CLK,SD_CMD,SD_D0);
    SD_MMC.begin("/sd",true);

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(7);

    const char* audiopath = "/testB.mp3";
    audio.connecttoFS(SD_MMC,audiopath);

    xTaskCreatePinnedToCore(file_read_task,"FileRead",2048, nullptr,5, nullptr,1);

}

void loop()
{
    audio.loop();
    vTaskDelay(1); // 适当的延迟，以避免任务看门狗触发
}

void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char *info){  //id3 metadata
    Serial.print("id3data     ");Serial.println(info);
}
void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
}
void audio_showstation(const char *info){
    Serial.print("station     ");Serial.println(info);
}
void audio_showstreamtitle(const char *info){
    Serial.print("streamtitle ");Serial.println(info);
}
void audio_bitrate(const char *info){
    Serial.print("bitrate     ");Serial.println(info);
}
void audio_commercial(const char *info){  //duration in sec
    Serial.print("commercial  ");Serial.println(info);
}
void audio_icyurl(const char *info){  //homepage
    Serial.print("icyurl      ");Serial.println(info);
}
void audio_lasthost(const char *info){  //stream URL played
    Serial.print("lasthost    ");Serial.println(info);
}
void audio_eof_speech(const char *info){
    Serial.print("eof_speech  ");Serial.println(info);
}

//extern "C" void app_main(void)
//{
//
//    Audio audio;
//    //init sd
//}
