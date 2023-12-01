/**
 * @file tkl_rtc.c
 * @brief RTC 操作接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"
#include "time.h"
#include "sys/time.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"

#include "tkl_system.h"
#include "tkl_memory.h"
#include "tkl_rtc.h"
#include "tkl_output.h"

/**
 * @brief rtc init
 * 
 * @param[in] none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_rtc_init(VOID_T)
{
    return OPRT_OK;
}

/**
 * @brief rtc time get
 * 
 * @param[in] time_sec:rtc time seconds
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_rtc_time_get(TIME_T *time_sec)
{   
    struct timeval now;
    gettimeofday(&now, NULL);
    *time_sec = now.tv_sec;
    return OPRT_OK;
}

/**
 * @brief rtc time set
 * 
 * @param[in] time_sec: rtc time seconds
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_rtc_time_set(TIME_T time_sec)
{
    struct timeval time_new;
    time_new.tv_sec = time_sec;
    time_new.tv_usec = 0;
    settimeofday(&time_new, NULL);
    return OPRT_OK;
}

/**
 * @brief rtc deinit
 * @param[in] none
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_rtc_deinit(VOID_T)
{
    return OPRT_OK;
}
