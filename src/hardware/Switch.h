/**
 * @file LED.h
 *
 * @brief Interface that switches have to implement
 */

#pragma once

/**
 * @class Abstract interface class for a switch
 */
class Switch {
    public:
        /**
         * @enum Type
         * @brief Type of switch
         * @details Supported switches are toggle switches which remain on until toggled back, or momentary switches which
         *          only provide a single trigger impulse.
         */
        enum Type {
            MOMENTARY,
            TOGGLE
        };

        enum Mode {
            NORMALLY_OPEN,
            NORMALLY_CLOSED
        };

        virtual bool isPressed() = 0;
        virtual bool longPressDetected() = 0;
        virtual void setType(Type type) = 0;
        virtual ~Switch() {
        }
};
