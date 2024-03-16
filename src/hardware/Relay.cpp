/**
 * @file Relay.cpp
 *
 * @brief A relay connected to a GPIO pin
 */

#include "Relay.h"
#include "GPIOPin.h"

Relay::Relay(GPIOPin& gpioInstance, TriggerType trigger) :
    gpio(gpioInstance), relayTrigger(trigger) {
}

void Relay::on() const {
    if (relayTrigger == HIGH_TRIGGER) {
        gpio.write(HIGH);
    }
    else {
        gpio.write(LOW);
    }
}

void Relay::off() const {
    if (relayTrigger == HIGH_TRIGGER) {
        gpio.write(LOW);
    }
    else {
        gpio.write(HIGH);
    }
}

GPIOPin& Relay::getGPIOInstance() const {
    return gpio;
}
