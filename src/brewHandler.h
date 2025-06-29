/**
 * @file brewHandler.h
 *
 * @brief Handler for brewing
 *
 */
// TODO:
//  show sections on website only if needed
//  add pressure to shot timer?
//  backflush also as bool, enable from website over diffrent var
//  SteamOn also as bool, rethink enable from website

#pragma once

enum BrewSwitchState {
    kBrewSwitchIdle = 10,
    kBrewSwitchPressed = 20,
    kBrewSwitchShortPressed = 30,
    kBrewSwitchLongPressed = 40,
    kBrewSwitchWaitForRelease = 50
};

enum BrewState {
    kBrewIdle = 10,
    kPreinfusion = 20,
    kPreinfusionPause = 30,
    kBrewRunning = 40,
    kBrewFinished = 50,
};

enum ManualFlushState {
    kManualFlushIdle = 10,
    kManualFlushRunning = 20,
};

enum BackflushState {
    kBackflushIdle = 10,
    kBackflushFilling = 20,
    kBackflushFlushing = 30,
    kBackflushFinished = 40
};

// Brew control states
inline BrewSwitchState currBrewSwitchState = kBrewSwitchIdle;
inline BrewState currBrewState = kBrewIdle;
inline ManualFlushState currManualFlushState = kManualFlushIdle;
inline BackflushState currBackflushState = kBackflushIdle;

inline uint8_t brewSwitchReading = LOW;
inline uint8_t currReadingBrewSwitch = LOW;
inline bool brewSwitchWasOff = false;

// Brew values
inline bool featureBrewControl = false;                   // enables control of pumpe and valve
inline double targetBrewTime = TARGET_BREW_TIME;          // brew time in s
inline double preinfusion = PRE_INFUSION_TIME;            // preinfusion time in s
inline double preinfusionPause = PRE_INFUSION_PAUSE_TIME; // preinfusion pause time in s
inline double totalTargetBrewTime = 0;                    // total target brew time including preinfusion and preinfusion pause
inline double currBrewTime = 0;                           // current running total brewed time
inline unsigned long startingTime = 0;                    // start time of brew
inline bool brewPIDDisabled = false;                      // is PID disabled for delay after brew has started?

// Backflush values
inline int backflushCycles = BACKFLUSH_CYCLES;
inline double backflushFillTime = BACKFLUSH_FILL_TIME;
inline double backflushFlushTime = BACKFLUSH_FLUSH_TIME;
inline bool backflushOn = false;
inline int currBackflushCycles = 1;

// Shot timer with or without scale
inline boolean scaleCalibrationOn = false;
inline boolean scaleTareOn = false;
inline int shottimerCounter = 10;
inline float currReadingWeight = 0; // value from HX711
inline float prewBrewWeight = 0;    // value of scale before brew started
inline float currBrewWeight = 0;    // weight value of current brew
inline float scaleDelayValue = 2.5; // value in gramm that takes still flows onto the scale after brew is stopped
inline bool scaleFailure = false;
inline HX711_ADC LoadCell(PIN_HXDAT, PIN_HXCLK);
inline HX711_ADC LoadCell2(PIN_HXDAT2, PIN_HXCLK);

/**
 * @brief True if in an intermediate brew state, false if idle or finished
 * if in an error state during brew return false
 */
bool checkBrewActive() {
    return ((currBrewState != kBrewIdle && currBrewState != kBrewFinished) && !(machineState >= kEmergencyStop));
}

/**
 * @brief True if in a machine state related to brew or flush, false if in other states
 */
bool checkBrewStates() {
    return (machineState == kBrew || machineState == kBackflush || machineState == kManualFlush);
}

/**
 * @brief turns off valve if not in an active brew state or if machineState changes away from one related to brewing or flushing
 */
inline void valveSafetyShutdownCheck() {
    if (!checkBrewActive() || !checkBrewStates()) {
        currBrewState == kBrewIdle; // reset state to idle if not in an active brew/flush state
        valveRelay->off();
    }
}

