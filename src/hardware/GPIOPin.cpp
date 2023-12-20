/**
 * @file GPIOPin.cpp
 * 
 * @brief Abstraction of a GPIO pin 
 */

#include "GPIOPin.h"

GPIOPin::GPIOPin(int pinNumber, PinType pinType)
    : pin(pinNumber), pinType(pinType) {

    setPinType(pinType);
}

void GPIOPin::write(bool value) {
    if (pinType == GPIO_OUTPUT) {
        digitalWrite(pin, value);
    }
}

int GPIOPin::read() {
    if (pinType == GPIO_INPUT_ANALOG) {
            return analogRead(pin);
    }
    else {
        return digitalRead(pin);
    }
}

PinType GPIOPin::getPinType() {
    return pinType;
}

void GPIOPin::setPinType(PinType pinType) {
    switch (pinType) {
        case GPIO_OUTPUT:
            pinMode(pin, OUTPUT);
            break;
        case GPIO_INPUT_STANDARD:
            pinMode(pin, INPUT);
            break;
        case GPIO_INPUT_PULLUP:
            pinMode(pin, INPUT_PULLUP);
            digitalWrite(pin, HIGH);
            break;
        case GPIO_INPUT_PULLDOWN:
            pinMode(pin, INPUT_PULLDOWN);
            digitalWrite(pin, LOW);
            break;
        case GPIO_INPUT_HARDWARE:
            pinMode(pin, INPUT);
            break;
        case GPIO_INPUT_ANALOG:
            pinMode(pin, INPUT);
            break;
    }
}