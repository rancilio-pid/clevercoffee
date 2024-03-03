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
        void setColor(int hue, int saturation, int value);
        void setBrightness(int value);

    private:
        GPIOPin& gpio;
};
