//
// Created by sungaoran on 2022/1/18.
//

#include "UIWeather.h"
#include "Anim/ui/tools.h"
#include "lv_conf.h"
#include "behaviortree_cpp_v3/utils/make_unique.hpp"
#include "Network/IvyNet.h"
#include "common/Sys.h"

#include "Anim/lang/Lang.h"
#include "Anim/lang/slot/SlotManager.h"

namespace UI {
    UIWeather::UIWeather() : Base(),lay_left(lv_obj_create(m_scr)),
    lay_right(lv_obj_create(m_scr)),
    weather_label(lay_left),
    city_label(lay_left){
        lv_obj_remove_style_all(lay_left);
        lv_obj_remove_style_all(lay_right);
        lv_obj_set_size(lay_left, 140, 240);
        lv_obj_set_pos(lay_left, 0, 0);
        lv_obj_set_size(lay_right, 140, 240);
        lv_obj_set_pos(lay_right, 140, 0);

        if (Prop::get<int>(Prop::language) == Lang::zh)
        {
            city_label.subscribe_slot(slot_city);
            weather_label.subscribe_slot(slot_weather);

            weather_label.label_set_text_style(lv_color_white(),LV_TEXT_ALIGN_RIGHT);
        }
        else
        {
            city_label.label_set_font({.zh = Lang::mi_light_font_36,
                                              .en = Lang::ba_font_30,
                                              .jp = Lang::jp_font_36});

            weather_label.label_set_font({.zh = Lang::mi_light_font_24,
                                                 .en = Lang::ba_font_16,
                                                 .jp = Lang::jp_font_24},
                                         lv_color_white(), LV_TEXT_ALIGN_RIGHT);
        }

        city_label.label_set_pos(18,30);
        city_label.label_set_width(100);


        weather_anim = lv_canvas_create(lay_left);
        anim_canvas_reset_asset();
        lv_obj_align(weather_anim, LV_ALIGN_CENTER, 0, -5);

        current_temp_label = lv_label_create(lay_right);
        Lang::Language::instance().lang_set_style_text_font(current_temp_label,
                                                            {.zh = Lang::mi_light_font_109,
                                                             .en = Lang::ba_font_120,
                                                             .jp = Lang::jp_font_120});
        minus_line = lv_line_create(m_scr);
        lv_obj_set_style_line_color(minus_line, lv_color_white(), 0);
        lv_line_set_points(minus_line, minus_line_points, 2);
        lv_obj_set_style_line_width(minus_line, 10, 0);

        weather_label.label_align(LV_ALIGN_CENTER,0, 60);

        temp_range_label = lv_label_create(m_scr);
        Lang::Language::instance().lang_set_style_text_font(temp_range_label,
                                                            {.zh = Lang::mi_light_font_24,
                                                             .en = Lang::ba_font_30,
                                                             .jp = Lang::jp_font_24});

        temp_unit_label = lv_label_create(m_scr);
        Lang::Language::instance().lang_set_style_text_font(temp_unit_label,
                                                            {.zh = Lang::mi_light_font_24,
                                                             .en = Lang::ba_font_30,
                                                             .jp = Lang::jp_font_24});
    }

    static void set_width(void *var, int32_t v) {
        lv_obj_set_width((lv_obj_t *) var, v);
    }

    static void set_height(void *var, int32_t v) {
        lv_obj_set_height((lv_obj_t *) var, v);
    }

    static std::string get_temp_abs() {
        return get_degree_string(Sys::get_geo_info().weather.temp);
    }

    static std::string get_temp_max() {
        return get_degree_string(Sys::get_geo_info().weather.max);
    }

    static std::string get_temp_min() {
        return get_degree_string(Sys::get_geo_info().weather.min);
    }

    void UIWeather::update() {
        const weather_t &weather = Sys::get_geo_info().weather;
        anim_canvas_bind_asset(weather_anim, get_weather_asset_name(weather));

        lv_label_set_text(current_temp_label, get_temp_abs().c_str());
        lv_obj_set_style_text_align(current_temp_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(current_temp_label, LV_ALIGN_CENTER, 10, -10);

        if (Prop::get<int>(Prop::language) != Lang::zh)
        {
            city_label.label_set_text(Sys::get_geo_info().location.city_district.c_str());
            weather_label.label_set_text(Sys::get_weather_by_lang((Lang::lang_t)Prop::get<int>(Prop::language)).c_str());
        }


        std::string range = get_temp_min() + "/" + get_temp_max() + "°";
        lv_label_set_text(temp_range_label, range.c_str());
        lv_obj_align_to(temp_range_label, current_temp_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

        lv_label_set_text(temp_unit_label, get_temp_unit().c_str());
        lv_obj_align_to(temp_unit_label, current_temp_label, LV_ALIGN_OUT_RIGHT_TOP, -15, -25);


        if (weather.temp < 0 && Prop::get<bool>(Prop::use_fahrenheit)) {
            lv_obj_align_to(minus_line, current_temp_label, LV_ALIGN_OUT_LEFT_MID, 10, 0);
            lv_obj_clear_flag(minus_line, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(minus_line, LV_OBJ_FLAG_HIDDEN);
        }

    }

    void UIWeather::routine() {
        anim_canvas_update(weather_anim);
        weather_label.routine();
        city_label.routine();

    }

    void UIWeather::start_routine() {
        Base::start_routine();
        update();
    }
}