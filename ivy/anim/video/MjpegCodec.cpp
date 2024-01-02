//
// Created by Gordon on 2023/6/8.
//

#include "MjpegCodec.h"

MjpegCodec::MjpegCodec()
        : m_event_group(xEventGroupCreate()) {
    xEventGroupSetBits(m_event_group, decode_task_stopped);
    xEventGroupSetBits(m_event_group, draw_task_stopped);

    if (begin_decode_task() == pdPASS) {
        log_i("Mjpeg decode task created!");
    } else {
        log_e("Mjpeg decode task creation failed!");
    }

    if (begin_draw_task() == pdPASS) {
        log_i("Mjpeg draw task created!");
    } else {
        log_e("Mjpeg draw task creation failed!");
    }
}

MjpegCodec::~MjpegCodec() {
    if (running()) {
        exit();
    }
    vEventGroupDelete(m_event_group);
}

uint8_t *MjpegCodec::read_frame(Stream *input) {
    auto *data = (uint8_t *) heap_caps_malloc_prefer(MJPEG_DECODER_BUF_SIZE, MALLOC_CAP_INTERNAL,
                                                     MALLOC_CAP_SPIRAM);
    uint32_t data_size = MJPEG_DECODER_BUF_SIZE;
    if (!data) {
        return nullptr;
    }
    if (m_input_index == 0) {
        m_buf_read = input->readBytes(m_read_buf, READ_BUFFER_SIZE);
        m_input_index += m_buf_read;
    }
    m_read_buf_offset = 0;
    int i = 0;
    bool found_FFD8 = false;
    while ((m_buf_read > 0) && (!found_FFD8)) {
        i = 0;
        while ((i < m_buf_read) && (!found_FFD8)) {
            if ((m_read_buf[i] == 0xFF) && (m_read_buf[i + 1] == 0xD8)) // JPEG header
            {
                // log_i("Found FFD8 at: %d.", i);
                found_FFD8 = true;
            }
            ++i;
        }
        if (found_FFD8) {
            --i;
        } else {
            m_buf_read = input->readBytes(m_read_buf, READ_BUFFER_SIZE);
        }
    }
    uint8_t *_p = m_read_buf + i;
    m_buf_read -= i;
    bool found_FFD9 = false;
    if (m_buf_read > 0) {
        i = 3;
        while ((m_buf_read > 0) && (!found_FFD9)) {
            if ((m_read_buf_offset > 0) && (data[m_read_buf_offset - 1] == 0xFF) &&
                (_p[0] == 0xD9)) // JPEG trailer
            {
                found_FFD9 = true;
            } else {
                while ((i < m_buf_read) && (!found_FFD9)) {
                    if ((_p[i] == 0xFF) && (_p[i + 1] == 0xD9)) // JPEG trailer
                    {
                        found_FFD9 = true;
                        ++i;
                    }
                    ++i;
                }
            }

            if (m_read_buf_offset + i > data_size) {
                log_v("debug reallocate");
                auto ret = (uint8_t *) heap_caps_realloc_prefer(data, m_read_buf_offset + i, MALLOC_CAP_INTERNAL,
                                                                MALLOC_CAP_SPIRAM);
                if (!ret) {
                    log_e("mem allocation error");
                    free(data);
                    return nullptr;
                }
                data_size = m_read_buf_offset + i;
                data = ret;
            }
            memcpy(data + m_read_buf_offset, _p, i);
            m_read_buf_offset += i;
            int32_t o = m_buf_read - i;
            if (o > 0) {
                memcpy(m_read_buf, _p + i, o);
                m_buf_read = input->readBytes(m_read_buf + o, READ_BUFFER_SIZE - o);
                _p = m_read_buf;
                m_input_index += m_buf_read;
                m_buf_read += o;
                // log_i("m_buf_read: %d", m_buf_read);
            } else {
                m_buf_read = input->readBytes(m_read_buf, READ_BUFFER_SIZE);
                _p = m_read_buf;
                m_input_index += m_buf_read;
            }
            i = 0;
        }
        if (found_FFD9) {
            // log_i("Found FFD9 at: %d.", m_read_buf_offset);
            if (m_read_buf_offset > data_size) {
                log_e("m_read_buf_offset(%d) > _mjpegBufufSize (%d)", m_read_buf_offset, data_size);
                free(data);
                return nullptr;
            }
            return data;
        }
    }
    free(data);
    return nullptr;
}

bool MjpegCodec::decode_frame(uint8_t *data) {
    decode_buf_t buf{.size=m_read_buf_offset, .data=data};
    xQueueSend(m_decode_task_param.xqh, &buf, portMAX_DELAY);
    return true;
}

bool MjpegCodec::begin_draw_task() {
    m_draw_task_param.xqh = xQueueCreate(NUMBER_OF_DRAW_BUFFER, sizeof(JPEGDRAW *));
    return
            xTaskCreatePinnedToCore(
                    (TaskFunction_t) draw_task_wrapper,
                    (const char *const) "draw",
                    (const uint32_t) 1024 * 4,
                    this,
                    (UBaseType_t) configMAX_PRIORITIES - 2,
                    (TaskHandle_t *const) &m_draw_task,
                    (const BaseType_t) MJPEG_DRAW_CORE) == pdPASS;
}

