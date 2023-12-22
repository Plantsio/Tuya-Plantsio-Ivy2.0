//
// Created by Huwensong on 2023/12/12.
//

#include "AudioCodec.h"

//esp-idf
#include "driver/i2s.h"

//arduino
#include "SD_MMC.h"

#define AUDIO_I2S_BCLK      13
#define AUDIO_I2S_LRC       14
#define AUDIO_I2S_DOUT      40

#define SD_CLK              45
#define SD_CMD              47
#define SD_D0               48

const char* mount_name = "/sd";

AudioCodec::AudioCodec() : aac_decoder(data_callback)
{
}

AudioCodec::~AudioCodec()
{
    free(audio_buffer);
}

void AudioCodec::init()
{
    if (init_i2s() && init_sd())
    {
        init_decoder();
        audio_buffer = (uint8_t *)heap_caps_calloc(AAC_MAX_FRAME_SIZE,1,MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
        init_finish = true;
        log_d("sd and i2s init succeed");
    }
    else
    {
        log_e("sd and i2s init failed");
    }
}

void AudioCodec::begin()
{
    int ret = xTaskCreatePinnedToCore(play_routine_wrapper,"AudioDecoder",1024 * 4, this,6,
                            nullptr,1);
    if (ret != pdPASS) {
        log_e("AudioDecoder task ret %d", ret);
    }
}

void AudioCodec::routine()
{
    if (init_finish && file_ready)
    {
        audio_file.readBytes((char *)audio_buffer,AAC_MAX_FRAME_SIZE);
        aac_decoder.write(audio_buffer,AAC_MAX_FRAME_SIZE);
    }
    else
        log_e("init or file open failed!");

}

void AudioCodec::audio_bind_asset(const char *asset_name)
{
    audio_file = SD_MMC.open(asset_name);

    if (!audio_file.available())
        log_e("Failed to bind audio asset filed");
    else
        file_ready = true;
}

bool AudioCodec::init_i2s()
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

bool AudioCodec::init_sd()
{
    if(SD_MMC.setPins(SD_CLK,SD_CMD,SD_D0) && SD_MMC.begin(mount_name, true))
    {
        log_d("sd init succeed");
        return true;
    }
    else
    {
        log_e("sd init failed");
        return false;
    }
}

void AudioCodec::init_decoder()
{
    aac_decoder.begin();
}

void AudioCodec::play_routine()
{
    int read_ret = 0;
    int write_ret = 0;

    while ((read_ret = audio_file.read(audio_buffer,AAC_MAX_FRAME_SIZE)) != -1)
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

void AudioCodec::play_routine_wrapper(void *param)
{
    static_cast<AudioCodec *>(param)->play_routine();
}

void AudioCodec::data_callback(AACFrameInfo &info,int16_t *pwm_buffer,size_t len,void* caller)
{
    static int _samprate = 0;
    if (_samprate != info.sampRateOut)
    {
        i2s_set_clk(I2S_NUM_0, info.sampRateOut /* sample_rate */, info.bitsPerSample /* bits_cfg */, (info.nChans == 2) ? I2S_CHANNEL_STEREO : I2S_CHANNEL_MONO /* channel */);
        _samprate = info.sampRateOut;
    }
    size_t i2s_bytes_written = 0;
    i2s_write(I2S_NUM_0, pwm_buffer, len * 2, &i2s_bytes_written, portMAX_DELAY);
}


