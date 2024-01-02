//
// Created by Huwensong on 2023/12/12.
//

#ifndef FIRMWARE_AUDIODRIVER_H
#define FIRMWARE_AUDIODRIVER_H

#include <string>

#include "AACDecoderHelix.h"
#include "AnimDriver.h"

typedef enum CodecType
{
    AAC,
    MP3,
}codec_type_t;


using namespace libhelix;

class AudioDriver : public AnimDriver
{
public:
    AudioDriver();
    ~AudioDriver();

public:
    void prepare_resource() override;

    void release_resource() override;

    void stop() override;

private:
    void play_routine() ;

private:
    bool init_i2s();

    static void data_callback(AACFrameInfo &info,int16_t *pwm_buffer,size_t len,void* caller);

private:
    AACDecoderHelix aac_decoder;

    uint8_t *audio_buffer = nullptr;

    bool init_finish = false;
    bool file_ready  = false;
};

#endif //FIRMWARE_AUDIODRIVER_H
