/**
 * @file tkl_semaphore.c
 * @brief semaphore相关接口封装
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/portmacro.h" 
#include "freertos/semphr.h"

#include "esp_err.h"
#include "esp_log.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"

#include "tkl_system.h"
#include "tkl_memory.h"
#include "tkl_semaphore.h"

/**
 * @brief tkl_semaphore_create_init 用于创建并初始化semaphore
 * 
 * @param[out] *pHandle semaphore句柄
 * @param[in] semCnt 
 * @param[in] sem_max 
 * @return int 0=成功，非0=失败
 */
int tkl_semaphore_create_init(TKL_SEM_HANDLE *pHandle, const unsigned int semCnt, const unsigned int sem_max)
{
    if(NULL == pHandle) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
    *pHandle = (TKL_SEM_HANDLE)xSemaphoreCreateCounting(sem_max, semCnt);
	
	return ((NULL == *pHandle) ? OPRT_OS_ADAPTER_SEM_CREAT_FAILED : OPRT_OK);
}


/**
 * @brief tkl_semaphore_wait timeout used fo wait semaphore with timeout
 *
 * @param[in] semHandle semaphore句柄
 * @param[in] timeout  semaphore超时时间
 * @return int 0=成功，非0=失败
*/
OPERATE_RET tkl_semaphore_wait(CONST TKL_SEM_HANDLE handle, CONST UINT_T timeout)
{
    BaseType_t ret;
    TickType_t xTicksToWait = portMAX_DELAY;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if(NULL == handle) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    if (pdTRUE == xPortInIsrContext()) {
        ret = xSemaphoreTakeFromISR((SemaphoreHandle_t)handle, &xHigherPriorityTaskWoken);
        if (pdTRUE == ret && pdTRUE == xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    } else {
        if (timeout < TKL_SEM_WAIT_FOREVER) {
            xTicksToWait = pdMS_TO_TICKS(timeout);
        }
        ret = xSemaphoreTake((SemaphoreHandle_t)handle, xTicksToWait);
    }
	
	return ((ret == pdTRUE) ? OPRT_OK : OPRT_OS_ADAPTER_SEM_WAIT_FAILED);
}

/**
 * @brief tkl_semaphore_post 用于post semaphore
 * 
 * @param[in] handle semaphore句柄
 * @return int 0=成功，非0=失败
 */
int tkl_semaphore_post(const TKL_SEM_HANDLE handle)
{
    BaseType_t ret;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if(NULL == handle) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
    if (pdTRUE == xPortInIsrContext()) {
        ret = xSemaphoreGiveFromISR((SemaphoreHandle_t)handle, &xHigherPriorityTaskWoken);
        if (pdTRUE == ret && pdTRUE == xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    } else {
        ret = xSemaphoreGive((SemaphoreHandle_t)handle);
    }

	return ((ret == pdTRUE) ? OPRT_OK : OPRT_OS_ADAPTER_SEM_POST_FAILED);
}

/**
 * @brief tkl_semaphore_release 用于release semaphore
 * 
 * @param[in] handle 
 * @return int 0=成功，非0=失败
 */
int tkl_semaphore_release(const TKL_SEM_HANDLE handle)
{
    if(NULL == handle) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
 
	vSemaphoreDelete((SemaphoreHandle_t)handle);
	return OPRT_OK;
}
