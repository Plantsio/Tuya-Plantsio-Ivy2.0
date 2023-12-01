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
#if 1
#include "esp_console.h"
#include "hal/uart_types.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#endif

//Arduino
#include "Arduino.h"
#include "SD_MMC.h"

//TuyaOS
#include "tuya_error_code.h"
#include "tuya_iot_wifi_api.h"
#include "ws_db_gw.h"
#include "base_event.h"
#include "tkl_system.h"
#include "tkl_memory.h"
#include "uni_log.h"
#include "tuya_iot_com_api.h"

#define APP_MAIN "APP_MAIN"
#ifdef ESP32S3
#define TUYA_CONSOLE_NUM_0_TXD  (GPIO_NUM_43)
#define TUYA_CONSOLE_NUM_0_RXD  (GPIO_NUM_44)
#else
#define TUYA_CONSOLE_NUM_0_TXD  (GPIO_NUM_33)
#define TUYA_CONSOLE_NUM_0_RXD  (GPIO_NUM_34)
#endif
#define TUYA_CONSOLE_NUM_0_RTS  (UART_PIN_NO_CHANGE)
#define TUYA_CONSOLE_NUM_0_CTS  (UART_PIN_NO_CHANGE)
#define TUYA_CONSOLE_NUM_0_BAUD_RATE 115200

extern void tuya_app_main(void);
extern int tuya_iot_wf_gw_unactive(void);

#define TUYA_CONSOLE_EN 1

#if TUYA_CONSOLE_EN
static int process_restart_cmd(int argc, char **argv)
{
    ESP_LOGI(APP_MAIN, "restarting...");
    esp_restart();
    return 0;
}


static int process_iotreset_cmd(int argc, char **argv)
{
    ESP_LOGI(APP_MAIN, "iot reset...");
    tuya_iot_wf_gw_unactive();
    return 0;
}
#endif /* TUYA_CONSOLE_EN */

#if 0
static void iterate_task(void (*callback)(TaskStatus_t *, unsigned int *, void *), unsigned int *jiffies, void *arg)
{
    TaskStatus_t *info;
    int i, nr_task;


    nr_task = uxTaskGetNumberOfTasks();
    info = tkl_system_malloc(nr_task * sizeof(*info));
    if (info == NULL)
        return;


    nr_task = uxTaskGetSystemState(info, nr_task, jiffies);
    for (i = 0; i < nr_task; i++) {
        callback(&info[i], jiffies, arg);
    }


    tkl_system_free(info);
    return;
}
#define tick_to_second(x) ((x) / (configTICK_RATE_HZ))


struct xtime {
    int dd;
    int hh;
    int mm;
    int ss;
};


static void ddhhmmss(long seconds, struct xtime *time) {


    if (!time)
        return;


    time->dd = seconds / (24 * 60 * 60);
    seconds -= time->dd * (24 * 60 * 60);
    time->hh = seconds / (60 * 60);
    seconds -= time->hh * (60 * 60);
    time->mm = seconds / 60;
    seconds -= time->mm * 60;
    time->ss = seconds;
};


static void print_task_info(TaskStatus_t *ti, uint32_t *jiffies, void *data)
{
    char state[] = {'X', 'R', 'B', 'S', 'D', 'I'};
    struct xtime time;
    unsigned long ratio;


    ratio = ((unsigned long long)ti->ulRunTimeCounter) * 1000 / (*jiffies + 1);
    ddhhmmss(tick_to_second(ti->ulRunTimeCounter), &time);


    //ESP_LOGI(APP_MAIN, "%4lu%5lu%6d%3c%5lu.%1d%5d:%02d:%02d   %-32s\n",
    ESP_LOGI(APP_MAIN, "%4u%5u%6d%3c%5lu.%1d%5d:%02d:%02d   %-32s\n",
           ti->xTaskNumber,
           ti->uxCurrentPriority,
           ti->usStackHighWaterMark,
           state[ti->eCurrentState],
           ratio / 10, (int) ratio % 10,
           time.hh, time.mm, time.ss,
           ti->pcTaskName);
}


static int process_show_statck_cmd(int argc, char **argv)
{
    unsigned int jiffies;

    //ESP_LOGI(APP_MAIN, "\n\n");
     ESP_LOGI(APP_MAIN, "%4s%5s%6s%3s%7s%11s   %-44s\n",
       "PID", "PR", "STWM", "S", "%CPU+", "TIME+", "TASK");


    iterate_task(print_task_info, &jiffies, NULL);
    return 0;
}
#endif

#if TUYA_CONSOLE_EN
void tuya_console_start(void)
{
    esp_err_t ret;
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t hw_config;

    const esp_console_cmd_t restart_cmd = {
        .command = "reboot",
        .help = "restart cmd",
        .hint = NULL,
        .func = &process_restart_cmd,
        .argtable = NULL
    };

    const esp_console_cmd_t iotreset_cmd = {
        .command = "iot_reset",
        .help = "iot reset cmd",
        .hint = NULL,
        .func = &process_iotreset_cmd,
        .argtable = NULL
    };


    hw_config.channel = 0;
    hw_config.baud_rate = TUYA_CONSOLE_NUM_0_BAUD_RATE;
    hw_config.tx_gpio_num = TUYA_CONSOLE_NUM_0_TXD;
    hw_config.rx_gpio_num = TUYA_CONSOLE_NUM_0_RXD;

    ret = esp_console_new_repl_uart(&hw_config, &repl_config, &repl);
    if (ESP_OK != ret) {
        ESP_LOGE(APP_MAIN, "%s: call esp_console_new_repl_uart failed(ret=%d)", __func__, ret);
        return;
    }

    ret = esp_console_start_repl(repl);
    if (ESP_OK != ret) {
        ESP_LOGE(APP_MAIN, "%s: call esp_console_start_repl failed(ret=%d)", __func__, ret);
    }

    esp_console_cmd_register(&restart_cmd);
    esp_console_cmd_register(&iotreset_cmd);

}
#endif /* TUYA_CONSOLE_EN */


extern "C" void app_main(void)
{
    esp_err_t ret;
    esp_chip_info_t chip_info;
    uint32_t flash_size;

#if TUYA_CONSOLE_EN
    tuya_console_start();
#endif

    ESP_LOGI(APP_MAIN, "%s: start tuya app main...", __func__);

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
