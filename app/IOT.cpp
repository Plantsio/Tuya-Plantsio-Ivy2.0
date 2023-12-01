//
// Created by Gordon on 2022/11/23.
//

#include "IOT.h"
#include "Arduino.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "Njson.hpp"
#include "tool/OTA.h"
#include "common/Sys.h"
#include "Engine/IvyEngine.h"
#include "tool/IvyTest.h"
#include "Arduino.h"
#include "Network/DownloadManager.h"
#include "Body/Skin.h"
#include "Body/Sense.h"
#include "Engine/Behavior/IvyBehavior.h"
#include "helpers/EvolveUpdateHelper.h"
#include "Anim/ui/tools.h"
#include "interface/fat_interface.h"
#include <fstream>
#include "Anim/drivers/LvglDriver.h"
#include "common/definitions.h"
#include "Body/Plant.h"
#include "Anim/drivers/CustomAnimPlayer.h"
#include "common/filepath.h"
#include "Anim/IvyAnim.h"
#include "Anim/lang/Lang.h"
#include "common/Boot.h"
#include "tool/system.h"


//SDK includes
#include "tuya_error_code.h"
#include "tuya_iot_wifi_api.h"
#include "ws_db_gw.h"
#include "base_event.h"
#include "device_interface.h"
#include "tuya_os_adapt_wifi.h"
#include "tuya_uf_db.h"
#include "tool/system.h"

#define TAG "IOT"

IOT &IOT::instance() {
    static std::shared_ptr<IOT> instance(new IOT());
    return *instance;
}

IOT::IOT() {
    init_dp_data();
}

void IOT::init_dp_data() {
    tuya_dp_map = {
            {Prop::brightness,              {brightness,              PROP_VALUE, 1}},
            {Prop::has_pot,                 {has_pot,                 PROP_BOOL,  1}},
            {Prop::ambient_light,           {ambient_light,           PROP_VALUE, 1}},
            {Prop::temperature,             {temperature,             PROP_VALUE, 10}},
            {Prop::humidity,                {humidity,                PROP_VALUE, 1}},
            {Prop::charging,                {charging,                PROP_BOOL,  1}},
            {Prop::water_level,             {water_level,             PROP_VALUE, 1}},
            {Prop::touch_enable,            {touch_enable,            PROP_BOOL,  1}},
            {Prop::ear_sensitivity,         {ear_sensitivity,         PROP_VALUE, 1}},
            {Prop::pot_sensitivity,         {pot_sensitivity,         PROP_VALUE, 1}},
            {Prop::evolve_status,           {evolve_status,           PROP_VALUE, 1}},
            {Prop::battery_percent,         {battery_percent,         PROP_VALUE, 1}},
            {Prop::soil_moisture,           {soil_moisture,           PROP_VALUE, 1}},
            {Prop::plant_auto_detect,       {plant_auto_detect,       PROP_BOOL,  1}},
            {Prop::water_manual_calib,      {water_manual_calib,      PROP_BOOL,  1}},
            {Prop::ear_enable,              {ear_enable,              PROP_BOOL,  1}},
            {Prop::plant_type,              {plant_type,              PROP_VALUE, 1}},
            {Prop::color,                   {color,                   PROP_VALUE, 1}},
            {Prop::pomodoro,                {pomodoro,                PROP_VALUE, 1}},
            {Prop::watering_status,         {watering_status,         PROP_VALUE, 1}},
            {Prop::use_fahrenheit,          {use_fahrenheit,          PROP_BOOL,  1}},
            {Prop::enable_night_sleep,      {enable_night_sleep,      PROP_BOOL,  1}},
            {Prop::enable_sleep,            {enable_sleep,            PROP_BOOL,  1}},
            {Prop::night_sleep_start,       {night_sleep_start,       PROP_VALUE, 1}},
            {Prop::night_sleep_period,      {night_sleep_period,      PROP_VALUE, 1}},
            {Prop::auto_brightness,         {auto_brightness,         PROP_BOOL,  1}},
            {Prop::use_12h_mode,            {use_12h_mode,            PROP_BOOL,  1}},
            {Prop::custom_anim_last_t,      {custom_anim_last_t,      PROP_VALUE, 1}},
            {Prop::light_status,            {light_status,            PROP_VALUE, 1}},
            {Prop::temperature_status,      {temperature_status,      PROP_VALUE, 1}},
            {Prop::humidity_status,         {humidity_status,         PROP_VALUE, 1}},
            {Prop::soil_status,             {soil_status,             PROP_VALUE, 1}},
            {Prop::enable_location_weather, {enable_location_weather, PROP_BOOL,  1}},
            {Prop::longitude,               {longitude,               PROP_VALUE, 1}},
            {Prop::latitude,                {latitude,                PROP_VALUE, 1}},
            {Prop::language,                {language,                PROP_VALUE, 1}},
            {Prop::ambient_light_history,   {ambient_light_history,   PROP_VALUE, 1}},
            {Prop::battery_percent_history, {battery_percent_history, PROP_VALUE, 1}},
            {Prop::soil_moisture_history,   {soil_moisture_history,   PROP_VALUE, 1}},
            {Prop::humidity_history,        {humidity_history,        PROP_VALUE, 1}},
            {Prop::temperature_history,     {temperature_history,     PROP_VALUE, 10}},
            {Prop::water_level_history,     {water_level_history,     PROP_VALUE, 1}},
            {Prop::tutorial_complete,       {tu_over,                 PROP_VALUE, 1}},

    };


}

