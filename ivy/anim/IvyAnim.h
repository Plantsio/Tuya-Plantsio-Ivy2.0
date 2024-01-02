//
// Created by Huwensong on 2023/12/26.
//

#ifndef FIRMWARE_IVYANIM_H
#define FIRMWARE_IVYANIM_H

#include "MjpegDriver.h"
#include "AudioDriver.h"

class IvyAnim
{
public:
    void init_file_sys();
    void bind_assets(const char* path);

    void play();

private:
    void find_decoder(std::shared_ptr<AnimDriver> decoder);

private:
    std::shared_ptr<AudioDriver> m_audio_decoder;
    std::shared_ptr<MjpegDriver> m_video_decoder;

    string anim_name;
};

#endif //FIRMWARE_IVYANIM_H
