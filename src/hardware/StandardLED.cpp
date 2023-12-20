/**
 * @file StandardLED.cpp
 * 
 * @brief An LED connected to a GPIO pin
 */

#include "GPIOPin.h"
#include "StandardLED.h"

StandardLED::StandardLED(GPIOPin& gpioInstance) : gpio(gpioInstance) {

}

void StandardLED::turnOn() {
    gpio.write(HIGH);
}

void StandardLED::turnOff() {
    gpio.write(LOW);
}

void StandardLED::setColor(int red, int green, int blue) {
    // Not applicable for standard LEDs
}

void StandardLED::setBrightness(int value) {
    // Not applicable for standard LEDs
}