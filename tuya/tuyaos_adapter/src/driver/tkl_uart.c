/**
 * @file tkl_uart.c
 * @brief uart操作接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#define ESP_UART_SUPPORT 0
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"

#include "esp_err.h"
#include "esp_log.h"
#include "hal/uart_types.h"
#include "hal/uart_hal.h"
#include "hal/gpio_hal.h"
#if ESP_UART_SUPPORT
#include "hal/clk_tree_ll.h"
#endif
#include "soc/uart_periph.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/uart_select.h"
#include "freertos/portmacro.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
#include "tkl_system.h"
#include "tkl_memory.h"
#include "tkl_uart.h"
#include "tkl_output.h"

//#define MAX_UART_NUM UART_NUM_MAX
#define MAX_UART_NUM 2

#ifdef ESP32S3
#define TKL_UART_NUM_0_TXD  (GPIO_NUM_43)
#define TKL_UART_NUM_0_RXD  (GPIO_NUM_44)
#else
#define TKL_UART_NUM_0_TXD  (GPIO_NUM_33)
#define TKL_UART_NUM_0_RXD  (GPIO_NUM_34)
#endif

#define TKL_UART_NUM_0_RTS  (UART_PIN_NO_CHANGE)
#define TKL_UART_NUM_0_CTS  (UART_PIN_NO_CHANGE)

#define TKL_UART_NUM_1_TXD  (GPIO_NUM_17)
#define TKL_UART_NUM_1_RXD  (GPIO_NUM_18)
#define TKL_UART_NUM_1_RTS  (UART_PIN_NO_CHANGE)
#define TKL_UART_NUM_1_CTS  (UART_PIN_NO_CHANGE)

// #define TKL_UART_NUM_2_TXD  (GPIO_NUM_4)
// #define TKL_UART_NUM_2_RXD  (GPIO_NUM_5)
// #define TKL_UART_NUM_2_RTS  (UART_PIN_NO_CHANGE)
// #define TKL_UART_NUM_2_CTS  (UART_PIN_NO_CHANGE)

// uart_port_t uart_port_num[MAX_UART_NUM];
// unsigned int uart_base_reg[MAX_UART_NUM];
TaskHandle_t tkl_uart_thread = NULL;
TUYA_UART_IRQ_CB uart_rx_cb[MAX_UART_NUM];
QueueHandle_t tkl_uart_rx_queue = NULL;

#define DBG_TAG "TKL_UART"

static void tkl_uart_rx_process(void *args)
{
    uart_port_t uart_num = UART_NUM_0;
    uart_event_t event;
    
    assert(NULL != args);
    uart_num = *(uart_port_t *)args;

    while (tkl_uart_rx_queue && xQueueReceive(tkl_uart_rx_queue, (void * )&event, portMAX_DELAY)) {
        switch (event.type) {
        case UART_DATA:
            if (uart_rx_cb[uart_num]) {
                uart_rx_cb[uart_num](uart_num);
            }
            break;
        case UART_BREAK:
            break;
        case UART_BUFFER_FULL:
            break;
        case UART_FIFO_OVF:
            break;
        case UART_FRAME_ERR:
            break;
        case UART_PARITY_ERR:
            break;
        case UART_DATA_BREAK:
            break;
        case UART_PATTERN_DET:
            break;
        default:
            break;                
        } /* switch (event.type) { */
    }
    vTaskDelete(NULL);
}

/**
 * @brief 用于初始化串口
 * 
 * @param[in]  uart     串口句柄
 * @param[in]  cfg      串口配置结构体
 */
