//
// Created by Huwensong on 2023/2/6.
//

#ifndef TUYA_PLANTS_IVY_UIPROVTIP_H
#define TUYA_PLANTS_IVY_UIPROVTIP_H

#include "Anim/ui/Base.h"
#include "Anim/ui/Components.h"
#include "Anim/ui/TutorialUI.h"

namespace UI {
    class UIProvTip : public TuCanvasBase {
    public:
        UIProvTip();

    public:
        void next() override;

    private:
        /**
         * process errors, should be called first
         */
        void check_error();

        void set_restart(bool should_restart, uint32_t timeout = 60 * 1000) {
            m_restart_t = millis() + timeout;
            m_should_restart = should_restart;
        }

        bool reach_status(int status, const std::string &processing_text);

        void set_next_status_timeout(uint32_t timeout);

    private:
        BasicText m_sub_text;
        BasicText m_title_text;

        uint32_t m_next_status_t = 0; /* when should the next status set */
        bool m_should_show_processing = false;

        uint32_t m_restart_t = 0; /* when should the next status set */

        int m_current_err = 0;
        bool m_should_restart = false;
    };
}

#endif //TUYA_PLANTS_IVY_UIPROVTIP_H
