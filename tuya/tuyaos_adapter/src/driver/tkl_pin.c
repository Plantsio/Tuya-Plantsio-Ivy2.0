/**
 * @file tkl_pin.c
 * @brief gpio操作接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
#include "tkl_system.h"
#include "tkl_memory.h"
#include "tkl_gpio.h"

#define DBG_TAG "TKL_PIN"

#define PIN_DEV_CHECK_ERROR_RETURN(__PIN, __ERROR)                          \
    if (__PIN >= sizeof(pinmap)/sizeof(pinmap[0])) {                        \
        return __ERROR;                                                     \
    }

typedef void (*tuya_pin_irq_cb)(void *args);

typedef struct {
    int gpio;
	tuya_pin_irq_cb cb;
    void *args;
} pin_dev_map_t;

static pin_dev_map_t pinmap[] = {
    {GPIO_NUM_0,  NULL, NULL}, {GPIO_NUM_1,  NULL, NULL}, {GPIO_NUM_2,  NULL, NULL}, {GPIO_NUM_3,  NULL, NULL}, 
    {GPIO_NUM_4,  NULL, NULL}, {GPIO_NUM_5,  NULL, NULL}, {GPIO_NUM_6,  NULL, NULL}, {GPIO_NUM_7,  NULL, NULL}, 
    {GPIO_NUM_8,  NULL, NULL}, {GPIO_NUM_9,  NULL, NULL}, {GPIO_NUM_10, NULL, NULL}, {GPIO_NUM_11, NULL, NULL}, 
    {GPIO_NUM_12, NULL, NULL}, {GPIO_NUM_13, NULL, NULL}, {GPIO_NUM_14, NULL, NULL}, {GPIO_NUM_15, NULL, NULL}, 
    {GPIO_NUM_16, NULL, NULL}, {GPIO_NUM_17, NULL, NULL}, {GPIO_NUM_18, NULL, NULL}, {GPIO_NUM_19, NULL, NULL}, 
    {GPIO_NUM_20, NULL, NULL}, {GPIO_NUM_21, NULL, NULL}, {GPIO_NUM_26, NULL, NULL}, {GPIO_NUM_27, NULL, NULL},
    {GPIO_NUM_28, NULL, NULL}, {GPIO_NUM_29, NULL, NULL}, {GPIO_NUM_30, NULL, NULL}, {GPIO_NUM_31, NULL, NULL},
    {GPIO_NUM_32, NULL, NULL}, {GPIO_NUM_33, NULL, NULL}, {GPIO_NUM_34, NULL, NULL}, {GPIO_NUM_35, NULL, NULL},
    {GPIO_NUM_36, NULL, NULL}, {GPIO_NUM_37, NULL, NULL}, {GPIO_NUM_38, NULL, NULL}, {GPIO_NUM_39, NULL, NULL},
#if defined(ESP32S3) || defined(ESP32S2)
    {GPIO_NUM_40, NULL, NULL}, {GPIO_NUM_41, NULL, NULL}, {GPIO_NUM_42, NULL, NULL}, {GPIO_NUM_43, NULL, NULL},
    {GPIO_NUM_44, NULL, NULL}, {GPIO_NUM_45, NULL, NULL}, {GPIO_NUM_46, NULL, NULL}, 
#if defined(ESP32S3)
    {GPIO_NUM_47, NULL, NULL}, {GPIO_NUM_48, NULL, NULL}
#endif
#endif
};

OPERATE_RET tkl_gpio_init(UINT32_T pin_id, CONST TUYA_GPIO_BASE_CFG_T *cfg)
{
    esp_err_t ret;
    int gpio_num;
    gpio_mode_t gpio_dir;
    gpio_pull_mode_t pull_type;

    if (NULL == cfg) {
        return OPRT_INVALID_PARM;
    }

    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
	gpio_num = pinmap[pin_id].gpio;

    gpio_reset_pin(gpio_num);
    gpio_set_level(gpio_num, cfg->level);

    gpio_dir = GPIO_MODE_DISABLE; 
    switch (cfg->direct)  {
    case TUYA_GPIO_INPUT:
        gpio_dir = GPIO_MODE_INPUT;
        break;
    case TUYA_GPIO_OUTPUT:
        gpio_dir = GPIO_MODE_OUTPUT;
        break;
    default:
        return OPRT_NOT_SUPPORTED;
    }
    ret = gpio_set_direction(gpio_num, gpio_dir);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: call gpio_set_direction failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

	pull_type = GPIO_FLOATING; 
    switch (cfg->mode) {
    case TUYA_GPIO_PULLUP:
		pull_type = GPIO_PULLUP_ONLY;
        break;
    case TUYA_GPIO_PULLDOWN:
		pull_type = GPIO_PULLDOWN_ONLY;
        break;
    default:
        return OPRT_NOT_SUPPORTED;
    }
    ret = gpio_set_pull_mode(gpio_num, pull_type);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: call gpio_set_pull_mode failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_gpio_write(UINT32_T pin_id, TUYA_GPIO_LEVEL_E level)
{
    int gpio_num;

    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    gpio_num = pinmap[pin_id].gpio;
    gpio_set_level(gpio_num, level);

    return OPRT_OK;    
}

OPERATE_RET tkl_gpio_read(UINT32_T pin_id, TUYA_GPIO_LEVEL_E *level)
{
    int gpio_num;

    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    gpio_num = pinmap[pin_id].gpio;
    
    *level = gpio_get_level(gpio_num);
    return OPRT_OK;
}

/**
 * @brief gpio irq init
 * NOTE: call this API will not enable interrupt
 * 
 * @param[in] port: gpio port 
 * @param[in] cfg:  gpio irq config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_init(UINT32_T pin_id, CONST TUYA_GPIO_IRQ_T *cfg)
{
    int gpio_num;
    int trigger;
    esp_err_t ret;

    if (NULL == cfg) {
        return OPRT_INVALID_PARM;
    }

    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);

    pinmap[pin_id].cb = cfg->cb;
    pinmap[pin_id].args = cfg->arg;
	gpio_num = pinmap[pin_id].gpio;	

    switch (cfg->mode) {
    case TUYA_GPIO_IRQ_RISE:
        trigger = GPIO_INTR_POSEDGE; 
        break;
    case TUYA_GPIO_IRQ_FALL:
        trigger = GPIO_INTR_NEGEDGE; 
        break;
    case TUYA_GPIO_IRQ_RISE_FALL:
        trigger = GPIO_INTR_ANYEDGE;
        break;    
    case TUYA_GPIO_IRQ_LOW:
        trigger = GPIO_INTR_LOW_LEVEL; 
        break;
    case TUYA_GPIO_IRQ_HIGH:
        trigger = GPIO_INTR_HIGH_LEVEL; 
        break;
    default: 
        return OPRT_NOT_SUPPORTED;
    } /* end switch (cfg->mode) { */

    ret = gpio_set_intr_type(gpio_num, trigger);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: call gpio_set_intr_type failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    ret = gpio_set_direction(gpio_num, GPIO_MODE_INPUT);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: call gpio_set_direction failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    ret = gpio_install_isr_service(0);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: call gpio_install_isr_service failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    ret = gpio_isr_handler_add(gpio_num, pinmap[pin_id].cb, pinmap[pin_id].args);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: call gpio_isr_handler_add failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief gpio irq enable
 * 
 * @param[in] port: gpio port 
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_enable(UINT32_T pin_id)
{
    int gpio_num;
    esp_err_t ret;

    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    gpio_num = pinmap[pin_id].gpio;
    ret = gpio_intr_enable(gpio_num);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: call gpio_intr_enable failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief gpio irq disable
 * 
 * @param[in] port: gpio port 
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_disable(UINT32_T pin_id)
{
    int gpio_num;

    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    gpio_num = pinmap[pin_id].gpio;
    
    gpio_intr_disable(gpio_num);
    
    return OPRT_OK;    
}

OPERATE_RET tkl_gpio_deinit(UINT32_T pin_id)
{
    int gpio_num;
    
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    gpio_num = pinmap[pin_id].gpio;

    gpio_isr_handler_remove(gpio_num);
    gpio_uninstall_isr_service();
    gpio_reset_pin(gpio_num);
    
    return OPRT_OK;
}
