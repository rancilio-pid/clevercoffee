/**
 * @file steamHandler.h
 *
 * @brief Handler for digital steam switch
 */
#pragma once

inline uint8_t currStateSteamSwitch;

inline void checkSteamSwitch() {
    if (!config.get<bool>("hardware.switches.steam.enabled") || steamSwitch == nullptr) {
        return;
    }

    if (!isPowerSwitchOperationAllowed()) {
        return;
    }

    const uint8_t steamSwitchReading = steamSwitch->isPressed();

    if (config.get<int>("hardware.switches.steam.type") == Switch::TOGGLE) {
        // Set steamON to 1 when steamswitch is HIGH
        if (steamSwitchReading == HIGH) {
            if (machineState != kStandby) {
                steamON = true;
            }
            else if (currStateSteamSwitch == LOW) {
                // only enable steam if we detected a transition from LOW to HIGH
                steamON = true;
            }
        }

        // if activated via web interface then steamFirstON == 1, prevent override
        if (steamSwitchReading == LOW && !steamFirstON) {
            steamON = false;
        }

        currStateSteamSwitch = steamSwitchReading;
    }
    else if (config.get<int>("hardware.switches.steam.type") == Switch::MOMENTARY) {
        if (steamSwitchReading != currStateSteamSwitch) {
            currStateSteamSwitch = steamSwitchReading;

            // only toggle heating power if the new button state is HIGH
            if (currStateSteamSwitch == HIGH) {
                if (steamON == 0) {
                    steamON = true;
                }
                else {
                    steamON = false;
                }
            }
        }
    }
}
