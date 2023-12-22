//
// Created by Gordon on 2023/6/8.
//

#ifndef MJPEGTEST_MJPEGCODEC_H
#define MJPEGTEST_MJPEGCODEC_H

#include "JPEGDEC.h"
#include "mutex"
#include "freertos/event_groups.h"
#include "Arduino.h"
#include "Screen.h"

#define READ_BUFFER_SIZE            1024
#define MJPEG_DECODER_BUF_SIZE      (320 * 240 * 2 / 8)
#define NUMBER_OF_DECODE_BUFFER     3
#define NUMBER_OF_DRAW_BUFFER       5
#define MJPEG_DECODE_CORE           1
#define MJPEG_DRAW_CORE             0

/* this class is designed to be allocated in heap */
class MjpegCodec {
public:
    typedef enum {
        draw_task_stopped = BIT1,
        decode_task_stopped = BIT2,
        draw_task_finish_processing = BIT3,
        decode_task_finish_processing = BIT4,
    } event_t;

    typedef struct {
        int32_t size;
        uint8_t* data;
    } decode_buf_t;

    MjpegCodec();

    ~MjpegCodec();

    //region external control
    /* for each routine
     * 1. call read_frame
     * 2. call decode_frame to process the actual decoding or skip
     * 3. call stop before delete instance
     * */
    uint8_t *read_frame(Stream *input);

    bool decode_frame(uint8_t*data);

    bool stop();

    bool exit();

    void reset_input_status();
    //endregion

    bool running();

    bool wait_processing(uint32_t timeout=2000);
private:
    bool begin_draw_task();

    static void draw_task_wrapper(void *param) {
        /* static cast param to this, so we can use main get_percent in a callback */
        static_cast<MjpegCodec *>(param)->draw_task();
    }

    void draw_task();

    bool begin_decode_task();

    static void decode_task_wrapper(void *param) {
        /* static cast param to this, so we can use main get_percent in a callback */
        static_cast<MjpegCodec *>(param)->decode_task();
    }

    void decode_task();

private:
    typedef struct {
        xQueueHandle xqh;
        JPEG_DRAW_CALLBACK *drawFunc;
    } draw_task_param_t;

    typedef struct {
        xQueueHandle xqh;
        decode_buf_t *mBuf;
    } decode_task_param_t;
private:
    std::mutex m_draw_mutex;
    std::mutex m_decode_mutex;

    EventGroupHandle_t m_event_group;

    //region input control
    int32_t m_buf_read = 0;
    int32_t m_read_buf_offset = 0;
    uint32_t m_input_index = 0;
    //endregion

//    JPEGDEC m_jpegDec;

    //region buf control
    uint8_t m_read_buf[READ_BUFFER_SIZE]{};
    //endregion

    //region task control
    decode_task_param_t m_decode_task_param{};
    draw_task_param_t m_draw_task_param{};
    TaskHandle_t m_decode_task{};
    TaskHandle_t m_draw_task{};
    //endregion

};


#endif //MJPEGTEST_MJPEGCODEC_H
