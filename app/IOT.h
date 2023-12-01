//
// Created by Gordon on 2022/11/23.
//

#ifndef PLANTSIO_IVY_IOT_H
#define PLANTSIO_IVY_IOT_H

/*  NOTE: Must Implement
class IOT{
public:
    void restore();

    template<typename T>
    bool dp_handler(Dp dp, T value);

    template<typename T>
    bool report_dp(Dp dp, T value);

    bool report_prop(Prop::prop_t prop);

    bool report_all_prop();

    void begin();

private:
    bool register_in_cloud();
}
 * */


#include "memory"
#include "common/Prop.h"
#include "IOT.h"
#include "Njson.hpp"
#include "Arduino.h"

// SDK includes
#include "device_interface.h"

#define ASYNC_TIMER_TICK 500

#define PRODUCT_KEY            "9sq9uydcv0aggmh5"

#define CREDENTIALS_FILEPATH   "/factory/tuya_credentials.json"

#define CREDENTIAL_SPACE        "credentials"
#define CREDENTIAL_ID           "uuid"
#define CREDENTIAL_KEY          "key"

struct time_sync_t {
    uint32_t timestamp;
    uint32_t sys_time;
};

class IOT {
public:
    //region Singleton
    /* todo use it or not use it, this is a question */
    static IOT &instance();

    IOT(const IOT &) = default;

    IOT(IOT &&) = delete;

    IOT &operator=(const IOT &) = delete;

    IOT &operator=(IOT &&) = delete;
    //endregion

    IOT();

public:
    void begin();

    bool dp_handler(Dp dp, int value);

    bool report_dp(Dp dp, DP_PROP_TP_E type, TY_OBJ_DP_VALUE_U value);

    bool report_prop(Prop::prop_t prop);

    bool report_all_prop();

    void report_ip();

    void report_touch_status(uint32_t status);

    void report_pomodoro();

    void restore();

    void set_time_sync(uint32_t timestamp, uint32_t sys_time);

    uint32_t get_synced_timestamp() const;

    void connect_cb();

    bool connected();

    void set_connection_status(bool value);

    void touch_switch_upload(Dp dp,bool value);

    void get_uid_authkey();

    void ensure_pass_provision(bool en);

private:
    static void main_routine_wrapper(void *param);

    void main_routine();

    void process_sys_cmd(int cmd);

    void init_dp_data();

    bool register_in_cloud();

    bool get_credentials();

private:
    std::string m_uuid;

    std::string m_authkey;

    bool m_connected = false;

    bool timer_is_changed = false;

    bool pass_provision = false;

    time_sync_t time_sync{};

    std::unordered_map<Prop::prop_t,dp_type_t> tuya_dp_map;
};

#endif //PLANTSIO_IVY_IOT_H
