//
// Created by Huwensong on 2023/2/6.
//

#include "UIProvTip.h"
#include "Anim/ui/tools.h"
#include "Anim/lang/Lang.h"
#include "Anim/drivers/LvglDriver.h"
#include "common/Boot.h"
#include "Anim/drivers/EmoDriver.h"
#include "NetInterface.h"


#define PWD_ERROR_DELAY               (60 * 1000)
#define AP_NOT_FOUND_DELAY            (30 * 1000)


namespace UI {

    UIProvTip::UIProvTip()
            : TuCanvasBase(),
              m_sub_text(m_scr, 280, 800),
              m_title_text(m_scr, 280, 800) {
        m_sub_text.set_text_font({.zh = Lang::mi_light_font_16,
                                  .en = Lang::ba_font_16,
                                  .jp = Lang::jp_font_16});
        m_title_text.set_text_font({.zh = Lang::mi_light_font_30,
                                    .en = Lang::ba_font_30,
                                    .jp = Lang::jp_font_30});

        lv_obj_set_width(m_bottom_text.label, 160);

        lv_label_set_long_mode(m_bottom_text.label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    }

    bool UIProvTip::reach_status(int status, const std::string &processing_text) {
        auto prov_status = NetInterface::instance().get_prov_status();
        if (prov_status >= status && prov_status < NetInterface::prov_err_start) {
            return true;
        }
        if (m_should_show_processing && !processing_text.empty() && millis() > m_next_status_t) {
            m_sub_text.update(processing_text.c_str());
            m_should_show_processing = false;
        }
        return false;
    }

    void UIProvTip::set_next_status_timeout(uint32_t timeout) {
        m_next_status_t = millis() + timeout;
        m_should_show_processing = true;
    }

    void UIProvTip::check_error() {
        NetInterface::prov_status_t status = NetInterface::instance().get_prov_status();
        if (status > NetInterface::prov_err_start) {
            if (m_current_err != status) {
                log_w("error %d occurred during provisioning", status);
                switch (status) {
                    case NetInterface::prov_wifi_conn_fail:
                        m_sub_text.update(get_colored_str(LANG(Lang::ui_tuya_tip_fail_reset), palette_failure).c_str());
                        break;
                    case NetInterface::prov_pwd_error:
                        m_sub_text.update(
                                get_colored_str(LANG(Lang::ui_tuya_tip_reason_pwd_error), palette_failure).c_str());
                        m_bottom_text.update(LANG(Lang::ui_prov_fail));
                        set_restart(true, PWD_ERROR_DELAY);
                        break;
                    case NetInterface::prov_ap_not_found:
                        m_sub_text.update(
                                get_colored_str(LANG(Lang::ui_tuya_tip_reason_ap_not_found), palette_failure).c_str());
                        set_restart(true, AP_NOT_FOUND_DELAY);
                        m_bottom_text.update(LANG(Lang::ui_prov_fail));
                        break;
                    case NetInterface::prov_rssi_low:
                        m_sub_text.update(
                                get_colored_str(LANG(Lang::ui_tuya_tip_reason_week), palette_failure).c_str());
                        break;
                    default:
                        break;
                }
                m_current_err = status;
            }
        } else {
            m_current_err = 0;
        }
    }

    void UIProvTip::next() {
        if (m_should_restart) {
            if (millis() > m_restart_t) {
                log_w("Error timeout, restart now");
                Boot::soft_reboot();
            }
            return;
        }

        check_error();

        if (m_current_err) {
            /* error occurred, skipping normal process */
            return;
        }

        switch (m_current_step) {
            case 0: {
                lv_timer_set_period(m_timer, 100);  /* check status every 100ms */
                m_top_text.update(LANG_COLOR(palette_notice,LANG(Lang::ui_tu_open_tuya_app)).c_str());
                m_sub_text.update(LANG_COLOR(palette_notice,LANG(Lang::ui_tu_tuya_add_device)).c_str(), true, false, 500);
                lv_obj_align_to(m_sub_text.label, m_top_text.label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
                lv_obj_align_to(m_title_text.label, m_top_text.label, LV_ALIGN_TOP_MID, 0, 20);

                std::string asst_name = "tu/tu_tuya_1";
                bool tu_exists = SD_MMC.exists(get_asset_path(asst_name,cus_tu).c_str());

                if (tu_exists)
                    anim_canvas_bind_asset(m_canvas, "tu/tu_tuya_1");
                else
                    anim_canvas_bind_asset(m_canvas, "provisioning",cus_custom);
                anim_canvas_update(m_canvas);
                lv_obj_align(m_canvas, LV_ALIGN_CENTER, 0, 15);
                auto a = anim_create(m_canvas, anim_fade, LV_OPA_TRANSP, LV_OPA_COVER, 800, 1500);
                lv_anim_start(&a);
                m_bottom_text.update(LANG(Lang::ui_prov_popup), true, false, 2000);
                break;
            }
            case 1: {
                if (!reach_status(NetInterface::prov_ble_connected, "")) {
                    return;
                }
                m_bottom_text.update("");
                m_top_text.update("");
                m_title_text.update(LANG(Lang::ui_prov_configuring), true, false, 400);
                auto a = anim_create(m_canvas, anim_fade, LV_OPA_COVER, LV_OPA_TRANSP, 800, 0);
                lv_anim_start(&a);
                m_sub_text.update(LANG(Lang::ui_prov_app_success));
                auto a2 = anim_create(m_sub_text.label, anim_move_ver, lv_obj_get_y(m_sub_text.label), 110, 800, 1000);
                lv_anim_start(&a2);
                set_next_status_timeout(1500);
                break;
            }
            case 2: {
                if (!reach_status(NetInterface::prov_wifi_connected, LANG(Lang::ui_prov_connecting_router))) {
                    return;
                }
                m_sub_text.update(LANG(Lang::ui_prov_router_success));
                set_next_status_timeout(1500);
                break;
            }
            case 3: {
                if (!reach_status(NetInterface::prov_activated, LANG(Lang::ui_prov_activating))) {
                    return;
                }
                m_sub_text.update(LANG(Lang::ui_prov_act_success));
                lv_timer_set_period(m_timer, 5000);
                break;
            }
            case 4: {
                    if (Prop::get<bool>(Prop::tutorial_complete))
                    {
                        LvglDriver::instance().set_locked(false);
                        Boot::booted_cb();
                        Sys::set_condition(Sys::BOOT_OVER);
                        if (Prop::get<bool>(Prop::has_pot)) {
                            IvyAnim::instance().set_current_drv(IvyAnim::emo);
                            EmoDriver::instance().set_current_anim(
                                    Anim::create({AnimUnit("yawn", 1)}, nullptr, true));
                        } else {
                            LvglDriver::instance().route_to_default();
                        }
                    }
                return;
            }
        }
        m_current_step++;
    }
}

