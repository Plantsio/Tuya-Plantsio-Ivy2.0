//
// Created by sungaoran on 2022/1/20.
//

#include "UIDate.h"
#include "vector"
#include "Anim/ui/tools.h"
#include "tool/system.h"

#include "Anim/lang/Lang.h"

namespace UI
{
    std::vector<lv_calendar_date_t> hl_dates;

    static void label_hide_cb_wrap(struct _lv_anim_t *anim)
    {
        auto *ui = static_cast<UIDate *>(anim->user_data);
        ui->label_hide_cb();
    }

    static void calendar_show_cb_wrap(struct _lv_anim_t *anim)
    {
        auto *ui = static_cast<UIDate *>(anim->user_data);
        ui->calendar_show_cb();
    }

    static void anim_line_width(void *var, int32_t value)
    {
        lv_obj_set_width((lv_obj_t *) var, value);
        lv_obj_set_x((lv_obj_t *) var, 160 - value / 2);
    }

    int btn_index = 0;

    static void anim_calender(struct _lv_timer_t *t)
    {
        for (int i = 0;i < 7;i++)
        {
            lv_btnmatrix_clear_btn_ctrl((lv_obj_t *) t->user_data, btn_index++, LV_BTNMATRIX_CTRL_HIDDEN);
        }
    }


    static void line_end_cb(struct _lv_anim_t *anim)
    {
        auto *ui = static_cast<UIDate *>(anim->user_data);
        lv_obj_set_style_line_width((lv_obj_t *) anim->var, 1, 0);
        ui->calendar_show_cb();
    }

    UIDate::UIDate() : Base()
    {
        time_t curtime;
        time(&curtime);
        tm *info = localtime(&curtime);

        mask = lv_obj_create(m_scr);
        lv_obj_remove_style_all(mask);
        lv_obj_set_size(mask, 320, 40);
        lv_obj_center(mask);

        calendar = lv_calendar_create(m_scr);
        lv_obj_set_size(calendar, 220, 200);
        lv_obj_align(calendar, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(calendar, lv_color_black(), 0);
        Lang::Language::instance().lang_set_style_text_font(lv_calendar_get_btnmatrix(calendar),
                                                            {.zh = Lang::mi_light_font_16,
                                                                              .en = Lang::ba_font_16,
                                                                              .jp = Lang::jp_font_16});
        lv_obj_set_style_border_color(calendar, lv_color_black(), 0);
        lv_obj_set_style_border_color(lv_calendar_get_btnmatrix(calendar), lv_color_black(), LV_PART_ITEMS);
        lv_obj_set_style_radius(lv_calendar_get_btnmatrix(calendar), 5, LV_PART_ITEMS);
//    lv_obj_set_style_bg_color(lv_calendar_get_btnmatrix(calendar),lv_color_black(),LV_PART_ITEMS);
        lv_calendar_set_today_date(calendar, info->tm_year + 1900, info->tm_mon + 1, info->tm_mday);
        lv_calendar_set_showed_date(calendar, info->tm_year + 1900, info->tm_mon + 1);

        hl_dates.push_back({.year=2022, .month=2, .day=14});
        hl_dates.push_back({.year=2022, .month=2, .day=25});
        lv_calendar_set_highlighted_dates(calendar, hl_dates.data(), hl_dates.size());

        lv_obj_add_flag(calendar, LV_OBJ_FLAG_HIDDEN);


        line = lv_line_create(m_scr);
        lv_obj_set_style_line_color(line, lv_color_make(245, 203, 38), 0);
        lv_line_set_points(line, line_points, 2);
        lv_obj_set_style_line_width(line, 2, 0);
        lv_obj_align_to(line, mask, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);

        init_label = lv_label_create(mask);
        Lang::Language::instance().lang_set_style_text_font(init_label,
                                                            {.zh = Lang::mi_light_font_36,
                                                             .en = Lang::ba_font_40,
                                                             .jp = Lang::jp_font_36});
        lv_obj_align(init_label, LV_ALIGN_CENTER, 0, 40);
        lv_obj_set_style_opa(init_label, LV_OPA_TRANSP, 0);

        lv_label_set_text(init_label, get_date_str(info).c_str());
    }

    void UIDate::start_routine()
    {
        timer = lv_timer_create(anim_calender, 20, lv_calendar_get_btnmatrix(calendar));
        lv_timer_pause(timer);
        lv_anim_t a1 = anim_create(init_label, anim_fade, LV_OPA_TRANSP, LV_OPA_COVER, 1000, 100, 1000, 1500, nullptr,
                                   this);
        lv_anim_t a2 = anim_create(init_label, anim_move_ver, lv_obj_get_y(init_label) + 40, lv_obj_get_y(init_label),
                                   1000,
                                   100, 0, 0, label_hide_cb_wrap, this);
        lv_anim_t a3 = anim_create(line, anim_fade, LV_OPA_TRANSP, LV_OPA_COVER, 1000, 100);
        lv_anim_start(&a1);
        lv_anim_start(&a2);
        lv_anim_start(&a3);
        Base::start_routine();
    }

    void UIDate::label_hide_cb()
    {
        auto a1 = anim_create(init_label, anim_move_ver, lv_obj_get_y(init_label), lv_obj_get_y(init_label) - 40, 500,
                              1500, 0, 0, nullptr, this);
        auto a2 = anim_create(line, anim_move_ver, lv_obj_get_y(line), 50, 1000,
                              1500);
        auto a3 = anim_create(line, anim_line_width, 260, 200, 1000, 1500, 0, 0, line_end_cb, this);
        lv_anim_start(&a1);
        lv_anim_start(&a2);
        lv_anim_start(&a3);
    }

    void UIDate::calendar_show_cb()
    {
        lv_obj_add_flag(init_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mask, LV_OBJ_FLAG_HIDDEN);
        lv_btnmatrix_set_btn_ctrl_all(lv_calendar_get_btnmatrix(calendar), LV_BTNMATRIX_CTRL_HIDDEN);
        lv_obj_clear_flag(calendar, LV_OBJ_FLAG_HIDDEN);
        btn_index = 0;
        lv_timer_resume(timer);
//    auto a = anim_create(lv_calendar_get_btnmatrix(calendar), anim_calender, 0, 48, 500);
        auto a = anim_create(lv_calendar_get_btnmatrix(calendar), anim_fade, LV_OPA_TRANSP, LV_OPA_COVER, 400);
        lv_anim_start(&a);
//    auto a = anim_create(calendar, anim_fade, LV_OPA_TRANSP, LV_OPA_COVER, 500);
//    lv_anim_start(&a);
    }

    void UIDate::clear()
    {
        if (timer)
        {
            lv_timer_del(timer);
        }
        Base::clear();
    }

    std::string UIDate::get_date_str(tm *time_info)
    {
        std::string date_str;
        switch (Lang::Language::instance().get_language())
        {
            case Lang::jp:
            case Lang::zh:
                date_str = std::to_string(time_info->tm_mon + 1) + "月" + std::to_string(time_info->tm_mday) + "日  " +
                           std::to_string(time_info->tm_year + 1900);
                break;
            case Lang::en:
                date_str = get_mon_str(time_info->tm_mon) + std::string(" ") + get_mday_str(time_info->tm_mday) +
                           std::string(", ") +
                           std::to_string(time_info->tm_year + 1900);
                break;
            default:
                break;
        }
        return date_str;
    }
}