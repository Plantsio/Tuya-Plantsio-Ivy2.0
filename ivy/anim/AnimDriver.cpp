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

void  AnimDriver::begin()
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





