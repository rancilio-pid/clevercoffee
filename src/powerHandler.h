/**
 * @file powerHandler.h
 *
 * @brief Handler for digital power switch
 */
#pragma once

inline bool currStatePowerSwitchPressed = false;
inline bool lastPowerSwitchPressed = false;
inline unsigned long systemInitializedTime = 0;
inline unsigned long firstSwitchPressTime = 0;
inline bool trackingPressTime = false;

extern bool systemInitialized;

void performSafeShutdown();

inline void checkPowerSwitch() {
    if (!config.get<bool>("hardware.switches.power.enabled") || powerSwitch == nullptr) {
        return;
    }

    const bool powerSwitchPressed = powerSwitch->isPressed();
    const long currentMillis = millis();

    // Record when system was first initialized
    if (systemInitialized && systemInitializedTime == 0) {
        systemInitializedTime = currentMillis;
    }

    if (const int powerSwitchType = config.get<int>("hardware.switches.power.type"); powerSwitchType == Switch::TOGGLE) {
        if (powerSwitchPressed != lastPowerSwitchPressed) {
            lastPowerSwitchPressed = powerSwitchPressed;

            if (powerSwitchPressed) {
                if (machineState == kStandby || machineState == kPidDisabled) {
                    machineState = kPidNormal;
                    resetStandbyTimer(kPidNormal);
                    setRuntimePidState(true);
                    u8g2->setPowerSave(0);
                }
            }
            else {
                if (machineState != kStandby) {
                    performSafeShutdown();
                    machineState = kStandby;
                    standbyModeRemainingTimeMillis = 0;
                    standbyModeRemainingTimeDisplayOffMillis = 0;
                }
            }
        }
    }
    else if (powerSwitchType == Switch::MOMENTARY) {
        if (powerSwitchPressed != currStatePowerSwitchPressed) {
            currStatePowerSwitchPressed = powerSwitchPressed;

            if (currStatePowerSwitchPressed && systemInitialized) {
                // Only start tracking press time if system has been initialized for at least 5 seconds
                if (currentMillis - systemInitializedTime > 5000) {
                    firstSwitchPressTime = currentMillis;
                    trackingPressTime = true;
                }

                if (machineState == kStandby) {
                    machineState = kPidNormal;
                    resetStandbyTimer(kPidNormal);
                    setRuntimePidState(true);
                    u8g2->setPowerSave(0);
                }
                else {
                    performSafeShutdown();
                    machineState = kStandby;
                    standbyModeRemainingTimeMillis = 0;
                    standbyModeRemainingTimeDisplayOffMillis = 0;
                }
            }
            else if (!currStatePowerSwitchPressed) {
                // Switch released - stop tracking
                trackingPressTime = false;
                firstSwitchPressTime = 0;
            }
        }

        // Check for long press to trigger reboot (only for momentary switches)
        // Only reboot when:
        // 1. System is initialized
        // 2. At least 5 seconds have passed since initialization
        // 3. A press that started after initialization is actively tracked
        // 4. The press has lasted long enough for longPressDetected()
        if (powerSwitchPressed && systemInitialized && (currentMillis - systemInitializedTime > 5000) && trackingPressTime && (currentMillis - firstSwitchPressTime > 1000) && // Minimum 1 second actual press
            powerSwitch->longPressDetected()) {
            LOG(INFO, "Power switch long press detected - initiating system reboot");
            u8g2->setPowerSave(0);

            // Display reboot message
            displayWrappedMessage("REBOOTING\nPlease wait...");
            delay(1000);

            performSafeShutdown();

            LOG(INFO, "System reboot initiated");
            delay(500);

            ESP.restart();
        }
    }
}

/**
 * @brief Check if power switch allows operation (for brew/steam/hot water handlers)
 * @return true if operation is allowed, false otherwise
 */
inline bool isPowerSwitchOperationAllowed() {
    if (!config.get<bool>("hardware.switches.power.enabled") || powerSwitch == nullptr) {
        return true; // No power switch configured, allow operation
    }

    if (const int powerSwitchType = config.get<int>("hardware.switches.power.type"); powerSwitchType == Switch::TOGGLE) {
        return powerSwitch->isPressed();
    }
    else if (powerSwitchType == Switch::MOMENTARY) {
        // For momentary switches, check machine state instead of switch state
        return machineState != kStandby;
    }

    return true;
}
