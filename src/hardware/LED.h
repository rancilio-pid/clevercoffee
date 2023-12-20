/**
 * @file LED.h
 * 
 * @brief Interface for handling LEDs
 */

#pragma once

enum LEDType {
    STANDARD_LED,
    WS2812B
};

class LED {
    public:
        virtual void turnOn() = 0;
        virtual void turnOff() = 0;
        virtual void setColor(int red, int green, int blue) = 0;
        virtual void setBrightness(int value) = 0;
        virtual ~LED() {}
};
