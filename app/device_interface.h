//
// Created by Huwensong on 2022/12/27.
//

#ifndef TUYA_PLANTS_IVY_DEVICE_INTERFACE_H
#define TUYA_PLANTS_IVY_DEVICE_INTERFACE_H

#include "tuya_cloud_com_defs.h"
#include "tuya_iot_wifi_api.h"
#include "common/definitions.h"

#ifdef _TUYA_DEVICE_GLOBAL
#define _TUYA_DEVICE_EXT
#else
#define _TUYA_DEVICE_EXT extern
#endif /* _TUYA_DEVICE_GLOBAL */

/***********************************************************
*************************micro define***********************
***********************************************************/
/* device information define */
#define APP_BIN_NAME "Tuya-Plants-Ivy"

#define DEV_SW_VERSION   SOFTWARE_VER

#define PRODUCT_ID      "8n3q0y4cwov8ifxt"
#define FIRMWARE_KEY    "8n3q0y4cwov8ifxt"

/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/
enum Dp {
    brightness = 101,
    has_pot = 102,
    ambient_light = 103,
    temperature = 104,
    humidity = 105,
    watering_status = 106,
    ip = 107,
    water_level = 108,
    charging = 109,
    touch_enable = 110,
    touching = 111,
    color = 113,
    ear_sensitivity = 115,
    pot_sensitivity = 116,
    evolve_status = 117,
    sys_cmd = 118,
    plant_type = 119,
    battery_percent = 121,
    soil_moisture = 122,
    plant_auto_detect = 123,
    water_manual_calib = 124,
    ear_enable = 125,
    pomodoro = 126,
    left_touch_switch = 127,
    right_touch_switch = 128,
    plant_touch_switch = 129,
    use_fahrenheit = 130,
    enable_night_sleep = 131,
    enable_sleep = 132,
    night_sleep_start = 133,
    night_sleep_period = 134,
    auto_brightness = 135,
    use_12h_mode = 136,
    custom_anim_last_t = 137,
    light_status = 138,
    temperature_status = 139,
    humidity_status = 140,
    enable_location_weather = 141,
    longitude = 142,
    latitude = 143,
    language = 144,
    ambient_light_history = 145,
    battery_percent_history = 146,
    soil_moisture_history = 147,
    humidity_history = 148,
    temperature_history = 149,
    water_level_history = 150,
    soil_status = 151,
    tu_over = 152,
};

enum sys_cmd {
    SYS_CMD_NONE = 0,
    SYS_CMD_SYNC_BT = 1,
//        SYS_CMD_EVOLVE = 2,
    SYS_CMD_CALIB_POT_IN = 3,
    SYS_CMD_CALIB_POT_OFF = 4,
    SYS_CMD_FLUID_CALIB_RESET = 5,
    SYS_CMD_FLUID_CALIB_EXE = 6,
    SYS_CMD_TOUCH_CALIB = 7,
    SYS_CMD_CHECK_EVOLVE = 8,
    SYS_CMD_UI_APPLY = 9,
    SYS_CMD_UI_CANCEL = 10,
    SYS_CMD_RE_TUTORIAL = 11,
    SYS_CMD_RESET_SOIL_CALIB_DATA = 12,
    SYS_CMD_FRONT_TOUCH = 13,
    SYS_CMD_LEFT_TOUCH = 14,
    SYS_CMD_RIGHT_TOUCH = 15,
    SYS_CMD_UPLOAD_LOG = 16,
    SYS_CMD_ANIM_PLAY = 17,
    SYS_CMD_ANIM_STOP = 18,
    SYS_CMD_ANIM_NEXT = 19,
    SYS_CMD_ANIM_SINGLE = 20,
    SYS_CMD_ANIM_ROTATE = 21,
    SYS_CMD_ANIM_RANDOM = 22,
    SYS_CMD_RESTORE_TO_FACTORY = 99,
};

typedef struct DpType {
    enum Dp dp;
    DP_PROP_TP_E type;
    UINT8_T factor;
} dp_type_t;

/***********************************************************
***********************function define**********************
***********************************************************/

#ifdef __cplusplus
extern "C" {
#endif

VOID mf_user_pre_gpio_test_cb(VOID);

VOID mf_user_callback(VOID);

VOID mf_user_enter_callback(VOID);

OPERATE_RET
mf_user_product_test_cb(USHORT_T cmd, UCHAR_T *data, UINT_T len, OUT UCHAR_T **ret_data, OUT USHORT_T *ret_len);

VOID pre_app_init(VOID);

VOID pre_device_init(VOID);

VOID app_init(VOID);

VOID hw_report_all_dp_status(VOID);

VOID upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID_T pri_data);

OPERATE_RET get_file_data_cb(IN CONST FW_UG_S *fw, IN CONST UINT_T total_len, IN CONST UINT_T offset,
                             IN CONST BYTE_T *data, IN CONST UINT_T len, OUT UINT_T *remain_len, IN PVOID_T pri_data);

INT_T gw_ug_inform_cb(IN CONST FW_UG_S *fw);

VOID gw_reset_cb(IN CONST GW_RESET_TYPE_E type);

VOID dev_obj_dp_cb(IN CONST TY_RECV_OBJ_DP_S *dp);

VOID dev_raw_dp_cb(IN CONST TY_RECV_RAW_DP_S *dp);

VOID wf_nw_status_cb(IN CONST GW_WIFI_NW_STAT_E stat);

VOID status_changed_cb(IN CONST GW_STATUS_E status);

OPERATE_RET device_init(VOID);

int tuya_dev_init_event_cb(void *);

bool chang_partition();

bool not_find_tuya_patition();

bool running_in_app1();

void update_partition_table();

void ota_update_partition_table();

void ota_ui_start();

void ota_ui_progress(unsigned int progress);

void ota_ui_end(bool success);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //TUYA_PLANTS_IVY_DEVICE_INTERFACE_H
