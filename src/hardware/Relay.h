/**
 * @file Relay.h
 * 
 * @brief A relay connected to a GPIO pin
 */
#pragma once

class GPIOPin;

enum RelayTriggerType {
    LOW_TRIGGER,
    HIGH_TRIGGER
};

class Relay {
    public:        
        Relay(GPIOPin& gpioInstance, RelayTriggerType trigger = HIGH_TRIGGER);

        void on();
        void off();
        GPIOPin& getGPIOInstance();

    private:
        GPIOPin& gpio;
        RelayTriggerType relayTrigger;
};
