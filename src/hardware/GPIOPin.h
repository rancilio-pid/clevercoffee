/**
 * @file GPIOPin.h
 *
 * @brief Abstraction of a GPIO pin
 */

#pragma once

#include <Arduino.h>

/**
 * @class GPIOPin
 * @brief Interface class to general purpose I/O pins
 * @details This class provides a convenient interface to configure GPIO pins as output or input with different options such
 *          as pull-up or pull-down.
 */
class GPIOPin {
    public:
        /**
         * @enum Type
         * @brief Type of GPIO pin
         */
        enum Type {
            OUT,
            IN_STANDARD,
            IN_PULLUP,   // Internal pull-up resistor
            IN_PULLDOWN, // Internal pull-down resistor
            IN_HARDWARE, // External pull-up/pull-down resistor
            IN_ANALOG
        };

        /**
         * @brief Constructor
         *
         * @param pinNumber Number of the GPIO pin of the respective hardware
         * @param pinType Type the pin should be configured as
         */
        GPIOPin(int pinNumber, Type pinType);

        /**
         * @brief Write value to pin
         * @details Writes the provided boolean value to the pin if it is configured as output pin, otherwise ignores call
         *
         * @param value Boolean value to set the pin to
         */
        void write(bool value) const;

        /**
         * @brief Read value from pin
         * @details Reads the value from this GPIO pin, either as analog value if configured as analog input, or as digital
         *          value
         * @return Value read from GPIO pin
         */
        int read() const;

        /**
         * @brief Returns configured type of this GPIO pin
         * @return Configured type of this pin
         */
        Type getType() const;

        /**
         * @brief Returns pin number of this GPIO pin
         * @return Pin number of this pin
         */
        int getPinNumber() const;

    private:
        /**
         * @brief Set the type of the GPIO pin
         *
         * @param pinType Desired type of the pin
         */
        void setType(Type pinType);

        int pin;
        Type pinType;
};
