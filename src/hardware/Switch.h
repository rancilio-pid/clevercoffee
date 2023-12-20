/**
 * @file LED.h
 * 
 * @brief Interface that switches have to implement
 */

#pragma once

enum SwitchType {
    SWITCHTYPE_MOMENTARY,
    SWITCHTYPE_TOGGLE
};

class Switch {
    public:
        virtual bool isPressed() = 0;
        virtual bool longPressDetected() = 0;
        virtual void setType(SwitchType type) = 0;
        virtual ~Switch() {}
};