void IOT::begin() {
    WiFi.persistent(false);
    WiFi.enableSTA(true);
    xTaskCreatePinnedToCore(main_routine_wrapper, "IOT", 8192, this, 3, nullptr, 1);
}

bool IOT::dp_handler(Dp dp, int value) {
    switch (dp) {
        case brightness:
            Prop::set(Prop::brightness, value, true, false, true);
            break;
        case has_pot:
            Prop::set(Prop::manual_has_pot, value, true, false, true);
            break;
        case touch_enable:
            Prop::set(Prop::touch_enable, value, true, false, true);
            break;
        case ear_sensitivity:
            Prop::set(Prop::ear_sensitivity, value, true, false, true);
            break;
        case pot_sensitivity:
            Prop::set(Prop::pot_sensitivity, value, true, false, true);
            break;
        case evolve_status:
            return false;
            break;
        case sys_cmd:
            process_sys_cmd(value);
            return false;
        case plant_auto_detect:
            Prop::set(Prop::plant_auto_detect, value, true, false, true);
            break;
        case water_manual_calib:
            if (value && Prop::get<bool>(Prop::has_pot)) {
                /* set water manual calibration but pot is in */
                Prop::set(Prop::water_manual_calib,false,true,false);
                UI::show_temp_text(LANG(Lang::ui_manual_calib_remove_pot));
                return false;
            } else {
                Sense::instance().set_level_calibration(value, true);
            }
            break;
        case ear_enable:
            Prop::set(Prop::ear_enable, value, true, false, true);
            break;
        case plant_type:
            Prop::set(Prop::plant_type, value, true, false, true);
            break;
        case enable_sleep:
            Prop::set(Prop::enable_sleep, value, true, false, true);
            break;
        case auto_brightness:
            Prop::set(Prop::auto_brightness, value, true, false, true);
            break;
        case enable_night_sleep:
            Prop::set(Prop::enable_night_sleep, value, true, false, true);
            break;
        case use_fahrenheit:
            Prop::set(Prop::use_fahrenheit, value, true, false, true);
            break;
        case use_12h_mode:
            Prop::set(Prop::use_12h_mode, value, true, false, true);
            break;
        case night_sleep_start:
            Prop::set(Prop::night_sleep_start, value, true, false, true);
            break;
        case night_sleep_period:
            Prop::set(Prop::night_sleep_period, value, true, false, true);
            break;
        case custom_anim_last_t:
            Prop::set(Prop::custom_anim_last_t, value, true, false, true);
            break;
        case enable_location_weather:
            Prop::set(Prop::enable_location_weather, value, true, false, true);
            break;
        case longitude:
            Prop::set(Prop::longitude, value, true, false, true);
            break;
        case latitude:
            Prop::set(Prop::latitude, value, true, false, true);
            break;
        case language:
            if (!Prop::get<int>(Prop::language_changed))
            {
                Prop::set(Prop::language_changed,true, false, false, true);
                Lang::Language::instance().check_change((Lang::lang_t)value,true);
            }
            else
            {
                Lang::Language::instance().check_change((Lang::lang_t)value,false);
            }
            break;
        case color:
            Prop::set(Prop::color, value, true, false, true);
            break;
        default:
            return false;
    }
    return true;
}

