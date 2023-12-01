//
// Created by sungaoran on 2022/1/18.
//

#ifndef UI_SANDBOX_UIWEATHER_H
#define UI_SANDBOX_UIWEATHER_H

#include "Anim/ui/Base.h"
#include "tool/system.h"
#include "Anim/lang/slot/SlotLabel.h"

namespace UI {
    class UIWeather : public UI::Base {
    public:
        UIWeather();

        void routine() override;

        void start_routine() override;

        void update();

    private:
        lv_obj_t *lay_left;
        lv_obj_t *lay_right;
        lv_obj_t *weather_anim;
        Label weather_label;
        Label city_label;
        lv_obj_t *current_temp_label;
        lv_obj_t *temp_range_label;
        lv_obj_t *temp_unit_label;
        lv_obj_t *minus_line;
        lv_point_t minus_line_points[2] = {{0,  0},
                                           {20, 0}};
    };
}

#endif //UI_SANDBOX_UIWEATHER_H
