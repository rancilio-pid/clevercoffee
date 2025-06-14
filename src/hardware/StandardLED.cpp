/**
 * @file StandardLED.cpp
 *
 * @brief An LED connected to a GPIO pin
 */

#include "StandardLED.h"
#include "GPIOPin.h"

StandardLED::StandardLED(GPIOPin& gpioInstance, const bool inverted) :
    gpio(gpioInstance), inverted(inverted) {
}

void StandardLED::setGPIOState(bool state) {
    if (inverted) {
        gpio.write(state ? LOW : HIGH); // Inverted logic
    }
    else {
        gpio.write(state ? HIGH : LOW); // Normal logic
    }
}

void StandardLED::turnOn() {
    setGPIOState(true); // Turn on
}

void StandardLED::turnOff() {
    setGPIOState(false); // Turn off
}

void StandardLED::setColor(int red, int green, int blue) {
    // Not applicable for standard LEDs
}

void StandardLED::setBrightness(int value) {
    // Not applicable for standard LEDs
}
