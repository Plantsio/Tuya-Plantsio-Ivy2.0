//
// Created by Huwensong on 2023/12/12.
//

#ifndef FIRMWARE_AUDIOCODEC_H
#define FIRMWARE_AUDIOCODEC_H

#include <string>
#include "AACDecoderHelix.h"

#include "FS.h"


using namespace libhelix;

class AudioCodec
{
public:
    AudioCodec();
    ~AudioCodec();

public:
    void init();

    void begin();

    void routine();

    void audio_bind_asset(const char *asset_name);

private:
    bool init_i2s();
    bool init_sd();
    void init_decoder();

    void play_routine();

    static void play_routine_wrapper(void *param);

    static void data_callback(AACFrameInfo &info,int16_t *pwm_buffer,size_t len,void* caller);

private:
    AACDecoderHelix aac_decoder;
    File audio_file;

    uint8_t *audio_buffer = nullptr;

    bool init_finish = false;
    bool file_ready  = false;
};

#endif //FIRMWARE_AUDIOCODEC_H
