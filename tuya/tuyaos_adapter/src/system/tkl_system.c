/**
 * @file tkl_system.c
 * @brief 操作系统相关接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_random.h"
#include "esp_task_wdt.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"

#include "tkl_system.h"
#include "tkl_watchdog.h"
#include "tkl_output.h"

#define DBG_TAG "TKL_SYSTEM"
unsigned char tuya_watchdog_tread_inited = 0;

void esp_task_wdt_isr_user_handler(void)
{

}

/**
 * @brief tkl_system_sleep 用于系统sleep
 * 
 * @param[in] msTime sleep time is ms
 */
VOID_T tkl_system_sleep(UINT_T num_ms)
{
    TickType_t xTicksToDelay;
    xTicksToDelay = pdMS_TO_TICKS(num_ms);
    vTaskDelay(xTicksToDelay);
}

/**
 * @brief tkl_system_reset 用于重启系统
 * 
 */
void tkl_system_reset(void)
{
    esp_restart();
}

/**
 * @brief tkl_system_get_reset_reason 用于获取硬件重启原因
 * 
 * @return 硬件重启原因
 */
TUYA_RESET_REASON_E tkl_system_get_reset_reason(CHAR_T** ext_info)
{
    esp_reset_reason_t reason;

    reason = esp_reset_reason();
    switch (reason) {
    case ESP_RST_POWERON:
        return TUYA_RESET_REASON_POWERON;
    case ESP_RST_EXT:
        return TUYA_RESET_REASON_EXTERNAL;
    case ESP_RST_SW:
        return TUYA_RESET_REASON_SOFTWARE;
    case ESP_RST_PANIC:
        return TUYA_RESET_REASON_FAULT;
    case ESP_RST_INT_WDT:
        return TUYA_RESET_REASON_SW_WDOG;
    case ESP_RST_TASK_WDT:
        return TUYA_RESET_REASON_SW_WDOG;
    case ESP_RST_WDT:
        return TUYA_RESET_REASON_SW_WDOG;
    case ESP_RST_DEEPSLEEP:
        return TUYA_RESET_REASON_DEEPSLEEP;
    case ESP_RST_BROWNOUT:
        return TUYA_RESET_REASON_BROWNOUT;
    case ESP_RST_SDIO:
        return TUYA_RESET_REASON_EXTERNAL;
    default:
        return TUYA_RESET_REASON_UNKNOWN;
    }
    
    return TUYA_RESET_REASON_UNSUPPORT;
}

/**
 * @brief tkl_system_get_random 用于获取指定条件下的随机数
 * 
 * @param[in] range 
 * @return 随机值
 */
int tkl_system_get_random(const unsigned int range)
{
    return esp_random() % range;
}

/**
 * @brief tkl_cpu_sleep_mode_set 用于设置cpu的低功耗模式
 * 
 * @param[in] en 
 * @param[in] mode
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_cpu_sleep_mode_set(BOOL_T enable, TUYA_CPU_SLEEP_MODE_E mode)
{
	return OPRT_OK;
}

/**
 * @brief 用于初始化并运行watchdog
 * 
 * @param[in] timeval watch dog检测时间间隔：如果timeval大于看门狗的
 * 最大可设置时间，则使用平台可设置时间的最大值，并且返回该最大值
 * @return int [out] 实际设置的看门狗时间
 */
UINT_T tkl_watchdog_init(TUYA_WDOG_BASE_CFG_T *cfg)
{
    uint32_t timeouts;

    timeouts = (cfg->interval_ms + 1000 - 1) / 1000;

    if (ESP_OK != esp_task_wdt_init(timeouts, true)) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_task_wdt_init faield", __func__);
        return OPRT_COM_ERROR;
    }
    
    return OPRT_OK;
}

/**
 * @brief 用于刷新watch dog
 * 
 */
OPERATE_RET tkl_watchdog_refresh(VOID_T)
{
    if (!tuya_watchdog_tread_inited) {
        if (ESP_OK != esp_task_wdt_add(NULL)) {
            //ESP_LOGE(DBG_TAG, "%s: call esp_task_wdt_init faield", __func__);
            return OPRT_COM_ERROR;
        }
        tuya_watchdog_tread_inited = 1;
    }

    if (ESP_OK != esp_task_wdt_reset()) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_task_wdt_reset faield", __func__);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief 用于停止watch dog
 * 
 */
OPERATE_RET tkl_watchdog_deinit(VOID_T)
{
    if (ESP_OK != esp_task_wdt_deinit()) {
        //ESP_LOGE(DBG_TAG, "%s: call esp_task_wdt_deinit faield", __func__);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

/**
* @brief Get system free heap size
*
* @param none
*
* @return heap size
*/
INT_T tkl_system_get_free_heap_size(VOID_T)
{
    return  (int)xPortGetFreeHeapSize();
}

SYS_TICK_T tkl_system_get_tick_count(VOID_T)
{
    return (SYS_TICK_T)xTaskGetTickCount();
}

SYS_TIME_T tkl_system_get_millisecond(VOID_T)
{
    return pdTICKS_TO_MS(tkl_system_get_tick_count());
}

/**
* @brief system delay
*
* @param[in] msTime: time in MS
*
* @note This API is used for system sleep.
*
* @return VOID
*/
VOID_T tkl_system_delay(UINT_T num_ms)
{
    tkl_system_sleep(num_ms);
}