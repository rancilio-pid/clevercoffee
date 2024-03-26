/**
 * @file StandardLED.h
 *
 * @brief An LED connected to a GPIO pin
 */

#pragma once

#include "LED.h"

class GPIOPin;

class StandardLED : public LED {
    public:
        StandardLED(GPIOPin& gpioInstance);

        void turnOn() override;
        void turnOff() override;
        void toggle() override;
        void setColor(int red, int green, int blue);
        void setBrightness(int value);

    private:
        GPIOPin& gpio;
};
