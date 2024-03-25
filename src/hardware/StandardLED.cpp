/**
 * @file StandardLED.cpp
 *
 * @brief An LED connected to a GPIO pin
 */

#include "StandardLED.h"
#include "GPIOPin.h"

StandardLED::StandardLED(GPIOPin& gpioInstance, unsigned int blinkDuration ) :
    gpio(gpioInstance) {
        this->blinkDuration = blinkDuration;
}

void StandardLED::turnOn() {
    gpio.write(HIGH);
    this->current_state = HIGH;
}

void StandardLED::turnOff() {
    gpio.write(LOW);
    this->current_state = LOW;
}

void StandardLED::setColor(int red, int green, int blue) {
    // Not applicable for standard LEDs
}

void StandardLED::setBrightness(int value) {
    // Not applicable for standard LEDs
}

void StandardLED::blink( unsigned long currentTime ) {
    if( currentTime > this->lastTimeBlink + this->blinkDuration) {
        //sufficient time has passed, let's toggle the LED
        switch (this->current_state)
        {
        case LOW:
            turnOn();
            break;

        case HIGH:
            turnOff();
            break;
        }

        this->lastTimeBlink = currentTime;
    }

}
