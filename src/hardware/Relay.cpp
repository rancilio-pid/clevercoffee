/**
 * @file Relay.cpp
 *
 * @brief A relay connected to a GPIO pin
 */

#include "Relay.h"
#include "GPIOPin.h"

Relay::Relay(GPIOPin& gpioInstance, const TriggerType trigger) :
    gpio(gpioInstance), relayTrigger(trigger), lastState(false) {
}

void Relay::on() {
    gpio.write(relayTrigger == HIGH_TRIGGER ? HIGH : LOW);
    lastState = true;
}

void Relay::off() {
    gpio.write(relayTrigger == HIGH_TRIGGER ? LOW : HIGH);
    lastState = false;
}

GPIOPin& Relay::getGPIOInstance() const {
    return gpio;
}

PumpControlType Relay::getType() const {
    return PumpControlType::RELAY;
}