OPERATE_RET tkl_uart_init(TUYA_UART_NUM_E port, TUYA_UART_BASE_CFG_T *cfg)
{
    esp_err_t ret;
	uart_port_t uart_num;
	uart_config_t uart_cfg;
    int intr_alloc_flags = 0, uart_txd, uart_rxd, uart_rts, uart_cts;
    
    if (NULL == cfg) {
        return OPRT_INVALID_PARM;
    }

    uart_num = (uart_port_t)port;
    if (uart_num >= MAX_UART_NUM) {
        return OPRT_INVALID_PARM;
    }

    if (TUYA_UART_DATA_LEN_5BIT == cfg->databits) {
        uart_cfg.data_bits = UART_DATA_5_BITS;
    } else if (TUYA_UART_DATA_LEN_6BIT == cfg->databits) {
        uart_cfg.data_bits = UART_DATA_6_BITS;
    } else if (TUYA_UART_DATA_LEN_7BIT ==  cfg->databits) {
        uart_cfg.data_bits = UART_DATA_7_BITS;
    } else if (TUYA_UART_DATA_LEN_8BIT == cfg->databits) {
        uart_cfg.data_bits = UART_DATA_8_BITS;
    } else {
        return OPRT_INVALID_PARM;
    }

    if (TUYA_UART_STOP_LEN_1BIT == cfg->stopbits) {
        uart_cfg.stop_bits = UART_STOP_BITS_1;
    } else if (TUYA_UART_STOP_LEN_2BIT == cfg->stopbits) {
        uart_cfg.stop_bits = UART_STOP_BITS_2;
    } else if (TUYA_UART_STOP_LEN_1_5BIT1 == cfg->stopbits) {
        uart_cfg.stop_bits = UART_STOP_BITS_1_5;
    } else {
        return OPRT_INVALID_PARM;
    }

    if (TUYA_UART_PARITY_TYPE_NONE == cfg->parity) {
        uart_cfg.parity = UART_PARITY_DISABLE;
    } else if (TUYA_UART_PARITY_TYPE_EVEN == cfg->parity) {
        uart_cfg.parity = UART_PARITY_EVEN;
    } else if (TUYA_UART_PARITY_TYPE_ODD == cfg->parity) {
        uart_cfg.parity = UART_PARITY_ODD;
    } else {
        return OPRT_INVALID_PARM;
    }

    uart_cfg.baud_rate = cfg->baudrate;
    uart_cfg.rx_flow_ctrl_thresh = 122;
	uart_cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
	//uart_cfg.source_clk = UART_SCLK_DEFAULT;
#ifdef ESP32S3    
    uart_cfg.source_clk = UART_SCLK_XTAL;
#else
    uart_cfg.source_clk = UART_SCLK_APB;
#endif
    if (UART_NUM_1 == uart_num) {
        uart_txd = TKL_UART_NUM_1_TXD;
        uart_rxd = TKL_UART_NUM_1_RXD;
        uart_cts = TKL_UART_NUM_1_CTS;
        uart_rts = TKL_UART_NUM_1_RTS;
    } else {
        uart_txd = TKL_UART_NUM_0_TXD;
        uart_rxd = TKL_UART_NUM_0_RXD;
        uart_cts = TKL_UART_NUM_0_CTS;
        uart_rts = TKL_UART_NUM_0_RTS;
    }

    ret = uart_param_config(uart_num, &uart_cfg);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: call uart_param_config failed(uart_num=%d,ret=%d)", __func__, uart_num, ret);
        return OPRT_COM_ERROR;
    }

    ret = uart_set_pin(uart_num, uart_txd, uart_rxd, uart_rts, uart_cts);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: call uart_set_pin failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }
    
    ret = uart_driver_install(uart_num, 256, 0, 10, &tkl_uart_rx_queue, intr_alloc_flags);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: call uart_driver_install failecd(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief uart deinit
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_deinit(TUYA_UART_NUM_E port)
{
    uart_port_t uart_num = (uart_port_t)port;
    
    assert(uart_num < MAX_UART_NUM);

    if (ESP_OK != uart_disable_rx_intr(uart_num)) {
        ESP_LOGE(DBG_TAG, "%s: call uart_disable_rx_intr failed", __func__);
        //return OPRT_COM_ERROR;
    }

    if (ESP_OK != uart_driver_delete(uart_num)) {
        //ESP_LOGE(DBG_TAG, "%s: call uart_driver_delete failed", __func__);
        return OPRT_COM_ERROR;
    }

    if (NULL != tkl_uart_thread) {
        vTaskDelete(tkl_uart_thread);
        tkl_uart_thread = NULL;
    }
    tkl_uart_rx_queue = NULL;

    return OPRT_OK;
}


/**
 * @brief 用于发送一个字节
 * 
 * @param[in]  uart     串口句柄
 * @param[in]  byte     要发送的字节
 */
INT_T tkl_uart_write(TUYA_UART_NUM_E port, VOID_T *buff, UINT16_T len)
{
    int ret;
    uart_port_t uart_num = (uart_port_t)port; 

    assert(uart_num < MAX_UART_NUM);
    if (NULL == buff || 0 == len) {
        return OPRT_INVALID_PARM;
    }

    ret = uart_write_bytes(uart_num, buff, len);
    if (ret < 0) {
        ESP_LOGI(DBG_TAG, "%s: call uart_write_bytes failed(ret=%d)", __func__, ret);
    }

    return ret;
}

/**
 * @brief uart read data
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[out] buff: read data
 * @param[in] len:  buff len
 * 
 * @return return >= 0: number of data read; return < 0: read errror
 */
INT_T tkl_uart_read(TUYA_UART_NUM_E port, VOID_T *buff, UINT16_T len)
{
    uart_port_t uart_num = (uart_port_t)port;

    assert(uart_num < MAX_UART_NUM);
    if (NULL == buff || 0 == len) {
        return OPRT_INVALID_PARM;
    }

    return  uart_read_bytes(uart_num, buff, 1, 0);
}

/**
 * @brief enable uart rx interrupt and regist interrupt callback
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] rx_cb: receive callback
 *
 * @return none
 */
VOID tkl_uart_rx_irq_cb_reg(TUYA_UART_NUM_E port, TUYA_UART_IRQ_CB rx_cb)
{
    BaseType_t ret;
    uart_port_t  uart_num = (uart_port_t)port; 
    assert(port < MAX_UART_NUM);

    if (NULL == tkl_uart_rx_queue) {
        ESP_LOGE(DBG_TAG, "%s: call tkl_uart_rx_queue is null", __func__);
        return;
    }

    if (NULL != tkl_uart_thread) {
        //ESP_LOGE(DBG_TAG,"%s: delete thread tkl_uart_thread %p", __func__, tkl_uart_thread);
        vTaskDelete(tkl_uart_thread);
        tkl_uart_thread = NULL;
    }


    ret = xTaskCreate(tkl_uart_rx_process, "UART_RX_PROC", 4096, &uart_num, 4, &tkl_uart_thread);
    if (ret != pdPASS) {
        ESP_LOGE(DBG_TAG, "%s: call xTaskCreate crate TKL_UART_RX_PROCESS thread failed", __func__);
        return;
    }

    //ESP_LOGE(DBG_TAG,"%s: cread thread tkl_uart_thread %p", __func__, tkl_uart_thread);
    uart_enable_rx_intr(uart_num);
    uart_rx_cb[uart_num] = rx_cb;
}

/**
 * @brief set uart transmit interrupt status
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] enable: TRUE-enalbe tx int, FALSE-disable tx int
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_set_tx_int(TUYA_UART_NUM_E port, BOOL_T enable)
{
    esp_err_t ret;
    uart_port_t  uart_num = (uart_port_t)port; 
    assert(port < MAX_UART_NUM);
    if (enable) {
        ret = uart_enable_tx_intr(uart_num, 1, 10); /* UART_EMPTY_THRESH_DEFAULT */
    } else {
        ret = uart_disable_tx_intr(uart_num);
    }
    return (ESP_OK == ret ? OPRT_OK : OPRT_COM_ERROR);
}

/**
 * @brief regist uart tx interrupt callback
 * If this function is called, it indicates that the data is sent asynchronously through interrupt,
 * and then write is invoked to initiate asynchronous transmission.
 *  
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] rx_cb: receive callback
 *
 * @return none
 */
VOID tkl_uart_tx_irq_cb_reg(TUYA_UART_NUM_E port, TUYA_UART_IRQ_CB tx_cb)
{
    port = port;
    tx_cb = tx_cb;
}

