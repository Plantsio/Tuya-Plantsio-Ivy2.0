//
// Created by Huwensong on 2023/12/26.
//

#include "IvyAnim.h"

#include <string>

#define SD_CLK              45
#define SD_CMD              47
#define SD_D0               48

const char* mount_name = "/sd";
#define ASSETS_PATH "/video"

void IvyAnim::init_file_sys()
{
    if(SD_MMC.setPins(SD_CLK,SD_CMD,SD_D0))
        log_d("set pin true");
    if(SD_MMC.begin(mount_name, true))
        log_d("mount true");
}

void IvyAnim::bind_assets(const char* name)
{
    std::string assets_path = std::string(ASSETS_PATH) + "/" + name + "/";
    std::string audio_name = assets_path + name + ".aac";
    std::string video_name = assets_path + name + ".mjpeg";

    log_d("audio name = %s,video name = %s",audio_name.c_str(),video_name.c_str());

    if (SD_MMC.exists(audio_name.c_str()))
    {
        m_audio_decoder = std::make_shared<AudioDriver>();
        m_audio_decoder->prepare_resource();
        m_audio_decoder->anim_bind_assets(audio_name.c_str());
    }

    if(SD_MMC.exists(video_name.c_str()))
    {
        m_video_decoder = std::make_shared<MjpegDriver>();
        m_video_decoder->prepare_resource();
        m_video_decoder->anim_bind_assets(video_name.c_str());
    }

}

void IvyAnim::play()
{
    if (m_audio_decoder)
    {
        find_decoder(m_audio_decoder);
        log_d("audio decoder play");
    }

    if (m_video_decoder)
    {
        find_decoder(m_video_decoder);
        log_d("video decoder play");
    }

}

void IvyAnim::find_decoder(std::shared_ptr<AnimDriver> decoder)
{
    if (decoder)
        decoder->begin();
}