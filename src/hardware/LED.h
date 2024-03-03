/**
 * @file LED.h
 * 
 * @brief Interface for handling LEDs
 */

#pragma once

/**
 * @class Abstract interface class for an LED
 */
class LED {
    public:
        /**
         * @enum Type
         * @brief Type of LED
         * @details Supported types are standard LEDs which are directly connected to a GPIO pin and addressable 
         *          WS2811, WS2812 (NeoPixel) LEDs connected to the Status LED pin
         */
        enum Type {
            STANDARD,
            WS2811,
            WS2812,
            WS2812B
        };

        virtual void turnOn() = 0;
        virtual void turnOff() = 0;
        virtual void setColor(int hue, int saturation, int value) = 0;
        virtual void setBrightness(int value) = 0;
        virtual ~LED() {}
};
