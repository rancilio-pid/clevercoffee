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
        StandardLED(GPIOPin& gpioInstance, bool inverted);

        void turnOn() override;
        void turnOff() override;
        void setColor(int red, int green, int blue) override;
        void setBrightness(int value) override;
        void setGPIOState(bool state) override;

    private:
        GPIOPin& gpio;
        bool inverted; // If true, invert on/off behavior
        bool enabled;  // If false, the LED will be disabled
};
