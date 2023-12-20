/**
 * @file Relay.cpp
 * 
 * @brief A relay connected to a GPIO pin
 */

#include "GPIOPin.h"
#include "Relay.h"

Relay::Relay(GPIOPin& gpioInstance, RelayTriggerType trigger) : gpio(gpioInstance), relayTrigger(trigger) {

}

void Relay::on() {
    if (relayTrigger == HIGH_TRIGGER) {
        gpio.write(HIGH);
    } 
    else {
        gpio.write(LOW);
    }
}

void Relay::off() {
    if (relayTrigger == HIGH_TRIGGER) {
        gpio.write(LOW);
    } 
    else {
        gpio.write(HIGH);
    }
}

GPIOPin& Relay::getGPIOInstance() {
    return gpio;
}
