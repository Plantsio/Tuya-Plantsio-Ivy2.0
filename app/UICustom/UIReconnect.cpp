//
// Created by Huwensong on 2023/1/9.
//

#include "UIReconnect.h"

#include "Anim/ui/tools.h"
#include "Anim/lang/Lang.h"
#include "Anim/drivers/LvglDriver.h"

#include "NetInterface.h"

#define TIMER_DELAY      500

namespace UI
{
    UIReconnect::UIReconnect() : Base(),
    m_tip_title_text(m_scr),
    m_tip_content_text(m_scr)
    {
        m_tip_title_text.set_text_font({.zh = Lang::mi_light_font_36,
                                        .en = Lang::ba_font_40,
                                        .jp = Lang::jp_font_36});
        m_tip_content_text.set_text_font({.zh = Lang::mi_light_font_16,
                                          .en = Lang::ba_font_16,
                                          .jp = Lang::jp_font_16});
    }

    void UIReconnect::clear()
    {
        if (m_timer) {
            lv_timer_del(m_timer);
        }
        Base::clear();
    }

    void UIReconnect::routine()
    {
        Base::routine();
        update_title(get_colored_str("Reconnecting WiFi",palette_notice).c_str());
    }

    void UIReconnect::start_routine()
    {
        Base::start_routine();
        m_timer = lv_timer_create(timer_cb, TIMER_DELAY, this);
        lv_timer_ready(m_timer);

    }

    void UIReconnect::input_cb(UI::input_t input)
    {

    }

    void UIReconnect::update_title(const char *content)
    {
        m_tip_title_text.update(content, false);
        lv_obj_align(m_tip_title_text.label, LV_ALIGN_TOP_MID, 0, 20);
    }

    void UIReconnect::update_tip(const char *content)
    {
        m_tip_content_text.update(content);
        lv_obj_set_style_text_line_space(m_tip_content_text.label,10,0);
        lv_obj_set_style_text_align(m_tip_content_text.label,LV_TEXT_ALIGN_CENTER,0);
        lv_obj_align(m_tip_content_text.label, LV_ALIGN_CENTER, 0, 20);
    }

    void UIReconnect::next()
    {
        update_tip("Reconnecting WiFi,Please wait for a moment...");

        if(NetInterface::instance().net_connected())
        {
            update_tip("Success to connect WiFi,Wake up Ivy again!");
            LvglDriver::instance().load(UI_BOOT);
        }
    }

    void UIReconnect::timer_cb(lv_timer_t *timer)
    {
        auto ui = static_cast<UIReconnect *>(timer->user_data);
        ui->next();
    }
}