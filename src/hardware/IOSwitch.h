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
        IOSwitch(GPIOPin& gpioInstance, SwitchType type = SWITCHTYPE_MOMENTARY);

        bool isPressed();
        bool longPressDetected();
        void setType(SwitchType type);
        GPIOPin& getGPIOInstance();

    private:
        GPIOPin& gpio;
        SwitchType switchType;
        uint8_t lastState;
        uint8_t currentState;
        unsigned long lastDebounceTime;
        unsigned long lastStateChangeTime;
        unsigned long pressStartTime;
        bool longPressTriggered;

        const unsigned long debounceDelay;
        const unsigned long longPressDuration;
};
