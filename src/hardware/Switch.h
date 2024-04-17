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

        /**
         * @enum Mode
         * @brief Switch mode, normally-open or normally-closed
         * @details There are two types of switches, the ones that are closed by default (normally-closed, NC) or open
         *          (normally-open, NO)
         */
        enum Mode {
            NORMALLY_OPEN,
            NORMALLY_CLOSED
        };

        /**
         * @brief Constructor for a new switch
         *
         * @param type Switch type
         * @param mode Operation mode
         */
        Switch(Type type, Mode mode) :
            type_(type), mode_(mode){};
        Switch() = delete;
        virtual ~Switch() = default;

        virtual bool isPressed() = 0;
        virtual bool longPressDetected() = 0;

    protected:
        Type type_;
        Mode mode_;
};