bool IOT::report_dp(Dp dp, DP_PROP_TP_E type, TY_OBJ_DP_VALUE_U value) {
    if (connected()) {
        TY_OBJ_DP_S dp_value;
        dp_value.dpid = dp;
        dp_value.type = type;
        dp_value.value = value;
        dp_value.time_stamp = 0;
        dev_report_dp_json_async(get_gw_cntl()->gw_if.id, &dp_value, 1);
        return true;
    }
    return false;
}

bool IOT::report_prop(Prop::prop_t prop) {
    if (connected()) {
        TY_OBJ_DP_S dp_value;
        auto it = tuya_dp_map.find(prop);
        if (it == tuya_dp_map.end()) {
            return false;
        }
        dp_value.dpid = it->second.dp;
        dp_value.type = it->second.type;
        if (it->second.type == PROP_BOOL)
            dp_value.value.dp_bool = Prop::get<bool>(prop);
        else
            dp_value.value.dp_value = Prop::get<int>(prop) * it->second.factor;
        dp_value.time_stamp = 0;

        dev_report_dp_json_async(get_gw_cntl()->gw_if.id, &dp_value, 1);

        return true;
    }
    return false;
}

bool IOT::report_all_prop() {
    if (connected()) {
        std::vector<TY_OBJ_DP_S> payload;
        for (auto itr: tuya_dp_map) {
            auto index = (Prop::prop_t) itr.first;
            if (!Prop::ready(index)) {
                continue;
            }
            if (index == Prop::pomodoro)
                continue;

            TY_OBJ_DP_S dp;
            dp.dpid = itr.second.dp;
            dp.type = itr.second.type;
            if (itr.second.type == PROP_BOOL)
                dp.value.dp_bool = Prop::get<bool>(itr.first);
            else
                dp.value.dp_value = Prop::get<int>(itr.first) * itr.second.factor;
            dp.time_stamp = 0;
            payload.push_back(dp);
        }

        OPERATE_RET ret = dev_report_dp_json_async_force(get_gw_cntl()->gw_if.id, payload.data(), payload.size());
        if (ret == OPRT_OK) {
            Sys::set_condition(Sys::DATA_REPORTED);
        }
        return true;
    }
    return false;
}

void IOT::report_ip() {
    if (connected()) {
        nlohmann::json ip_ver;
        static std::string ip_value = WiFi.localIP().toString().c_str();

        ip_ver["ip"] = WiFi.localIP().toString().c_str();
        ip_ver["media_ver"] = Prop::get<uint32_t>(Prop::last_evolve_time);
        ip_ver["hd_ver"] = Prop::get<uint32_t>(Prop::hardware_version);

        std::string ip_ver_value = ip_ver.dump();

        TY_OBJ_DP_VALUE_U value = {.dp_str =(char *) ip_ver_value.c_str()};
        log_d("debug-ip %s ", value.dp_str);
        report_dp(ip, PROP_STR, value);
    }
}

void IOT::report_touch_status(uint32_t status)
{
    TY_OBJ_DP_VALUE_U upload_value = {.dp_enum = status};
    report_dp(touching,PROP_ENUM,upload_value);
}

void IOT::report_pomodoro()
{
    if (Prop::get<int>(Prop::pomodoro) == 0 || pass_provision)
    {
        TY_OBJ_DP_VALUE_U upload_value = {.dp_value = Prop::get<int>(Prop::pomodoro)};
        report_dp(pomodoro,PROP_VALUE,upload_value);
        pass_provision = false;
    }

}

void IOT::restore() {
    /* Stop MQTT service */
//    if (m_client.is_activated && tuya_mqtt_connected(&m_client.mqctx)) {
//        tuya_mqtt_stop(&m_client.mqctx);
//    }
//
//    /* Clean client local data */
//    tuya_iot_activated_data_remove(&m_client);
}

void IOT::set_time_sync(uint32_t timestamp, uint32_t sys_time) {
    time_sync.timestamp = timestamp;
    time_sync.sys_time = sys_time;
}

uint32_t IOT::get_synced_timestamp() const {
    return (millis() / 1000 - time_sync.sys_time) + time_sync.timestamp;
}

