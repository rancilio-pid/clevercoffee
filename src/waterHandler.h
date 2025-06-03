/**
 * @file waterHandler.h
 *
 * @brief Handler for digital hot water switch
 */

uint8_t currStateWaterSwitch;

MachineState lastMachineStateDebug = kInit;

void checkWaterSwitch() {
    if (FEATURE_WATERSWITCH) {
        uint8_t waterSwitchReading = waterSwitch->isPressed();

        if (WATERSWITCH_TYPE == Switch::TOGGLE) {
            // Set waterON to 1 when waterswitch is HIGH
            if (waterSwitchReading == HIGH) {
                waterON = 1;
            }

            // Set waterON to 0 when waterswitch is LOW
            if (waterSwitchReading == LOW) {
                waterON = 0;
            }
        }
        else if (WATERSWITCH_TYPE == Switch::MOMENTARY) {
            if (waterSwitchReading != currStateWaterSwitch) {
                currStateWaterSwitch = waterSwitchReading;

                // only toggle water power if the new button state is HIGH
                if (currStateWaterSwitch == HIGH) {
                    if (waterON == 0) {
                        waterON = 1;
                    }
                    else {
                        waterON = 0;
                    }
                }
            }
        }
    }
}

/**
 * @brief Control the pump based on machine state
 * @return void
 */
void waterHandler(void) {
    // turn on pump if water switch is on, only turn off if not in a brew or flush state
    if (machineState == kWaterTankEmpty) {
        pumpRelay.off();
        waterStateDebug = "off-we"; // turn off due to water empty
    }
    else if (machineState == kWater || (machineState == kSteam && waterON == 1)) {
        pumpRelay.on();
        waterStateDebug = "on-sw";                                                                 // turned on due to switch input
    }
    else {
        if (machineState != kBrew && machineState != kBackflush && machineState != kManualFlush) { // switch turned off and not in brew or flush
            pumpRelay.off();
            waterStateDebug = "off-sw";
        }
        else {
            if (waterStateDebug != "on" && waterStateDebug != "off") { // on or off states from brewhandler
                waterStateDebug = "brew or flush";                     // label it brew or flush if brewhandler doesnt add labels
            }
        }
    }

    IFLOG(DEBUG) {
        if (machineState != lastMachineStateDebug || waterStateDebug != lastWaterStateDebug) {
            LOGF(DEBUG, "Water state: %s, MachineState=%s", waterStateDebug, machinestateEnumToString(machineState));
            lastMachineStateDebug = machineState;
            lastWaterStateDebug = waterStateDebug;
        }
    }
}