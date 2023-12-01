/**
 * @file tkl_bt.c
 * @brief bt操作接口
 * 
 * @copyright Copyright (c) {2018-2020} 涂鸦科技 www.tuya.com
 * 
 */
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_bt.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
#include "tkl_memory.h"
#include "tkl_system.h"
#include "tkl_output.h"
#include "tkl_bluetooth.h"
#include "tkl_hci.h"

#define DBG_TAG "TKL_BLE"
#define BLE_HCI_CMD_MSG_TYPE        0x01 	/* command message type */
#define BLE_HCI_ACL_MSG_TYPE        0x02    /* ACL data message type */
#define BLE_HCI_SYNC_MSG_TYPE       0x03    /* Synchronous data message type */
#define BLE_HCI_EVT_MSG_TYPE        0x04    /* event message type */

#define BLE_HCI_CMD_MSG_TYPE_LEN 1
#define BLE_HCI_ACL_MSG_TYPE_LEN 1

static TKL_HCI_FUNC_CB tuya_ble_hci_rx_cmd_hs_cb;
static TKL_HCI_FUNC_CB tuya_ble_hci_rx_acl_hs_cb;
#define TRACE_PACKET_DEBUG(t, buf, len) ESP_LOG_BUFFER_HEXDUMP((t), (buf), (len), ESP_LOG_INFO)

static void controller_rcv_pkt_ready(void)
{
	//ESP_LOGI(DBG_TAG, "%s: controller rcv pkt ready", __func__);
}

int tuya_ble_hci_ll_tx(UINT8_T *data, UINT16_T len)
{
	UINT8_T type;

	if (NULL == data || 0 == len) {
		ESP_LOGE(DBG_TAG, "%s: input invalid params", __func__);
		return -1;
	}

	type = data[0];
	//TRACE_PACKET_DEBUG("tuya_ble_hci_ll_tx", data, len);

	switch (type) {
	case BLE_HCI_EVT_MSG_TYPE:
		if (tuya_ble_hci_rx_cmd_hs_cb) {
			tuya_ble_hci_rx_cmd_hs_cb(data + BLE_HCI_CMD_MSG_TYPE_LEN, len - BLE_HCI_CMD_MSG_TYPE_LEN);
		}
		break;
	case BLE_HCI_ACL_MSG_TYPE:
		if (tuya_ble_hci_rx_acl_hs_cb) {
			tuya_ble_hci_rx_acl_hs_cb(data + BLE_HCI_ACL_MSG_TYPE_LEN, len - BLE_HCI_ACL_MSG_TYPE_LEN);
		}
		break;
	default:
		ESP_LOGI(DBG_TAG, "%s: unsupport message type %d", __func__, type);
		break;			
 	} /* end switch (type) { */

	return 0;
}