bool MjpegCodec::begin_decode_task() {
    m_decode_task_param.xqh = xQueueCreate(NUMBER_OF_DECODE_BUFFER, sizeof(decode_buf_t));
    return xTaskCreatePinnedToCore(
            (TaskFunction_t) decode_task_wrapper,
            (const char *const) "decoder",
            (const uint32_t) 1024 * 3,  /* todo smaller? */
            this,
            (UBaseType_t) configMAX_PRIORITIES - 2,
            (TaskHandle_t *const) &m_decode_task,
            (const BaseType_t) MJPEG_DECODE_CORE) == pdPASS;
}

void MjpegCodec::decode_task() {
    xEventGroupClearBits(m_event_group, decode_task_stopped);
    decode_buf_t buf;
    auto *m_jpegDec = (JPEGDEC *) heap_caps_malloc(sizeof(JPEGDEC), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!m_jpegDec) {
        m_jpegDec = (JPEGDEC *) heap_caps_malloc_prefer(sizeof(JPEGDEC), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT,
                                                        MALLOC_CAP_SPIRAM);
    }

    while (true) {
        if (xQueueReceive(m_decode_task_param.xqh, &buf, portMAX_DELAY) != pdTRUE) {
            log_e("debug decode error exit");
            break;
        }
        std::lock_guard<std::mutex> _lock(m_decode_mutex);
        xEventGroupClearBits(m_event_group, decode_task_finish_processing);
        if (buf.size == 0 && buf.data == nullptr) {
            /* exit */
            log_v("debug decode exit");
            break;
        }
        m_jpegDec->openRAM(buf.data, buf.size, m_draw_task_param.xqh);
        m_jpegDec->setMaxOutputSize(MAX_MCU);
        m_jpegDec->decode(0, 0, 0);
        m_jpegDec->close();
        free(buf.data);
        xEventGroupSetBits(m_event_group, decode_task_finish_processing);
    }
    free(m_jpegDec);
    vQueueDelete(m_decode_task_param.xqh);
    xEventGroupSetBits(m_event_group, decode_task_stopped);
    vTaskDelete(NULL);

}

void MjpegCodec::draw_task() {
    xEventGroupClearBits(m_event_group, draw_task_stopped);
    while (true) {
        JPEGDRAW *pDraw;
        if (xQueueReceive(m_draw_task_param.xqh, &pDraw, portMAX_DELAY) != pdTRUE) {
            log_e("debug draw error exit");
            break;
        }
        std::lock_guard<std::mutex> _lock(m_draw_mutex);
        xEventGroupClearBits(m_event_group, draw_task_finish_processing);
        if (pDraw->pPixels == nullptr) {
            /* exit */
            log_v("debug draw exit");
            break;
        }
        Screen::instance().startWrite();
        Screen::instance().pushImageDMA(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels,
                                        pDraw->pPixels);
        Screen::instance().endWrite();
        Screen::instance().dmaWait();

        free(pDraw->pPixels);
        free(pDraw);
        xEventGroupSetBits(m_event_group, draw_task_finish_processing);
    }
    vQueueDelete(m_draw_task_param.xqh);
    xEventGroupSetBits(m_event_group, draw_task_stopped);
    vTaskDelete(NULL);

}

bool MjpegCodec::stop() {
    std::lock_guard<std::mutex> lock_decode(m_decode_mutex);
    std::lock_guard<std::mutex> lock_draw(m_draw_mutex);

    decode_buf_t buf;
    while (true) {
        /* free all existing elements in the queue */
        if (xQueueReceive(m_decode_task_param.xqh, &buf, 0) == pdTRUE) {
            free(buf.data);
        } else {
            break;
        }
    }

    while (true) {
        JPEGDRAW *pDraw;
        /* free all existing elements in the queue */
        if (xQueueReceive(m_draw_task_param.xqh, &pDraw, 0) == pdTRUE) {
            free(pDraw->pPixels);
            free(pDraw);
        } else {
            break;
        }
    }
    return wait_processing();
}

bool MjpegCodec::exit() {
    decode_buf_t decode_stop_sig = {.size=0, .data=nullptr};
    uint32_t timeout = millis() + 3000;
    /* wait until all jobs are done before sending out the stop signal */
    stop();

    xQueueSend(m_decode_task_param.xqh, &decode_stop_sig, 2000);
    if (!(xEventGroupWaitBits(m_event_group, decode_task_stopped, false, true, 2000) & decode_task_stopped)) {
        log_w("Mjpeg decoder task failed to exit");
        return false;
    }

    JPEGDRAW pDraw;
    pDraw.pPixels = nullptr;
    auto draw_stop_sig = &pDraw;
    xQueueSend(m_draw_task_param.xqh, &draw_stop_sig, 2000);

    if (!(xEventGroupWaitBits(m_event_group, draw_task_stopped, false, true, 2000) & draw_task_stopped)) {
        log_w("Mjpeg draw task failed to exit");
        return false;
    }

    return true;
}

bool MjpegCodec::running() {
    auto condition = draw_task_stopped | decode_task_stopped;
    return (bool) ((xEventGroupGetBits(m_event_group) & condition) != condition);
}

void MjpegCodec::reset_input_status() {
    m_buf_read = 0;
    m_read_buf_offset = 0;
    m_input_index = 0;
}

bool MjpegCodec::wait_processing(uint32_t timeout) {
    auto condition = draw_task_finish_processing | decode_task_finish_processing;
    auto ret = xEventGroupWaitBits(m_event_group, condition, false, true, timeout);
    return (ret & condition) == condition;
}