/**
 * @brief Toggle or momentary input for Brew Switch
 */
inline void checkBrewSwitch() {
    static bool loggedEmptyWaterTank = false;
    brewSwitchReading = brewSwitch->isPressed();

    // Block brewSwitch input when water tank is empty
    if (machineState == kWaterTankEmpty) {

        if (!loggedEmptyWaterTank && (currBrewSwitchState == kBrewSwitchIdle || currBrewSwitchState == kBrewSwitchPressed)) {
            LOG(WARNING, "Brew switch input ignored: Water tank empty");
            loggedEmptyWaterTank = true;
        }
        return;
    }

    loggedEmptyWaterTank = false;

    // Convert toggle brew switch input to brew switch state
    if (const int brewSwitchType = config.get<int>("hardware.switches.brew.type"); brewSwitchType == Switch::TOGGLE) {
        if (currReadingBrewSwitch != brewSwitchReading) {
            currReadingBrewSwitch = brewSwitchReading;
        }

        switch (currBrewSwitchState) {
            case kBrewSwitchIdle:
                if (currReadingBrewSwitch == HIGH) {
                    currBrewSwitchState = kBrewSwitchShortPressed;
                    LOG(DEBUG, "Toggle Brew switch is ON -> got to currBrewSwitchState = kBrewSwitchShortPressed");
                }
                break;

            case kBrewSwitchShortPressed:
                if (currReadingBrewSwitch == LOW) {
                    currBrewSwitchState = kBrewSwitchIdle;
                    LOG(DEBUG, "Toggle Brew switch is OFF -> got to currBrewSwitchState = kBrewSwitchIdle");
                }
                else if (currBrewState == kBrewFinished || currBackflushState == kBackflushFinished) {
                    currBrewSwitchState = kBrewSwitchWaitForRelease;
                    LOG(DEBUG, "Brew reached target or backflush done -> got to currBrewSwitchState = kBrewSwitchWaitForRelease");
                }
                break;

            case kBrewSwitchWaitForRelease:
                if (currReadingBrewSwitch == LOW) {
                    currBrewSwitchState = kBrewSwitchIdle;
                    LOG(DEBUG, "Brew switch reset -> got to currBrewSwitchState = kBrewSwitchIdle");
                }
                break;

            default:

                currBrewSwitchState = kBrewSwitchIdle;
                LOG(DEBUG, "Unexpected switch state -> currBrewSwitchState = kBrewSwitchIdle");
                break;
        }
    }
    // Convert momentary brew switch input to brew switch state
    else if (brewSwitchType == Switch::MOMENTARY) {
        if (currReadingBrewSwitch != brewSwitchReading) {
            currReadingBrewSwitch = brewSwitchReading;
        }

        switch (currBrewSwitchState) {
            case kBrewSwitchIdle:
                if (currReadingBrewSwitch == HIGH) {
                    currBrewSwitchState = kBrewSwitchPressed;
                    LOG(DEBUG, "Brew switch press detected -> got to currBrewSwitchState = kBrewSwitchPressed");
                }
                break;

            case kBrewSwitchPressed:                // Brew switch pressed - check for short or long press
                if (currReadingBrewSwitch == LOW) { // Brew switch short press detected
                    currBrewSwitchState = kBrewSwitchShortPressed;
                    LOG(DEBUG, "Brew switch short press detected -> got to currBrewSwitchState = kBrewSwitchShortPressed; start brew");
                }
                else if (currReadingBrewSwitch == HIGH && brewSwitch->longPressDetected()) { // Brew switch long press detected
                    currBrewSwitchState = kBrewSwitchLongPressed;
                    LOG(DEBUG, "Brew switch long press detected -> got to currBrewSwitchState = kBrewSwitchLongPressed; start manual flush");
                }
                break;

            case kBrewSwitchShortPressed:
                if (currReadingBrewSwitch == HIGH) { // Brew switch short press detected while brew is running - abort brew
                    currBrewSwitchState = kBrewSwitchWaitForRelease;
                    LOG(DEBUG, "Brew switch short press detected -> got to currBrewSwitchState = kBrewSwitchWaitForRelease; brew or backflush stopped manually");
                }
                else if (currBrewState == kBrewFinished || currBackflushState == kBackflushFinished) { // Brew reached target and stopped or blackflush cycle done
                    currBrewSwitchState = kBrewSwitchWaitForRelease;
                    LOG(DEBUG, "Brew reached target or backflush done -> got to currBrewSwitchState = kBrewSwitchWaitForRelease");
                }
                break;

            case kBrewSwitchLongPressed:
                if (currReadingBrewSwitch == LOW) { // Brew switch got released after long press detected - reset brewswitch
                    currBrewSwitchState = kBrewSwitchWaitForRelease;
                    LOG(DEBUG, "Brew switch long press released -> got to currBrewSwitchState = kBrewSwitchWaitForRelease; stop manual flush");
                }
                break;

            case kBrewSwitchWaitForRelease: // wait for brew switch got released
                if (currReadingBrewSwitch == LOW) {
                    currBrewSwitchState = kBrewSwitchIdle;
                    LOG(DEBUG, "Brew switch reset -> got to currBrewSwitchState = kBrewSwitchIdle");
                }
                break;

            default:
                currBrewSwitchState = kBrewSwitchIdle;
                LOG(DEBUG, "Unexpected switch state -> currBrewSwitchState = kBrewSwitchIdle");
                break;
        }
    }
}

