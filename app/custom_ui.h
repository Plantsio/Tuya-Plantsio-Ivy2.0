//
// Created by Huwensong on 2022/12/5.
//

#ifndef MI_PLANTS_IVY_CUSTOM_UI_H
#define MI_PLANTS_IVY_CUSTOM_UI_H

#include "Anim/ui/Base.h"

namespace UI
{
#define CUSTOM_UI(index)  ((index_t)(index))
    enum ui_extra_index {
        UI_EXTRA_START = UI_INTERNAL_END + 1,
        UI_RECONNECT,
        UI_PROV_TIP,
        UI_WEATHER,
        UI_TIME,
        UI_DATE,
    };

    void custom_ui_register();

    void custom_ui_load_tutorial();

    std::vector<index_t> custom_get_service_ui();
}

#endif //MI_PLANTS_IVY_CUSTOM_UI_H
