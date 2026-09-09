/**
 * @file Relay.h
 *
 * @brief A relay connected to a GPIO pin
 */
#pragma once

#include "pumpControl.h"

// Forward declaration of GPIOPin class
class GPIOPin;

/**
 * @file Relay.h Relay control class
 * @brief This class provides control for relay switches
 */
class Relay : public PumpControl {
    public:
        /**
         * @enum TriggerType
         * @brief Type of trigger for this relay
         * @details Relays can either trigger in HIGH or LOW setting of their control input
         */
        enum TriggerType {
            LOW_TRIGGER,
            HIGH_TRIGGER
        };

        /**
         * @brief Constructor
         *
         * @param gpioInstance GPIO pin this relay is connected to
         * @param trigger Trigger type this relay requires
         */
        explicit Relay(GPIOPin& gpioInstance, TriggerType trigger = HIGH_TRIGGER);

        /**
         * @brief Switch relay on
         */
        void on();

        /**
         * @brief Switch relay off
         */
        void off();

        /**
         * @brief read current state of output
         */
        bool getState() const override {
            return lastState;
        }

        PumpControlType getType() const override;

        /**
         * @brief Get the GPIO pin this relay is connected to
         * @return GPIO pin of the relay
         */
        [[nodiscard]] GPIOPin& getGPIOInstance() const;

    private:
        GPIOPin& gpio;
        TriggerType relayTrigger;
        bool lastState = false;
};
