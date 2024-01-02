//
// Created by Huwensong on 2023/12/12.
//

#include "AudioDriver.h"

//esp-idf
#include "driver/i2s.h"

#define AUDIO_I2S_BCLK      13
#define AUDIO_I2S_LRC       14
#define AUDIO_I2S_DOUT      40

AudioDriver::AudioDriver() : aac_decoder(data_callback)
{
    audio_buffer = (uint8_t *)heap_caps_calloc(AAC_MAX_FRAME_SIZE,sizeof(uint8_t),MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
}

AudioDriver::~AudioDriver()
{
    free(audio_buffer);
}

void AudioDriver::prepare_resource()
{

    if (init_i2s() && audio_buffer)
    {
        aac_decoder.begin();
        log_d("audio prepare_resource succeed");
    }
    else
    {
        log_e("audio prepare_resource failed");
    }
}

void AudioDriver::release_resource()
{
    aac_decoder.end();
    free(audio_buffer);
}

void AudioDriver::stop()
{
    log_d("xxxxxxxxxxxxxx");
}

bool AudioDriver::init_i2s()
{
    i2s_config_t i2s_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
            .sample_rate = 44100,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 16,
            .dma_buf_len = 512,
            .use_apll = false,
            .tx_desc_auto_clear = true,
            .fixed_mclk = I2S_PIN_NO_CHANGE,
            .mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT,
            .bits_per_chan = I2S_BITS_PER_CHAN_16BIT,
    };

    i2s_pin_config_t i2s_pin_config = {
            .mck_io_num   = I2S_PIN_NO_CHANGE,
            .bck_io_num   = AUDIO_I2S_BCLK,
            .ws_io_num    = AUDIO_I2S_LRC,
            .data_out_num = AUDIO_I2S_DOUT,
            .data_in_num  = I2S_PIN_NO_CHANGE,
    };

    esp_err_t ret = ESP_OK;

    ret = i2s_driver_install(I2S_NUM_0,&i2s_config,0, nullptr);

    if (ret != ESP_OK)
    {
        log_e("Audio i2s driver install failed ret = 0x%x",ret);
        return false;
    }

    ret = i2s_set_pin(I2S_NUM_0,&i2s_pin_config);

    if (ret != ESP_OK)
    {
        log_e("Audio i2s set pin failed ret = 0x%x",ret);
        return false;
    }

    log_d("i2s init succeed");

    return true;
}


void AudioDriver::play_routine()
{
    int read_ret = 0;
    int write_ret = 0;

    while ((read_ret = m_anim_file.read(audio_buffer,AAC_MAX_FRAME_SIZE)) != -1)
    {
        while (read_ret >0)
        {
            write_ret = aac_decoder.write(audio_buffer,read_ret);
            read_ret -= write_ret;
            vTaskDelay(10);
        }
        vTaskDelay(10);
    }
    vTaskDelete(nullptr);
}

void AudioDriver::data_callback(AACFrameInfo &info,int16_t *pwm_buffer,size_t len,void* caller)
{
    int16_t *buff = (int16_t *) heap_caps_malloc(len,MALLOC_CAP_SPIRAM);

    memcpy(buff,pwm_buffer,len);

    for (int i = 0; i < len; i ++)
    {
        buff[i] = buff[i] / 2;
    }

    static int _samprate = 0;
    if (_samprate != info.sampRateOut)
    {
        i2s_set_clk(I2S_NUM_0, info.sampRateOut /* sample_rate */, info.bitsPerSample /* bits_cfg */, (info.nChans == 2) ? I2S_CHANNEL_STEREO : I2S_CHANNEL_MONO /* channel */);
        _samprate = info.sampRateOut;
    }
    size_t i2s_bytes_written = 0;
    i2s_write(I2S_NUM_0, buff, len * 2, &i2s_bytes_written, portMAX_DELAY);
    free(buff);
}


