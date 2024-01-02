//
// Created by Huwensong on 2023/12/22.
//

#include "Screen.h"

//
// Created by Huwensong on 2023/12/22.
//

//#include "AnimDriver.h"

#define SCR_BCK_GPIO 1

Bck::Bck() : LED(SCR_BCK_GPIO) {

}

Screen::Screen() : TFT_eSPI() {
    /* tft initialization */
    //fade_off(true, 10);
    init();
    setRotation(1);
    fillScreen(TFT_BLACK);
    initDMA();
    setSwapBytes(true);
    /* todo
     * should ensure visible brightness under:
     * 1. normal boot
     * 2. reconfigure, re-activation and their related ui
     * 3. tutorial
     * 4. OTA
     * 5. other situations where manual setting of brightness is not possible
     * */
}

void
Screen::print_raw(const char *content, uint16_t fg_color, uint16_t bg_color, uint16_t delay, int16_t x, int16_t y) {
    fillScreen(TFT_BLACK);
    if (delay) {
        vTaskDelay(delay);
    }
    setCursor(x, y, 4);
    setTextColor(fg_color, bg_color);
    setTextSize(1);
    println(content);
}

void Screen::clear() {
    fillScreen(TFT_BLACK);
}

void Screen::sleep() {
    writecommand(0x10);
    delay(5);
}

void Screen::wakeup() {
    writecommand(0x11);
    delay(120);
}

void Screen::set_idle(bool set) {
    if (set) {
        writecommand(0x38);
        delay(120);
    } else {
        writecommand(0x39);
        delay(120);

    }

}

void Screen::fade_off(bool block, int t) {
    Bck::instance().fade_to_end(false, block, t);
}





