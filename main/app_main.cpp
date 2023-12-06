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

#define Motor_Control_Pin 12
const char *mount_name = "/sd";

Audio audio;
bool isPlaying = true; // 用于追踪音频是否正在播放
bool isFileFinished = false; // 用于追踪音频文件是否播放完成

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

void setup()
{
//    Serial.begin(115200);
//    pinMode(12, OUTPUT);
//    SD_MMC.setPins(SD_CLK,SD_CMD,SD_D0);
//    SD_MMC.begin("/sd",true);
//    digitalWrite(12, 0);
//    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
//    audio.setVolume(12);
//
//    uint8_t cardType = SD_MMC.cardType();
//    Serial.print("SD_MMC Card Type: ");
//    if(cardType == CARD_MMC){
//        Serial.println("MMC");
//    } else if(cardType == CARD_SD){
//        Serial.println("SDSC");
//    } else if(cardType == CARD_SDHC){
//        Serial.println("SDHC");
//    } else {
//        Serial.println("UNKNOWN");
//    }

//    const char* audiopath = "/testB.mp3";
//    audio.connecttoFS(SD_MMC,audiopath);

    Serial.begin(115200);
    //esp_log_level_set("*", ESP_LOG_VERBOSE);
    pinMode(Motor_Control_Pin, OUTPUT);
//    digitalWrite(Motor_Control_Pin, 0);
    SD_MMC.setPins(SD_CLK,SD_CMD,SD_D0);
    SD_MMC.begin(mount_name, true);
    digitalWrite(Motor_Control_Pin, 0);
    uint8_t cardType = SD_MMC.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD_MMC card attached");
        return;
    }

    Serial.print("SD_MMC Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);


    // Setup I2S
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

    // Set Volume
    audio.setVolume(8);

    // Open music file
    const char sdpath = '/sd/';
//    listDir(SD_MMC,&sdpath,0);
//    File audioFile = SD_MMC.open("/music/test16.mp3");
//    if (!audioFile) {
//        Serial.println("Failed to open audio file");
//        return;
//    }
//    audio.connecttoFS(SD_MMC,"/test.mp3");
    int32_t timestamp1 = millis();
    log_e("begin to open file: %d",timestamp1);
//    audio.connecttoFS(SD_MMC,"/music/test16.mp3");

    const char* audiopath = "/testB.mp3";
    audio.connecttoFS(SD_MMC,audiopath);
}

int beginloop = false;
int32_t timestamp2 = millis();
void loop()
{
//    audio.loop();
//    vTaskDelay(1); // 适当的延迟，以避免任务看门狗触发

    if(!beginloop) {
        log_i("begin to loop: %d", timestamp2);
        beginloop = true;
    }
    if (audio.isRunning()) {
//        log_i("print m_f_running: %",audio.m_f_running);
        audio.loop();

        // 检查音频是否播放完成
        if (isFileFinished) {
            isPlaying = false;
            // 处理音频播放完成事件，例如停止播放、跳转到下一个文件等
            // ...
        }
    } else{
        vTaskDelay(100);
    }

    // 其他代码逻辑

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