void IOT::connect_cb() {
    log_i("report all prop\n");
    report_all_prop();
    /* other dp sync */
    report_ip();

    /* report pomodoro */
    report_pomodoro();

    /* ota session result is processed here for simplicity */
    int status = Prop::get<int>(Prop::ota_status);
    log_d("debug-ota :%d", status);
    if (status == OTAStatusType::ota_start) {
        /* ota somehow wasn't properly ended */
        // tuya_ota_upgrade_status_report(&m_ota_handle, TUS_UPGRD_EXEC);
    } else if (status == OTAStatusType::ota_success) {
        /* force evolve after ota */
        Prop::set(Prop::force_evolve, true, true, false);
        EvolveUpdateHelper::check_evolve_update();
    }
    /*fixme*/
//        Prop::set(Prop::force_evolve, true, true, false);
//        EvolveUpdateHelper::check_evolve_update();

    /* reset ota status */
    Prop::set(Prop::ota_status, OTAStatusType::no_ota, true, false, true, true);
}

bool IOT::connected() {
    return m_connected;
}

void IOT::set_connection_status(bool value) {
    m_connected = value;
}

void IOT::touch_switch_upload(Dp dp,bool value) {

    TY_OBJ_DP_VALUE_U value_u = {.dp_bool = value};
    report_dp(dp,PROP_BOOL,value_u);
}

void IOT::get_uid_authkey() {
    OPERATE_RET rt = OPRT_OK;
    if (m_uuid.empty() || m_authkey.empty()) {
        log_w("IOT credential not found!");
        /* register and get credential from server */
        for (int i = 0; i < 3; i++) {
            if (register_in_cloud()) {
                break;
            }
            vTaskDelay(1000);
        }
    }
}

void IOT::ensure_pass_provision(bool en)
{
    pass_provision = en;
}


void IOT::main_routine_wrapper(void *param) {
    static_cast<IOT *>(param)->main_routine();
    vTaskDelete(NULL);
}

void IOT::main_routine() {

    if (!get_credentials())
    {
        if (LvglDriver::instance().get_current_ui_index() == UI::UI_LOADING)
            LvglDriver::instance().get_current_ui<UI::UILoading>()->update_desc("Fail to get tuya credentials,cannot init tuya sdk");
        return;
    }
    OPERATE_RET rt = OPRT_OK;
    // 涂鸦Device OS SDK 初始化， 因为DB初始化耗时较长，影响一些设备的启动效率，因此初始化的时候进行特殊处理
    // 对DB进行延后初始化
    TY_INIT_PARAMS_S init_param = {0};
    init_param.init_db = FALSE;
    strcpy(init_param.sys_env, TARGET_PLATFORM);
    TUYA_CALL_ERR_LOG(tuya_iot_init_params(NULL, &init_param));

    // 设备功能初始化前置准备工作，处理一些依赖于基线的基础能力，又需要快速处理的业务
    pre_device_init();

    // DB延后初始化
    tuya_iot_kv_flash_init(NULL);


    TUYA_CALL_ERR_LOG(tuya_iot_kv_flash_init("/tuya"));

    ws_db_init_mf();

    WF_GW_PROD_INFO_S prod_info = {(char *) m_uuid.c_str(), (char *) m_authkey.c_str(),
                                   "SmartLife_Ivy", "icl123159"};
    TUYA_CALL_ERR_LOG(tuya_iot_set_wf_gw_prod_info(&prod_info));

    ty_subscribe_event(EVENT_INIT, "dev_init", tuya_dev_init_event_cb, FALSE);

    TUYA_CALL_ERR_LOG(device_init());
}

