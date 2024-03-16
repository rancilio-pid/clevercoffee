/**
 * @file Relay.h
 *
 * @brief A relay connected to a GPIO pin
 */
#pragma once

// Forward declaration of GPIOPin class
class GPIOPin;

/**
 * @class Relay control class
 * @brief This class provides control for relay switches
 */
class Relay {
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
        Relay(GPIOPin& gpioInstance, TriggerType trigger = HIGH_TRIGGER);

        /**
         * @brief Switch relay on
         */
        void on() const;

        /**
         * @brief Switch relay off
         */
        void off() const;

        /**
         * @brief Get the GPIO pin this relay is connected to
         * @return GPIO pin of the relay
         */
        GPIOPin& getGPIOInstance() const;

    private:
        GPIOPin& gpio;
        TriggerType relayTrigger;
};
