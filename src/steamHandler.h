/**
 * @file steamHandler.h
 *
 * @brief Handler for digital steam switch
 */
#pragma once

inline uint8_t currStateSteamSwitch;

inline void checkSteamSwitch() {
    if (config.getSteamSwitchEnabled()) {
        const uint8_t steamSwitchReading = steamSwitch->isPressed();

        if (config.getSteamSwitchType() == Switch::TOGGLE) {
            // Set steamON to 1 when steamswitch is HIGH
            if (steamSwitchReading == HIGH) {
                steamON = 1;
            }

            // if activated via web interface then steamFirstON == 1, prevent override
            if (steamSwitchReading == LOW && !steamFirstON) {
                steamON = 0;
            }
        }
        else if (config.getSteamSwitchType() == Switch::MOMENTARY) {
            if (steamSwitchReading != currStateSteamSwitch) {
                currStateSteamSwitch = steamSwitchReading;

                // only toggle heating power if the new button state is HIGH
                if (currStateSteamSwitch == HIGH) {
                    if (steamON == 0) {
                        steamON = 1;
                    }
                    else {
                        steamON = 0;
                    }
                }
            }
        }
    }
}
