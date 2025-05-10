/**
 * @file StandardLED.cpp
 *
 * @brief An LED connected to a GPIO pin
 */

#include "StandardLED.h"
#include "GPIOPin.h"

StandardLED::StandardLED(GPIOPin& gpioInstance, int featureFlag) :
    gpio(gpioInstance), enabled(featureFlag != 0), inverted(featureFlag == 2) {
}

void StandardLED::setGPIOState(bool state) {
    if (enabled) {
        if (inverted) {
            gpio.write(state ? LOW : HIGH); // Inverted logic
        }
        else {
            gpio.write(state ? HIGH : LOW); // Normal logic
        }
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
