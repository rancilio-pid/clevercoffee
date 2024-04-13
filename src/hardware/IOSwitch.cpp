/**
 * @file IOSwitch.h
 *
 * @brief A physical switch connected to a GPIO Pin
 */

#include "IOSwitch.h"
#include "GPIOPin.h"

IOSwitch::IOSwitch(int pinNumber, GPIOPin::Type pinType, Type switchType, Mode mode) :
    Switch(switchType, mode), gpio(pinNumber, pinType) {
}

bool IOSwitch::isPressed() {
    uint8_t reading = gpio.read();
    unsigned long currentMillis = millis();

    if (reading != lastState) {
        lastDebounceTime = currentMillis;
    }

    if ((currentMillis - lastDebounceTime) > debounceDelay) {
        if ((reading ^ mode_) != currentState) {
            currentState = reading ^ mode_;

            if (currentState == LOW) {
                lastStateChangeTime = currentMillis;
            }
        }
    }

    lastState = reading;

    if (lastState != currentState) {
        pressStartTime = millis();
    }

    if (type_ == MOMENTARY) {
        if (currentState == HIGH && (pressStartTime + longPressDuration) <= currentMillis) {
            longPressTriggered = true;
        }
        else if (currentState == LOW && lastStateChangeTime == currentMillis) {
            longPressTriggered = false;
        }
    }

    return currentState == HIGH;
}

bool IOSwitch::longPressDetected() {
    if (type_ == TOGGLE) {
        return false;
    }
    else if (type_ == MOMENTARY) {
        return longPressTriggered;
    }

    return false;
}
