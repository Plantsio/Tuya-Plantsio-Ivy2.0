//
// Created by Gordon on 2023/6/14.
//

#include "MjpegDriver.h"

void MjpegDriver::drv_activation_cb(bool activate)
{
    if (activate) {
        disableCore0WDT();
    } else {
        stop();
        enableCore0WDT();
    }
}

void MjpegDriver::prepare_resource() {
//    mem_print("mjpeg 1");
    //IvyAnim::instance().release_resources();
//    mem_print("mjpeg 2");
    Screen &scrren = Screen::instance();
    m_decoder = new MjpegCodec();
//    mem_print("mjpeg 3");
    log_v("init mjpeg driver decoder %d", m_decoder);
}

void MjpegDriver::release_resource() {
    delete (m_decoder);
    //IvyAnim::instance().acquire_resources();
}

bool MjpegDriver::anim_bind_assets(const char *assets_name)
{
    if (is_playing())
    {
        return false;
    }

    m_playing = true;

    return AnimDriver::anim_bind_assets(assets_name);
}

MjpegDriver::play_status_t MjpegDriver::play_frame()
{
    std::lock_guard<std::mutex> _lock(m_lock);
    if (!m_playing) {
        return complete;
    }
    if (m_is_image) {
        return normal;
    }
    while (true) {
        if (!m_anim_file.available()) {
            /* todo global sleep */
            if (m_repeat == -1 || m_current_repeat < m_repeat) {
                if (m_current_frames == 1) {
                    /* only one frame, this is an image */
                    m_is_image = true;
                    return normal;
                } else {
                    /* repeat */
                    m_current_frames = 0;
                    m_start_ms = 0;
                    m_current_repeat++;
                    m_anim_file.seek(0);
                    if (m_decoder)
                        m_decoder->reset_input_status();
                }
            } else {
                stop();
                return complete;
            }
        }

        auto data = m_decoder->read_frame(&m_anim_file);

        if (!data) {
            stop();
            return error;
        }
        bool skip = (millis() > m_next_frame_ms) && (m_start_ms > 0);   /* do not skip for 1st frame*/

        if (!skip) {
            if (!m_decoder->decode_frame(data)) {
                log_w("Mjpeg player decoding failed!");
            }
            while (millis() < m_next_frame_ms) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        } else {
            free(data);
            log_d("skip");
        }

        if (m_start_ms == 0) {
            /* first frame */
            if (!m_faded) {
                m_faded = true;
                //Screen::auto_fade(false, 500);
            }
            m_start_ms = millis();
            m_next_frame_ms = m_start_ms + (++m_current_frames * 1000 / m_fps);
        } else {
            m_next_frame_ms = m_start_ms + (++m_current_frames * 1000 / m_fps);
        }

       // float frames = (float)(1000.0) / ((float)(millis() - m_start_ms) / (float)m_current_frames);

        //log_d("fps: %f",frames);

        if (!skip) {
            return normal;
        }
    }
}

void MjpegDriver::reset_play_status()
{
    m_current_repeat = 0;
    m_start_ms = 0;
//    m_next_frame = 0;
    m_next_frame_ms = 0;
    if (m_anim_file) {
        m_anim_file.close();
    }
    m_decoder->reset_input_status();
    m_current_frames = 0;
    m_is_image = false;
    if (!m_faded) {
        //Screen::auto_fade(false, 500);
    }
    m_faded = false;
}

void MjpegDriver::stop() {
    log_v("debug-player stopping %s", m_anim_file.name());
    if (m_playing) {
        {
            std::lock_guard<std::mutex> _lock(m_lock);
            m_playing = false;
        }
        /* wait until the two decoder tasks finished current frame processing */
        m_decoder->stop();
//        m_decoder->wait_processing();
        reset_play_status();
    }
}



void MjpegDriver::play_routine()
{
    while (true)
    {
        play_frame();
        vTaskDelay(10);
    }
}
