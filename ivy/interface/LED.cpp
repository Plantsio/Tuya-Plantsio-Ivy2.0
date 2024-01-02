//
// Created by Gordon on 2021/8/12.
//

#include "LED.h"
#include "math.h"
#include "Arduino.h"


LED::LED(int t_pin) :
        m_channel({t_pin, LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0,
                   LEDC_INTR_DISABLE, LEDC_TIMER_0, 0, 0, {0}}) {
    static bool initialized = false;
    if (!initialized) {
        m_timer.speed_mode = LEDC_LOW_SPEED_MODE;
        m_timer.duty_resolution = LEDC_TIMER_11_BIT;
        m_timer.timer_num = LEDC_TIMER_0;
        m_timer.freq_hz = 20000;
        m_timer.clk_cfg = LEDC_AUTO_CLK;
        m_max_dc = pow(2, (int) m_timer.duty_resolution) - 1;
//        m_max_dc = (1 << m_timer.duty_resolution) - 1;;
        ledc_timer_config(&m_timer);
        initialized = true;
        ledc_fade_func_install(0);
        log_i("LEDC initialized");
    }

    ledc_channel_config(&m_channel);
}

void LED::fade_to_percent(double t_percent, bool t_block, uint16_t t_fade_t) {
    int dc = (int) (t_percent / 100 * m_max_dc);
    fade_to_dc(dc, t_block, t_fade_t);
}

void LED::fade_to_dc(uint32_t t_target, bool t_block, uint16_t t_fade_t) {
    log_v("dc:%d, max:%d", t_target, m_max_dc);
    uint32_t percent = t_target * 100 / m_max_dc;
    if (m_current_percent == percent) {
        return;
    }
    if (t_block) {
        ledc_set_fade_time_and_start(m_channel.speed_mode,m_channel.channel,t_target,t_fade_t,LEDC_FADE_WAIT_DONE);
    } else {
        ledc_set_fade_time_and_start(m_channel.speed_mode,m_channel.channel,t_target,t_fade_t, LEDC_FADE_NO_WAIT);
    }
    m_current_percent = percent;
}

void LED::fade_to_end(bool t_on, bool t_block, uint16_t t_fade_t) {
    if (t_on) {
        fade_to_dc(m_max_dc, t_block, t_fade_t);
    } else {
        fade_to_dc(0, t_block, t_fade_t);
    }
}
