/**
 * @file GPIOPin.cpp
 *
 * @brief Abstraction of a GPIO pin
 */

#include "GPIOPin.h"

GPIOPin::GPIOPin(const int pinNumber, const Type pinType) :
    pin(pinNumber), pinType(pinType) {

    setType(pinType);
}

void GPIOPin::write(const bool value) const {
    if (pinType == OUT) {
        digitalWrite(pin, value);
    }
}

int GPIOPin::read() const {
    if (pinType == IN_ANALOG) {
        return analogRead(pin);
    }

    return digitalRead(pin);
}

GPIOPin::Type GPIOPin::getType() const {
    return pinType;
}

void GPIOPin::setType(const Type pinType) const {
    switch (pinType) {
        case OUT:
            pinMode(pin, OUTPUT);
            break;
        case IN_STANDARD:
            pinMode(pin, INPUT);
            break;
        case IN_PULLUP:
            pinMode(pin, INPUT_PULLUP);
            digitalWrite(pin, HIGH);
            break;
        case IN_PULLDOWN:
            pinMode(pin, INPUT_PULLDOWN);
            digitalWrite(pin, LOW);
            break;
        case IN_HARDWARE:
        case IN_ANALOG:
            pinMode(pin, INPUT);
            break;
    }
}
