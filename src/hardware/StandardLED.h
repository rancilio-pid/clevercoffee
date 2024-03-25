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
        StandardLED(GPIOPin& gpioInstance, unsigned int blinking_time = 750);

        void turnOn() override;
        void turnOff() override;
        void blink( unsigned long currentTime ) override;
        void setColor(int red, int green, int blue);
        void setBrightness(int value);


    private:
        GPIOPin& gpio;
};
