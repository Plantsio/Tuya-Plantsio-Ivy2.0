//
// Created by Huwensong on 2023/1/9.
//

#ifndef TUYA_PLANTS_IVY_UIRECONNECT_H
#define TUYA_PLANTS_IVY_UIRECONNECT_H

#include "Anim/ui/Base.h"
#include "Anim/ui/Components.h"

namespace UI
{
    class UIReconnect : public Base
    {
    public:
        UIReconnect();

    public:

        void clear() override;

        void routine() override;

        void start_routine() override;

        void input_cb(UI::input_t input) override;


    private:
        void update_title(const char *content);

        void update_tip(const char *content);

        void next();

        static void timer_cb(lv_timer_t *timer);

    private:

        BasicText m_tip_title_text;
        BasicText m_tip_content_text;

        lv_timer_t *m_timer;
    };
}

#endif //TUYA_PLANTS_IVY_UIRECONNECT_H
