//
// Created by Huwensong on 2023/12/22.
//

#ifndef FIRMWARE_SCREEN_H
#define FIRMWARE_SCREEN_H

#include <memory>

#include "TFT_eSPI.h"
#include "LED.h"

class Bck : public LED {
public:
    //region Singleton
    /* todo use it or not use it, this is a question */
    static Bck &instance() {
        static std::shared_ptr<Bck> instance(new Bck());
        return *instance;
    }

    Bck(const Bck &) = delete;

    Bck(Bck &&) = delete;

    Bck &operator=(const Bck &) = delete;

    Bck &operator=(Bck &&) = delete;
    //endregion

    Bck();
};

class Screen : public TFT_eSPI {
public:
    //region Singleton
    /* todo use it or not use it, this is a question */
    static Screen &instance() {
        static std::shared_ptr<Screen> instance(new Screen());
        return *instance;
    }

    Screen(const Screen &) = delete;

    Screen(Screen &&) = delete;

    Screen &operator=(const Screen &) = delete;

    Screen &operator=(Screen &&) = delete;
    //endregion

    Screen();

    void print_raw(const char *content, uint16_t fg_color, uint16_t bg_color, uint16_t delay = 0, int16_t x = 20,
                   int16_t y = 100);

    void clear();

    void sleep();

    void wakeup();

    void set_idle(bool set);

    static void fade_off(bool block = false, int t = 500);
    //endregion
};

#endif //FIRMWARE_SCREEN_H