/**
 * @brief If set to publish debug messages then list what the current action is and what triggered it
 * @return void
 */
void debugPumpState(String label, String state) {
    hotWaterStateDebug = state;
    IFLOG(DEBUG) {
        if (hotWaterStateDebug != lastHotWaterStateDebug) {
            LOGF(DEBUG, "Hot water state: %s - BrewHandler: %s", hotWaterStateDebug, label);
            lastHotWaterStateDebug = hotWaterStateDebug;
        }
    }
}

/**
 * @brief Brew process handeling including timer and state machine for brew-by-time and brew-by-weight
 * @return true if brew is running, false otherwise
 */
inline bool brew() {
    if (!config.get<bool>("hardware.switches.brew.enabled") || brewSwitch == nullptr) {
        return false; // brew switch is not enabled, so no brew process running
    }

    const unsigned long currentMillisTemp = millis();
    checkBrewSwitch();

    // abort function for state machine from every state
    if (currBrewSwitchState == kBrewSwitchIdle && currBrewState > kBrewIdle && currBrewState < kBrewFinished) {
        if (currBrewState != kBrewFinished) {
            LOG(INFO, "Brew stopped manually");
        }
        currBrewState = kBrewFinished;
    }
    // calculated brew time while brew is running
    if (currBrewState > kBrewIdle && currBrewState < kBrewFinished) {
        currBrewTime = currentMillisTemp - startingTime;
    }

    if (featureBrewControl) { // brew-by-time and brew-by-weight

        // check if brewswitch was turned off after a brew; Brew only runs once even brewswitch is still pressed
        if (currBrewSwitchState == kBrewSwitchIdle) {
            brewSwitchWasOff = true;
        }

        // set brew time every cycle, in case changes are done during brew
        if (targetBrewTime > 0) {
            totalTargetBrewTime = preinfusion * 1000 + preinfusionPause * 1000 + targetBrewTime * 1000;
        }
        else {
            // Stop by time deactivated --> totalTargetBrewTime = 0
            totalTargetBrewTime = 0;
        }

        // state machine for brew
        switch (currBrewState) {
            case kBrewIdle:           // waiting step for brew switch turning on
                if (currBrewSwitchState == kBrewSwitchShortPressed && brewSwitchWasOff && !backflushOn && machineState != kBackflush) {
                    startingTime = millis();
                    currBrewTime = 0; // reset currBrewTime, last brew is still stored
                    LOG(INFO, "Brew started");

                    if (preinfusionPause == 0 || preinfusion == 0) {
                        LOG(INFO, "Brew running");
                        currBrewState = kBrewRunning;
                    }
                    else {
                        LOG(INFO, "Preinfusion running");
                        currBrewState = kPreinfusion;
                    }
                }

                break;

            case kPreinfusion:
                valveRelay->on();
                pumpRelay->on();
                debugPumpState("Preinfusion", "on");

                if (currBrewTime > preinfusion * 1000) {
                    LOG(INFO, "Preinfusion pause running");
                    currBrewState = kPreinfusionPause;
                }

                break;

            case kPreinfusionPause:
                valveRelay->on();
                pumpRelay->off();
                debugPumpState("PreinfusionPause", "off");

                if (currBrewTime > (preinfusion + preinfusionPause) * 1000) {
                    LOG(INFO, "Brew running");
                    currBrewState = kBrewRunning;
                }

                break;

            case kBrewRunning:
                valveRelay->on();
                pumpRelay->on();
                debugPumpState("BrewRunning", "on");

                // stop brew if target-time is reached --> No stop if stop by time is deactivated via Parameter (0)
                if (currBrewTime > totalTargetBrewTime && targetBrewTime > 0) {
                    LOG(INFO, "Brew reached time target");
                    currBrewState = kBrewFinished;
                }

                // stop brew if target-weight is reached --> No stop if stop by weight is deactivated via Parameter (0)
                else if (config.get<bool>("hardware.sensors.scale.enabled") && currBrewWeight > targetBrewWeight && targetBrewWeight > 0) {
                    LOG(INFO, "Brew reached weight target");
                    currBrewState = kBrewFinished;
                }

                break;

            case kBrewFinished:
                valveRelay->off();
                pumpRelay->off();
                debugPumpState("BrewFinished", "off");
                brewSwitchWasOff = false;
                LOG(INFO, "Brew finished");
                LOGF(INFO, "Shot time: %4.1f s", currBrewTime / 1000);
                LOG(INFO, "Brew idle");
                currBrewState = kBrewIdle;

                break;

            default:
                currBrewState = kBrewIdle;
                LOG(DEBUG, "Unexpected brew state -> currBrewState = kBrewIdle");

                break;
        }
    }
    else {                            // brewControlOn == 0, only brew time
        switch (currBrewState) {
            case kBrewIdle:           // waiting step for brew switch turning on
                if (currBrewSwitchState == kBrewSwitchShortPressed) {
                    startingTime = millis();
                    currBrewTime = 0; // reset timeBrewed, last brew is still stored
                    LOG(INFO, "Brew timer started");
                    currBrewState = kBrewRunning;
                }

                break;

            case kBrewRunning:
                if (currBrewSwitchState == kBrewSwitchIdle && currBrewState == kBrewRunning) {
                    currBrewState = kBrewFinished;
                }

                break;

            case kBrewFinished:
                LOG(INFO, "Brew finished");
                LOGF(INFO, "Shot time: %4.1f s", currBrewTime / 1000);
                LOG(INFO, "Brew idle");
                currBrewState = kBrewIdle;

                break;

            default:
                currBrewState = kBrewIdle;
                LOG(DEBUG, "Unexpected brew state -> currBrewState = kBrewIdle");

                break;
        }
    }

    return checkBrewActive();
}

