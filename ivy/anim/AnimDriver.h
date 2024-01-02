//
// Created by Huwensong on 2023/12/22.
//

#ifndef FIRMWARE_ANIMDRIVER_H
#define FIRMWARE_ANIMDRIVER_H

#include "Arduino.h"
#include "SD_MMC.h"
#include "FS.h"

class AnimDriver
{
public:
    AnimDriver();
    virtual ~AnimDriver();

public:
    virtual void prepare_resource();

    virtual void release_resource();

    virtual void begin();

    virtual void stop();

    virtual bool anim_bind_assets(const char *assets_name);

protected:
    File m_anim_file;
};

#endif //FIRMWARE_ANIMDRIVER_H
