//
// Created by Huwensong on 2023/12/12.
//

#include "AudioDriver.h"

//esp-idf
#include "driver/i2s.h"

#define AUDIO_I2S_BCLK      13
#define AUDIO_I2S_LRC       14
#define AUDIO_I2S_DOUT      40

AudioDriver::AudioDriver()
{
}

AudioDriver::~AudioDriver()
{
}

void AudioDriver::prepare_resource()
{
    aac_decoder = new Audio();
    aac_decoder->setPinout(AUDIO_I2S_BCLK,AUDIO_I2S_LRC,AUDIO_I2S_DOUT);
    aac_decoder->setVolume(5);
}

void AudioDriver::release_resource()
{
    stop();
    delete (aac_decoder);
}

void AudioDriver::begin()
{
    int ret = xTaskCreatePinnedToCore(play_routine_wrapper,"AudioDecoder",1024 * 6, this,10,
                                      nullptr,0);
    if (ret != pdPASS) {
        log_e("Decoder task ret %d", ret);
    }
}

void AudioDriver::stop()
{
    if (aac_decoder->isRunning())
    {
        aac_decoder->stopSong();
    }
}

bool AudioDriver::anim_bind_assets(const char *assets_name)
{
    aac_decoder->connecttoFS(SD_MMC,assets_name);
    return true;
}

void AudioDriver::play_routine_wrapper(void *param)
{
    static_cast<AudioDriver *>(param)->play_routine();
}

void AudioDriver::play_routine()
{
    while (true)
    {
        aac_decoder->loop();
    }
}