void IOT::process_sys_cmd(int cmd) {
    log_d("debug-tuya process sys cmd %d", cmd);
    switch (cmd) {
        case SYS_CMD_SYNC_BT: {
            break;
        }
        case SYS_CMD_CALIB_POT_IN:
        case SYS_CMD_CALIB_POT_OFF:
//            Belly::instance().set_external_calib_state(false);
            break;
        case SYS_CMD_FLUID_CALIB_RESET:
            Sense::instance().reset_water_level_calib();
            break;
        case SYS_CMD_FLUID_CALIB_EXE:
            Sense::instance().calibrate_water_level();
            break;
        case SYS_CMD_TOUCH_CALIB:
            Skin::instance().calibrate_all(100);
            break;
        case SYS_CMD_CHECK_EVOLVE: {
            EvolveUpdateHelper::check_evolve_update();
            break;
        }
        case SYS_CMD_UI_APPLY:
            UI::ui_set_action(true);
            break;
        case SYS_CMD_UI_CANCEL:
            UI::ui_set_action(false);
            break;
        case SYS_CMD_RE_TUTORIAL:
//            Prop::set(Prop::tutorial_complete, false, true, false, true, true);
//            Boot::soft_reboot();
            break;
        case SYS_CMD_RESET_SOIL_CALIB_DATA:
            Plant::reset_soil_sensor_calib_info();
            break;
        case SYS_CMD_FRONT_TOUCH:
            Skin::instance().get_tp_by_index(Skin::TP_INDEX_FRONT).dispatch_event(BUTTON_TAP);
            break;
        case SYS_CMD_LEFT_TOUCH:
            Skin::instance().get_tp_by_index(Skin::TP_INDEX_LEFT).dispatch_event(BUTTON_TAP);
            break;
        case SYS_CMD_RIGHT_TOUCH:
            Skin::instance().get_tp_by_index(Skin::TP_INDEX_RIGHT).dispatch_event(BUTTON_TAP);
            break;
        case SYS_CMD_UPLOAD_LOG:
            IvyNet::instance().upload_log();
            break;
        case SYS_CMD_ANIM_PLAY:
            CustomAnimPlayer::instance().play(CustomAnimPlayer::keep);
            break;
        case SYS_CMD_ANIM_STOP:
            CustomAnimPlayer::instance().stop();
            break;
        case SYS_CMD_ANIM_NEXT:
            CustomAnimPlayer::instance().next();
            break;
        case SYS_CMD_ANIM_SINGLE:
            CustomAnimPlayer::instance().set_mode(CustomAnimPlayer::single);
            break;
        case SYS_CMD_ANIM_ROTATE:
            CustomAnimPlayer::instance().set_mode(CustomAnimPlayer::rotate);
            break;
        case SYS_CMD_ANIM_RANDOM:
            CustomAnimPlayer::instance().set_mode(CustomAnimPlayer::random);
            break;
        case SYS_CMD_RESTORE_TO_FACTORY:
            /* todo add UI confirm */
            Sys::restore_to_factory();
            break;
        default:
            break;
    }
}

bool IOT::register_in_cloud() {
    std::string uri = "/ivy/factory-register";
    try {
        std::string ret_str = API::get(plantsio::get_host().c_str(), uri.c_str(), plantsio::get_headers(),
                                       plantsio::get_port());
        plantsio::ret_t ret = plantsio::get_ret(ret_str);
        if (ret == plantsio::SUCCESS) {
            nlohmann::json js = nlohmann::json::parse(ret_str);
            auto uuid = js["uuid"].get<std::string>();
            auto key = js["key"].get<std::string>();
            auto color = js["color"].get<int>();
            sd_store_credentials(key, uuid);
            Prop::set(Prop::color, color, true, false, true);
            return true;
        }
    } catch (...) {}
    return false;
}

bool IOT::get_credentials() {

    bool in_sd = false;
    bool in_nvs = false;

    nlohmann::json js = get_credentials_json();

    std::string nvs_uuid = nvs_get_id();
    std::string nvs_authkey = nvs_get_key();
    std::string sd_uuid = sd_get_id(js);
    std::string sd_authkey = sd_get_key(js);

    log_d("nvs uuid = %s authkey = %s",nvs_uuid.c_str(),nvs_authkey.c_str());
    log_d("sd uuid = %s authkey = %s",sd_uuid.c_str(),sd_authkey.c_str());

    in_nvs = has_credentials(nvs_uuid,nvs_authkey);

    in_sd = has_credentials(sd_uuid,sd_authkey);

    if (!in_nvs && !in_sd)
        return false;

    if (in_nvs && in_sd)
    {
        m_uuid = nvs_uuid;
        m_authkey = nvs_authkey;

        if (nvs_uuid != sd_uuid || nvs_authkey != sd_authkey)
        {
            sd_store_credentials(nvs_uuid,nvs_authkey);
        }
    }
    else
    {
        if (in_nvs)
        {
            m_uuid = nvs_uuid;
            m_authkey = nvs_authkey;
            sd_store_credentials(nvs_uuid,nvs_authkey);
        }
        if (in_sd)
        {
            m_uuid = sd_uuid;
            m_authkey = sd_authkey;
            nvs_store_credentials(sd_uuid,sd_authkey);
        }
    }

    return true;
}