/**
 * @brief manual grouphead flush
 * @return true if manual flush is running, false otherwise
 */
inline bool manualFlush() {
    if (!config.get<bool>("hardware.switches.brew.enabled") || brewSwitch == nullptr) {
        return false; // brew switch is not enabled, so no brew process running
    }

    const unsigned long currentMillisTemp = millis();
    checkBrewSwitch();

    if (currManualFlushState == kManualFlushRunning) {
        currBrewTime = currentMillisTemp - startingTime;
    }

    switch (currManualFlushState) {
        case kManualFlushIdle:
            if (currBrewSwitchState == kBrewSwitchLongPressed) {
                startingTime = millis();
                valveRelay->on();
                pumpRelay->on();
                debugPumpState("ManualFlush", "on");
                LOG(INFO, "Manual flush started");
                currManualFlushState = kManualFlushRunning;
            }
            break;

        case kManualFlushRunning:
            if (currBrewSwitchState != kBrewSwitchLongPressed) {
                valveRelay->off();
                pumpRelay->off();
                debugPumpState("ManualFlush", "off");
                LOG(INFO, "Manual flush stopped");
                LOGF(INFO, "Manual flush time: %4.1f s", currBrewTime / 1000);
                currManualFlushState = kManualFlushIdle;
            }
            break;

        default:
            currManualFlushState = kManualFlushIdle;
            LOG(DEBUG, "Unexpected manual flush state -> currManualFlushState = kManualFlushIdle");

            break;
    }

    return currManualFlushState == kManualFlushRunning;
}

