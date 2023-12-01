/**
 * @file tkl_ota.c
 * @brief ota底层操作接口
 * 
 * @copyright Copyright (c) {2018-2020} 涂鸦科技 www.tuya.com
 * 
 */
#include "freertos/FreeRTOS.h"
#include "assert.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
#include "tkl_memory.h"
#include "tkl_system.h"
#include "tkl_ota.h"

typedef enum {
    TKL_OTA_INIT,
    TKL_OTA_BEGIN,
    TKL_OTA_IN_PROGRESS,
    TKL_OTA_SUCCESS,
} TKL_OTA_STATE_T;

typedef struct {
    TKL_OTA_STATE_T  state; 
    esp_ota_handle_t handle;
    esp_partition_t *partition;
    unsigned int     image_size;
    unsigned int     offset_image_size;
} TKL_OTA_HANDLE_S;

TKL_OTA_HANDLE_S tkl_ota_handle = {
    .state = TKL_OTA_INIT,
    .handle = 0,
    .partition = NULL,
    .image_size = 0,
    .offset_image_size = 0
};

#define DBG_TAG "TKL_OTA"

#define TKL_OTA_UPGRADE_HANDLE_INIT(h) do { \
     (h).state = TKL_OTA_INIT; \
     (h).handle = 0; \
     (h).partition = NULL; \
     (h).image_size = 0; \
     (h).offset_image_size = 0; \
} while (0)

#define IS_TKL_OTA_UPGRADE_IN_PROCESS(s) ((s) != TKL_OTA_INIT)

/**
 * @brief 升级开始通知函数
 * 
 * @param[in] file_size 升级固件大小
 * 
 * @retval  =0      成功
 * @retval  <0      错误码
 */
OPERATE_RET tkl_ota_start_notify(UINT_T image_size, TUYA_OTA_TYPE_E type, TUYA_OTA_PATH_E path)
{
    if (0 == image_size) {
        ESP_LOGE(DBG_TAG, "%s: invalid ota image size %d", __func__, image_size);
        return OPRT_INVALID_PARM;
    }

    if (IS_TKL_OTA_UPGRADE_IN_PROCESS(tkl_ota_handle.state)) {
        ESP_LOGE(DBG_TAG, "%s: ota upgrade in process", __func__);
        return OPRT_COM_ERROR;
    }

    TKL_OTA_UPGRADE_HANDLE_INIT(tkl_ota_handle);
    
    tkl_ota_handle.partition = (esp_partition_t *)esp_ota_get_next_update_partition(NULL);
    if (NULL == tkl_ota_handle.partition) {
        ESP_LOGE(DBG_TAG, "%s: get next upgrade partition failed", __func__);
        return OPRT_COM_ERROR;
    }

    if (ESP_OK != esp_ota_begin(tkl_ota_handle.partition, image_size, &tkl_ota_handle.handle)) {
        tkl_ota_handle.handle = 0;
        tkl_ota_handle.partition = NULL;
        ESP_LOGE(DBG_TAG, "%s: call esp_ota_begin failed", __func__);
        return OPRT_COM_ERROR;
    }

    tkl_ota_handle.image_size = image_size;
    tkl_ota_handle.state = TKL_OTA_BEGIN;

    return OPRT_OK;
}

/**
 * @brief ota数据包处理
 * 
 * @param[in] total_len ota升级包总大小
 * @param[in] offset 当前data在升级包中的偏移
 * @param[in] data ota数据buffer指针
 * @param[in] len ota数据buffer长度
 * @param[out] remain_len 内部已经下发但该函数还未处理的数据长度
 * @param[in] pri_data 保留参数
 *
 * @retval  =0      成功
 * @retval  <0      错误码
 */
OPERATE_RET tkl_ota_data_process(TUYA_OTA_DATA_T *pack, UINT_T* remain_len)
{
    if (NULL == pack || NULL == remain_len) {
        ESP_LOGE(DBG_TAG, "%s: input invalid params", __func__);
        return OPRT_INVALID_PARM;
    }

    //ESP_LOGI(DBG_TAG, "%s: total_len %u offset %u len %u state %d",
    //    __func__, pack->total_len, pack->offset, pack->len, tkl_ota_handle.state);

    if (TKL_OTA_INIT == tkl_ota_handle.state) {
        ESP_LOGE(DBG_TAG, "%s: ota upgrade is not init", __func__);
        return OPRT_COM_ERROR;
    }

    if (TKL_OTA_SUCCESS == tkl_ota_handle.state) {
        if (remain_len) {
            *remain_len = 0;
        }
        ESP_LOGI(DBG_TAG, "%s: ota process is successful", __func__);
        return OPRT_OK;
    }

    if (ESP_OK != esp_ota_write(tkl_ota_handle.handle, (const void *)pack->data, pack->len)) {
        ESP_LOGE(DBG_TAG, "%s: call esp_ota_write failed", __func__);
        return OPRT_COM_ERROR;
    }

    tkl_ota_handle.offset_image_size += pack->len;
    if (TKL_OTA_BEGIN == tkl_ota_handle.state) {
        tkl_ota_handle.state = TKL_OTA_IN_PROGRESS;
    }

    //ESP_LOGI(DBG_TAG, "%s: pack_len %d offset_image_size %d state %d remain_len %d",
    //    __func__, pack->len, tkl_ota_handle.offset_image_size, tkl_ota_handle.state, *remain_len);

    if (    (TKL_OTA_IN_PROGRESS == tkl_ota_handle.state)
         && (tkl_ota_handle.offset_image_size == tkl_ota_handle.image_size) ) {
        tkl_ota_handle.state = TKL_OTA_SUCCESS;
    }

    return OPRT_OK;
}

/**
 * @brief 固件ota数据传输完毕通知
 *        用户可以做固件校验以及设备重启
 * param[in]        reset       是否需要重启
 * @retval  =0      成功
 * @retval  <0      错误码
 */
OPERATE_RET tkl_ota_end_notify(BOOL_T reset)
{
    if (TKL_OTA_SUCCESS != tkl_ota_handle.state) {
        ESP_LOGE(DBG_TAG, "%s: ota upgrade state is not sucess", __func__);
        goto err_exit;
    }

    if (ESP_OK != esp_ota_end(tkl_ota_handle.handle)) {
        ESP_LOGE(DBG_TAG, "%s: call esp_ota_end failed", __func__);
        goto err_exit;
    }

    if (ESP_OK != esp_ota_set_boot_partition(tkl_ota_handle.partition)) {
        ESP_LOGE(DBG_TAG, "%s: call esp_ota_set_boot_partition failed", __func__);
        goto err_exit;
    }

    TKL_OTA_UPGRADE_HANDLE_INIT(tkl_ota_handle);

    if (reset) {
        ESP_LOGI(DBG_TAG, "%s: ota upgrade successful, system reboot now...", __func__);
        esp_restart();
    }
    return OPRT_OK;

err_exit:
    TKL_OTA_UPGRADE_HANDLE_INIT(tkl_ota_handle);
    return OPRT_COM_ERROR;
}

/**
* @brief get ota ability
*
* @param[out] image_size:  max image size
* @param[out] type:        ota type
*
* @note This API is used for get chip ota ability
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_ota_get_ability(UINT_T *image_size, UINT32_T *type)
{
    *image_size = 0xFFFFFFFF;
    *type = TUYA_OTA_FULL;

    return OPRT_OK;
}
