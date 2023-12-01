/**
 * @file tkl_mutex.c
 * @brief 互斥锁操作接口
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
#include "freertos/semphr.h"
#include "freertos/portmacro.h"

#include "esp_err.h"
#include "esp_log.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"

#include "tkl_memory.h"
#include "tkl_mutex.h"

/**
 * @brief tkl_mutex_create_init 用于创建并初始化tuya mutex
 * 
 * @param[out] pMutexHandle 返回mutex句柄
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_mutex_create_init(TKL_MUTEX_HANDLE *pMutexHandle)
{
    if (NULL == pMutexHandle) {
        return OPRT_INVALID_PARM;
    }

    *pMutexHandle = (TKL_MUTEX_HANDLE)xSemaphoreCreateRecursiveMutex();
	return ((NULL == *pMutexHandle) ? OPRT_MALLOC_FAILED : OPRT_OK);
}

/**
 * @brief tkl_mutex_lock 用于lock tuya mutex
 * 
 * @param[in] mutexHandle tuya mutex句柄
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_mutex_lock(const TKL_MUTEX_HANDLE mutexHandle)
{
    BaseType_t ret;

    if (NULL == mutexHandle) {
        return OPRT_INVALID_PARM;
    }

    ret = xSemaphoreTakeRecursive((SemaphoreHandle_t)mutexHandle, portMAX_DELAY);
	return ((ret == pdTRUE) ? OPRT_OK : OPRT_OS_ADAPTER_MUTEX_LOCK_FAILED);
}

/**
 * @brief tkl_mutex_unlock 用于unlock tuya mutex
 * 
 * @param[in] mutexHandle tuya mutex句柄
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_mutex_unlock(const TKL_MUTEX_HANDLE mutexHandle)
{
    BaseType_t ret;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (NULL == mutexHandle) {
        return OPRT_INVALID_PARM;
    }

    if (pdTRUE == xPortInIsrContext()) {
        ret = xSemaphoreGiveFromISR((SemaphoreHandle_t)mutexHandle, &xHigherPriorityTaskWoken);
        if (pdTRUE == ret && pdTRUE == xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    } else {
        ret = xSemaphoreGiveRecursive((SemaphoreHandle_t)mutexHandle);
    }
	
	return ((ret == pdTRUE) ? OPRT_OK : OPRT_OS_ADAPTER_MUTEX_UNLOCK_FAILED);
}

/**
 * @brief tkl_mutex_release 用于释放tuya mutex
 * 
 * @param[in] mutexHandle TKL_MUTEX_HANDLE tuya mutex句柄
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_mutex_release(const TKL_MUTEX_HANDLE mutexHandle)
{
    if (NULL == mutexHandle) {
        return OPRT_INVALID_PARM;
    }

    vSemaphoreDelete((SemaphoreHandle_t)mutexHandle);	
	return OPRT_OK;
}

/**
 * @brief tkl_mutex_trylock用于try lock tuya mutex
 * 
 * @param[in] mutexHandle tuya mutex句柄
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_mutex_trylock(const TKL_MUTEX_HANDLE mutexHandle)
{
    BaseType_t ret;

    if (NULL == mutexHandle) {
        return OPRT_INVALID_PARM;
    }
    
    ret = xSemaphoreTakeRecursive((SemaphoreHandle_t)mutexHandle, 0);
	return ((ret == pdTRUE) ? OPRT_OK : OPRT_OS_ADAPTER_MUTEX_LOCK_FAILED);
}
