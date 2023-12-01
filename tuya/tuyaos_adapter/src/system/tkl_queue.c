/**
 * @file tkl_queue.c
 * @brief 队列操作接口
 *
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 *
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/portmacro.h"

#include "esp_err.h"
#include "esp_log.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"

#include "tkl_queue.h"

/**
 * @brief create queue
 *
 * @param[out]     queue      queue to be create
 * @param[in]      size       the deep of the queue
 * @return  OPRT_OK: SUCCESS other:fail
 */
OPERATE_RET tkl_queue_create_init(TKL_QUEUE_HANDLE *queue, INT_T msgsize, INT_T msgcount)
{
    if (NULL == queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    *queue = (TKL_QUEUE_HANDLE)xQueueCreate(msgcount, msgsize);
	return ((NULL == *queue) ? OPRT_OS_ADAPTER_QUEUE_CREAT_FAILED : OPRT_OK);
}

/**
 * @brief free queue
 *
 * @param[in]     queue      queue to be free
 * @return  void
 */
VOID_T tkl_queue_free(CONST TKL_QUEUE_HANDLE queue)
{
    if (NULL == queue) {
        return;
    }

    vQueueDelete((QueueHandle_t)queue);
}

/**
 * @brief post msg to queue in timeout ms
 *
 * @param[in]      queue      queue to post
 * @param[in]      msg        msg to post
 * @param[in]      timeout    max time to wait for msg(ms), TUYA_OS_ADAPT_QUEUE_FOREVER means forever wait
 * @return  int OPRT_OK:success    other:fail
 */
OPERATE_RET tkl_queue_post(CONST TKL_QUEUE_HANDLE queue, VOID_T *data, UINT_T timeout)
{
    BaseType_t ret;
    TickType_t xTicksToWait = portMAX_DELAY;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (NULL == queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
    if (pdTRUE == xPortInIsrContext()) {
        ret = xQueueSendFromISR((QueueHandle_t)queue, data, &xHigherPriorityTaskWoken);
        if (ret == pdTRUE && pdTRUE == xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    } else {
        if (timeout < TKL_QUEUE_WAIT_FROEVER) {
            xTicksToWait = pdMS_TO_TICKS(timeout);
        }
        ret = xQueueSend((QueueHandle_t)queue, data, xTicksToWait);
    }

    return ((ret == pdTRUE) ? OPRT_OK : OPRT_OS_ADAPTER_QUEUE_SEND_FAIL);
}

/**
 * @brief fetch msg from queue in timeout ms, copy data from queue
 *
 * @param[in]      queue      queue to post
 * @param[out]     msg        msg to fetch
 * @param[in]      timeout    max time to wait for msg(ms), TUYA_OS_ADAPT_QUEUE_FOREVER means forever wait
 * @return  int OPRT_OK:success    other:fail
 */
OPERATE_RET tkl_queue_fetch(CONST TKL_QUEUE_HANDLE queue, VOID_T *msg, UINT_T timeout)

{
    BaseType_t ret;
    TickType_t xTicksToWait = portMAX_DELAY;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (NULL == queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    if (pdTRUE == xPortInIsrContext()) {
        ret = xQueueReceiveFromISR((QueueHandle_t)queue, msg, &xHigherPriorityTaskWoken);
        if (pdTRUE == ret && pdTRUE == xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    } else {
        if(timeout < TKL_QUEUE_WAIT_FROEVER)    {
    	    xTicksToWait = pdMS_TO_TICKS(timeout);
        }
        ret = xQueueReceive((QueueHandle_t)queue, msg, xTicksToWait);
    }

    return ((ret == pdTRUE) ? OPRT_OK : OPRT_OS_ADAPTER_QUEUE_RECV_FAIL);
}
