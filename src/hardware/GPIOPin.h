/**
 * @file GPIOPin.h
 * 
 * @brief Abstraction of a GPIO pin 
 */

#pragma once

#include <Arduino.h>

enum PinType {
    GPIO_OUTPUT,
    GPIO_INPUT_STANDARD,
    GPIO_INPUT_PULLUP,      // Internal pull-up resistor
    GPIO_INPUT_PULLDOWN,    // Internal pull-down resistor
    GPIO_INPUT_HARDWARE,    // External pull-up/pull-down resistor
    GPIO_INPUT_ANALOG
};

class GPIOPin {
    public:
        GPIOPin(int pinNumber, PinType pinType);

        void write(bool value);
        void setPinType(PinType pinType);
        int read();
        PinType getPinType();

    private:
        int pin;
        PinType pinType;
};

