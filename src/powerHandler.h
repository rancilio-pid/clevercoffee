/**
 * @file powerHandler.h
 *
 * @brief Handler for digital power switch
 */
#pragma once

inline uint8_t currStatePowerSwitch; // the current reading from the input pin

inline void checkPowerSwitch() {
    if (config.getPowerSwitchEnabled()) {
        uint8_t powerSwitchReading = powerSwitch->isPressed();

        if (config.getPowerSwitchType() == Switch::TOGGLE) {
            // Set pidON to 1 when powerswitch is HIGH
            if ((powerSwitchReading == HIGH && machineState != kStandby) || (powerSwitchReading == LOW && machineState == kStandby)) {
                setRuntimePidState(true);
            }
            else {
                // Set pidON to 0 when powerswitch is not HIGH
                setRuntimePidState(false);
            }
        }
        else if (config.getPowerSwitchType() == Switch::MOMENTARY) {
            // if the button state has changed:
            if (powerSwitchReading != currStatePowerSwitch) {
                currStatePowerSwitch = powerSwitchReading;

                // only toggle heating power if the new button state is HIGH
                if (currStatePowerSwitch == HIGH) {
                    if (pidON == 0) {
                        setRuntimePidState(true);
                    }
                    else {
                        setRuntimePidState(false);
                    }
                }
            }
        }
    }
}
