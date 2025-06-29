/**
 * @file hotWaterHandler.h
 *
 * @brief Handler for digital hot water switch
 */

uint8_t currStateHotWaterSwitch;

MachineState lastMachineStateHotWaterDebug = kInit;

/**
 * @brief True if in a hot water state or in Steam with hot water on, false otherwise
 * if in an error state it will change away from these machine states and return false
 */
bool checkHotWaterActive() {
    return (machineState == kHotWater || (machineState == kSteam && hotWaterON));
}

/**
 * @brief If set to publish debug messages then list what the current action is and what triggered it
 * @return void
 */
void debugHotWaterState(String label, String state) {
    hotWaterStateDebug = state;
    IFLOG(DEBUG) {
        if (machineState != lastMachineStateHotWaterDebug || hotWaterStateDebug != lastHotWaterStateDebug) {
            LOGF(DEBUG, "Hot water state: %s, MachineState=%s", state, label);
            lastMachineStateHotWaterDebug = machineState;
            lastHotWaterStateDebug = hotWaterStateDebug;
        }
    }
}

void checkHotWaterSwitch() {
    uint8_t hotWaterSwitchReading = hotWaterSwitch->isPressed();

    if (config.get<int>("hardware.switches.hot_water.type") == Switch::TOGGLE) {
        // Set hotWaterON to 1 when hot water switch is HIGH
        if (hotWaterSwitchReading == HIGH) {
            hotWaterON = 1;
        }

        // Set waterON to 0 when waterswitch is LOW
        if (hotWaterSwitchReading == LOW) {
            hotWaterON = 0;
        }
    }
    else if (config.get<int>("hardware.switches.hot_water.type") == Switch::MOMENTARY) {
        if (hotWaterSwitchReading != currStateHotWaterSwitch) {
            currStateHotWaterSwitch = hotWaterSwitchReading;

            // only toggle water power if the new button state is HIGH
            if (currStateHotWaterSwitch == HIGH) {
                if (hotWaterON == 0) {
                    hotWaterON = 1;
                }
                else {
                    hotWaterON = 0;
                }
            }
        }

        // override to zero and set switch off if in PID Disabled mode
        if (machineState == kPidDisabled) {
            currStateHotWaterSwitch == LOW;
            hotWaterON = 0;
        }
    }
}

/**
 * @brief Control the pump based on machine state
 * @return pumps state
 */
inline bool hotWaterHandler(void) {
    if (!config.get<bool>("hardware.switches.hot_water.enabled") || hotWaterSwitch == nullptr) {
        return false; // hot water switch is not enabled
    }

    String machineStateString = machinestateEnumToString(machineState);
    const unsigned long currentMillisTemp = millis();
    checkHotWaterSwitch();


    // turn on pump if water switch is on, only turn off if not in a brew or flush state
    if (machineState == kWaterTankEmpty) {
        pumpRelay->off();
        hotWaterStateDebug = "off-we"; // turn off due to water empty
        debugHotWaterState(machineStateString, hotWaterStateDebug);
        return false;
    }
    else if (machineState >= kStandby) {
        pumpRelay->off();
        hotWaterStateDebug = "off-standby"; // turn off due to entering standby mode
        debugHotWaterState(machineStateString, hotWaterStateDebug);
        return false;
    }
    else if (machineState == kEmergencyStop || machineState == kSensorError || machineState == kEepromError) {
        pumpRelay->off();
        hotWaterStateDebug = "off-error"; // turn off due to error
        debugHotWaterState(machineStateString, hotWaterStateDebug);
        return false;
    }
    else if (checkHotWaterActive()) {
        pumpRelay->on();
        hotWaterStateDebug = "on-sw"; // turned on due to switch input
        debugHotWaterState(machineStateString, hotWaterStateDebug);
        return true;
    }
    else {
        if (!checkBrewStates()) {  // switch turned off and not in brew or flush
            pumpRelay->off();
            hotWaterStateDebug = "off-sw";
            debugHotWaterState(machineStateString, hotWaterStateDebug);
            return false;
        }
        else {
            if (hotWaterStateDebug != "on" && hotWaterStateDebug != "off") { // on or off states from brewhandler
                hotWaterStateDebug = "brew or flush";                     // label it brew or flush if brewhandler doesnt add labels
                debugHotWaterState(machineStateString, hotWaterStateDebug);
            }
        }
    }
    return false;
}