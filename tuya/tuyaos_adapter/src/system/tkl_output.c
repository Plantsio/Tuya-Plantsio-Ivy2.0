/**
 * @file tkl_output.c
 * @brief 日志操作接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "esp_err.h"
#include "esp_log.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"

#include "tkl_output.h"

/**
 * @brief tkl_log_output 用于输出log信息
 * 
 * @param[in] str log buffer指针
 */
VOID_T tkl_log_output(CONST CHAR_T *str, ...)
{
    if(str == NULL) {
        return;
    }

    ESP_LOGI("TKL_LOG", "%s", str);
}

/**
 * @brief 用于关闭原厂sdk默认的log口
 * 
 */
OPERATE_RET tkl_log_close(VOID_T)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief 用于恢复原厂sdk默认的log口
 * 
 */
OPERATE_RET tkl_log_open(VOID_T)
{
    return OPRT_NOT_SUPPORTED;
}
