//
// Created by Huwensong on 2022/12/5.
//

#include "custom_ui.h"
#include "Anim/ui/UI.h"
#include "Anim/drivers/LvglDriver.h"

//custom ui include
#include "UIReconnect.h"
#include "UIProvTip.h"

#include "NetInterface.h"

namespace UI {
    void custom_ui_register() {
        register_ui<UIReconnect>(CUSTOM_UI(UI_RECONNECT));
        register_ui<UIProvTip>(CUSTOM_UI(UI_PROV_TIP));
        register_ui<UIWeather>(CUSTOM_UI(UI_WEATHER)),
        register_ui<UITime>(CUSTOM_UI(UI_TIME));
        register_ui<UIDate>(CUSTOM_UI(UI_DATE));
    }

    void custom_ui_load_tutorial() {
//        LvglDriver::instance().load(UI::UI_PROV_TIP);
        NetInterface::instance().net_prov_start();
    }

    std::vector<index_t> custom_get_service_ui()
    {
        return {CUSTOM_UI(UI_TIME), CUSTOM_UI(UI_WEATHER),CUSTOM_UI(UI_DATE)};
    }
}
