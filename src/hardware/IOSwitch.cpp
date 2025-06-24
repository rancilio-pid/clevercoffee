/**
 * @file IOSwitch.h
 *
 * @brief A physical switch connected to a GPIO Pin
 */

#include "IOSwitch.h"
#include "GPIOPin.h"

#include "Logger.h"
IOSwitch::IOSwitch(const int pinNumber, const GPIOPin::Type pinType, const Type switchType, const Mode mode, const uint8_t initialState) :
    Switch(switchType, mode), gpio(pinNumber, pinType), lastState(initialState), currentState(initialState) {
}

bool IOSwitch::isPressed() {
    const uint8_t reading = gpio.read();
    const unsigned long currentMillis = millis();

    if (reading != lastState) {
        lastDebounceTime = currentMillis;
    }

    if (currentMillis - lastDebounceTime > debounceDelay) {
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
        if (currentState == HIGH && pressStartTime + longPressDuration <= currentMillis) {
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

    if (type_ == MOMENTARY) {
        return longPressTriggered;
    }

    return false;
}