/**
 * @brief Backflush
 */
inline void backflush() {
    if (!config.get<bool>("hardware.switches.brew.enabled") || brewSwitch == nullptr) {
        return; // brew switch is not enabled, so no brew process running
    }

    checkBrewSwitch();

    if (currBackflushState != kBackflushIdle && !backflushOn) {
        currBackflushState = kBackflushFinished; // Force reset in case backflushOn is reset during backflush!
        LOG(INFO, "Backflush: Disabled via webinterface");
    }
    else if (offlineMode || currBrewState > kBrewIdle || backflushCycles <= 0 || !backflushOn) {
        return;
    }

    // abort function for state machine from every state
    if (currBrewSwitchState == kBrewSwitchIdle && currBackflushState > kBackflushIdle && currBackflushState < kBackflushFinished) {
        currBackflushState = kBackflushFinished;

        LOG(INFO, "Backflush stopped manually");
    }

    // check if brewswitch was turned off after a backflush; Backflush only runs once even brewswitch is still pressed
    if (currBrewSwitchState == kBrewSwitchIdle) {
        brewSwitchWasOff = true;
    }

    // State machine for backflush
    switch (currBackflushState) {
        case kBackflushIdle:
            if (currBrewSwitchState == kBrewSwitchShortPressed && backflushOn && brewSwitchWasOff) {
                startingTime = millis();
                valveRelay->on();
                pumpRelay->on();
                debugPumpState("Backflush", "on");
                LOGF(INFO, "Start backflush cycle %d", currBackflushCycles);
                LOG(INFO, "Backflush: filling portafilter");
                currBackflushState = kBackflushFilling;
            }

            break;

        case kBackflushFilling:
            if (millis() - startingTime > backflushFillTime * 1000) {
                startingTime = millis();
                valveRelay->off();
                pumpRelay->off();
                debugPumpState("Backflush", "off");
                LOG(INFO, "Backflush: flushing into drip tray");
                currBackflushState = kBackflushFlushing;
            }
            break;

        case kBackflushFlushing:
            if (millis() - startingTime > backflushFlushTime * 1000) {
                if (currBackflushCycles < backflushCycles) {
                    startingTime = millis();
                    valveRelay->on();
                    pumpRelay->on();
                    debugPumpState("Backflush", "on");
                    currBackflushCycles++;
                    LOGF(INFO, "Backflush: next backflush cycle %d", currBackflushCycles);
                    LOG(INFO, "Backflush: filling portafilter");
                    currBackflushState = kBackflushFilling;
                }
                else {
                    currBackflushState = kBackflushFinished;
                }
            }
            break;

        case kBackflushFinished:
            valveRelay->off();
            pumpRelay->off();
            debugPumpState("Backflush", "off");
            LOGF(INFO, "Backflush finished after %d cycles", currBackflushCycles);
            currBackflushCycles = 1;
            brewSwitchWasOff = false;
            currBackflushState = kBackflushIdle;

            break;

        default:
            currBackflushState = kBackflushIdle;
            LOG(DEBUG, "Unexpected backflush state -> currBackflushState = kBackflushIdle");

            break;
    }
}
