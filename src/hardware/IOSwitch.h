/**
 * @file GPIOPin.h
 * 
 * @brief A physical switch connected to a GPIO Pin  
 */

#pragma once

#include "Switch.h"

class GPIOPin;

class IOSwitch : public Switch {
    public:
        IOSwitch(GPIOPin& gpioInstance, Type type = MOMENTARY);

        bool isPressed() override;
        bool longPressDetected() override;
        void setType(Type type);
        GPIOPin& getGPIOInstance();

    private:
        GPIOPin& gpio;
        Type switchType;
        uint8_t lastState;
        uint8_t currentState;
        unsigned long lastDebounceTime;
        unsigned long lastStateChangeTime;
        unsigned long pressStartTime;
        bool longPressTriggered;

        const unsigned long debounceDelay;
        const unsigned long longPressDuration;
};
