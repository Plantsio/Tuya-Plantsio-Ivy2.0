//
// Created by sungaoran on 2022/1/19.
//

#ifndef LVGL_UITIME_H
#define LVGL_UITIME_H

#include <ctime>
#include "Anim/ui/Base.h"
#include "tool/system.h"

namespace UI {
    class UITime : public Base {
    public:
        UITime();
        void start_routine() override;

        void routine() override;

        void update_info(bool force = false);

    private:
        static std::string get_date_str(tm * tm_info);

    private:
        lv_obj_t *weather_anim;
        lv_obj_t *time_label;
        lv_obj_t *day_label;
        lv_obj_t *date_label;
        lv_obj_t *sep_line;
        lv_obj_t *am_label;
        lv_point_t sep_line_points[2] = {{0, 0},
                                         {0, 80}};
        std::string weather_name;
        tm last_tm{0};
    };
}

#endif //LVGL_UITIME_H
