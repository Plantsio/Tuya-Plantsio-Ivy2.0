//
// Created by Huwensong on 2023/12/22.
//

#include "AnimDriver.h"

AnimDriver::AnimDriver()
{
}

AnimDriver::~AnimDriver()
{

}

void AnimDriver::prepare_resource()
{

}

void AnimDriver::release_resource()
{

}

void AnimDriver::stop()
{

}

bool AnimDriver::anim_bind_assets(const char *assets_name)
{
    m_anim_file = SD_MMC.open(assets_name);
    if (!m_anim_file.available())
    {
        return false;
    }
    return true;
}

void AnimDriver::begin()
{
    int ret = xTaskCreatePinnedToCore(play_routine_wrapper,std::to_string((int)this).c_str(),1024 * 6, this,6,
                                      nullptr,1);
    if (ret != pdPASS) {
        log_e("Decoder task ret %d", ret);
    }
}

void AnimDriver::play_routine_wrapper(void *param)
{
    static_cast<AnimDriver *>(param)->play_routine();
}

void AnimDriver::play_routine()
{
    vTaskDelete(nullptr);
}




