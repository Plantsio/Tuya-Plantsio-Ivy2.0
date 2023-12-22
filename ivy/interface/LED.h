//
// Created by Gordon on 2021/8/12.
//

#ifndef PLANTSIO_IVY_LED_H
#define PLANTSIO_IVY_LED_H

#include "driver/ledc.h"

class LED {
public:

    explicit LED(int t_pin);

    /**
     * fade led in percentage
     * @param percent
     * @param block
     * @param fade_t
     */
    void fade_to_percent(double t_percent, bool t_block, uint16_t t_fade_t);

    /**
     * fade led in duty cycle
     * @param t_target
     * @param t_block
     * @param t_fade_t
     */
    void fade_to_dc(uint32_t t_target, bool t_block, uint16_t t_fade_t);

    /**
     * fade to end point
     * @param t_on
     * @param t_block
     * @param t_fade_t
     */
    void fade_to_end(bool t_on, bool t_block, uint16_t t_fade_t);

    uint32_t get_current_percent() {
        return m_current_percent;
    }

private:
    uint32_t m_current_percent = 0;
    uint32_t m_max_dc;
    ledc_timer_config_t m_timer;
    ledc_channel_config_t m_channel;
};

#endif //PLANTSIO_IVY_LED_H
