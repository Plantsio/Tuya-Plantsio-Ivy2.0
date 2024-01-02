//
// Created by Gordon on 2023/6/14.
//

#ifndef MI_PLANTS_IVY_MJPEGDRIVER_H
#define MI_PLANTS_IVY_MJPEGDRIVER_H

#include "MjpegCodec.h"
#include "mutex"

#include "AnimDriver.h"

class MjpegDriver : public AnimDriver
{
public:
    MjpegDriver(const MjpegDriver &) = delete;

    MjpegDriver(MjpegDriver &&) = delete;

    MjpegDriver &operator=(const MjpegDriver &) = delete;

    MjpegDriver &operator=(MjpegDriver &&) = delete;

public:
    void prepare_resource() override;

    void release_resource() override;

    bool anim_bind_assets(const char *assets_name) override;

    void stop() override;

public:
    typedef enum {
        normal = 0,
        complete = 1,
        error = 2,
    } play_status_t;

    MjpegDriver() = default;

    void drv_activation_cb(bool activate);

public:

    bool is_playing() const {
        return m_playing;
    }

    void set_repeat(int repeat) {
        m_repeat = repeat;
    }

private:
    void reset_play_status();

    play_status_t play_frame();

    void play_routine() override;

private:
    std::mutex m_lock;

    bool m_playing = false;
    MjpegCodec *m_decoder = nullptr;
    uint8_t m_fps = 25;

    //region player management
    int m_repeat = 0;
    uint32_t m_current_repeat = 0;

    uint32_t m_next_frame_ms = 0;
    uint32_t m_start_ms = 0;
    uint32_t m_next_frame = 0;

    bool m_is_image = false;
    uint32_t m_current_frames = 0;
    //endregion

    bool m_faded = false;

};


#endif //MI_PLANTS_IVY_MJPEGDRIVER_H