/**
 * Sends an HCI command from the host to the controller.
 *
 * @param cmd                   The HCI command to send.  This buffer must be
 *                                  allocated via tuya_ble_hci_buf_alloc().
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
OPERATE_RET tkl_hci_cmd_packet_send(CONST UCHAR_T *p_buf, USHORT_T buf_len)
{
	UCHAR_T *cmd_buf;
	USHORT_T cmd_buf_len;

	if (NULL == p_buf || 0 == buf_len) {
		ESP_LOGE(DBG_TAG, "%s: input invalid params", __func__);
		return OPRT_INVALID_PARM;
	}

	if (!esp_vhci_host_check_send_available()) {
		ESP_LOGE(DBG_TAG, "%s: hci send is not available", __func__);
		return OPRT_COM_ERROR;
	}

	cmd_buf_len = buf_len + BLE_HCI_CMD_MSG_TYPE_LEN;
	cmd_buf = tkl_system_malloc(cmd_buf_len);
	if (NULL == cmd_buf) {
		return OPRT_MALLOC_FAILED;
	}

	*cmd_buf = BLE_HCI_CMD_MSG_TYPE;
	memcpy(cmd_buf + BLE_HCI_CMD_MSG_TYPE_LEN, p_buf, buf_len);
	//TRACE_PACKET_DEBUG("tkl_hci_cmd_packet_send", cmd_buf, cmd_buf_len);

	esp_vhci_host_send_packet(cmd_buf, cmd_buf_len);
	tkl_system_free(cmd_buf);
	cmd_buf = NULL;
	return OPRT_OK;
}

/**
 * Sends ACL data from host to controller.
 *
 * @param om                    The ACL data packet to send.
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
OPERATE_RET tkl_hci_acl_packet_send(CONST UCHAR_T *p_buf, USHORT_T buf_len)
{
	UCHAR_T *acl_buf;
	USHORT_T acl_buf_len;

	if (NULL == p_buf || 0 == buf_len) {
		ESP_LOGE(DBG_TAG, "%s: input invalid params", __func__);
		return OPRT_INVALID_PARM;
	}

	if (!esp_vhci_host_check_send_available()) {
		ESP_LOGE(DBG_TAG, "%s: hci send is not available", __func__);
		return OPRT_COM_ERROR;
	}

	acl_buf_len = buf_len + BLE_HCI_ACL_MSG_TYPE_LEN;
	acl_buf = tkl_system_malloc(acl_buf_len);
	if (NULL == acl_buf) {
		return OPRT_MALLOC_FAILED;
	}

	*acl_buf = BLE_HCI_ACL_MSG_TYPE;
	memcpy(acl_buf + BLE_HCI_ACL_MSG_TYPE_LEN, p_buf, buf_len);

	//TRACE_PACKET_DEBUG("tkl_hci_acl_packet_send", acl_buf, acl_buf_len);
	esp_vhci_host_send_packet(acl_buf, acl_buf_len);
	tkl_system_free(acl_buf);
	acl_buf = NULL;
	return OPRT_OK;
}

OPERATE_RET tkl_hci_callback_register(CONST TKL_HCI_FUNC_CB hci_evt_cb, CONST TKL_HCI_FUNC_CB acl_pkt_cb)
{
    tuya_ble_hci_rx_cmd_hs_cb = hci_evt_cb;
    tuya_ble_hci_rx_acl_hs_cb = acl_pkt_cb;
	return OPRT_OK;
}

static esp_vhci_host_callback_t vhci_host_cb = {
    .notify_host_send_available = controller_rcv_pkt_ready,
    .notify_host_recv = tuya_ble_hci_ll_tx
};

/**
 * Resets the HCI module to a clean state.  Frees all buffers and reinitializes
 * the underlying transport.
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
OPERATE_RET tkl_hci_reset(void)
{
	//UINT16_T size; 
	//UINT8_T cmd_buf[128] = { 0 };

	if (!esp_vhci_host_check_send_available()) {
		ESP_LOGE(DBG_TAG, "%s: hci send is not available", __func__);
		return OPRT_COM_ERROR;
	}

	//size = make_cmd_reset(cmd_buf);
	//esp_vhci_host_send_packet(cmd_buf, size);
	return OPRT_OK;
}

/**
 * Init the HCI module
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
OPERATE_RET tkl_hci_init(void)
{
	esp_err_t ret;
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

	ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
	if (ESP_OK != ret) {
		ESP_LOGE(DBG_TAG, "%s: call esp_bt_controller_mem_release failed(ret=%d)", __func__, ret);
		return OPRT_COM_ERROR;
	}

	ret = esp_bt_controller_init(&bt_cfg);
	if (ESP_OK != ret) {
		ESP_LOGE(DBG_TAG, "%s: call esp_bt_controller_init failed(ret=%d)", __func__, ret);
		return OPRT_COM_ERROR;
	}

	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ESP_OK != ret) {
		ESP_LOGE(DBG_TAG, "%s: call esp_bt_controller_enable failed(ret=%d)", __func__, ret);
		return OPRT_COM_ERROR;
	}

	esp_vhci_host_register_callback(&vhci_host_cb);
	return OPRT_OK;
}

/* Deinit the controller and hci interface*/
/* Host Use */
OPERATE_RET tkl_hci_deinit(void)
{
	esp_err_t ret;
    
	ret = esp_bt_controller_disable();
	if (ESP_OK != ret) {
		ESP_LOGE(DBG_TAG, "%s: call esp_bt_controller_disable failed", __func__);
		return OPRT_COM_ERROR;
	}

    ret = esp_bt_controller_deinit();
	if (ESP_OK != ret) {
		ESP_LOGE(DBG_TAG, "%s: call esp_bt_controller_deinit failed", __func__);
		return OPRT_COM_ERROR;
	}

    return OPRT_OK;
}
