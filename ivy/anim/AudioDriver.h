//
// Created by Huwensong on 2023/12/12.
//

#ifndef FIRMWARE_AUDIODRIVER_H
#define FIRMWARE_AUDIODRIVER_H

#include <string>

#include "Audio.h"
#include "AnimDriver.h"

class AudioDriver : public AnimDriver
{
public:
    AudioDriver();
    ~AudioDriver();

public:
    void prepare_resource() override;

    void release_resource() override;

    void begin()override;

    void stop() override;

    bool anim_bind_assets(const char *assets_name) override;

private:
    static void play_routine_wrapper(void *param);

    void play_routine();


private:
    Audio *aac_decoder = nullptr;

};

#endif //FIRMWARE_AUDIODRIVER_H
