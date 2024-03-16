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
         *          WS2812 LEDs (NeoPixel) connected to the Status LED pin
         */
        enum Type {
            STANDARD,
            WS2812
        };

        virtual void turnOn() = 0;
        virtual void turnOff() = 0;
        virtual void setColor(int red, int green, int blue) = 0;
        virtual void setBrightness(int value) = 0;
        virtual ~LED() {}
};
