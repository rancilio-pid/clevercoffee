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
BrewSwitchState currBrewSwitchState = kBrewSwitchIdle;
BrewState currBrewState = kBrewIdle;
ManualFlushState currManualFlushState = kManualFlushIdle;
BackflushState currBackflushState = kBackflushIdle;

uint8_t brewSwitchReading = LOW;
uint8_t currReadingBrewSwitch = LOW;
bool brewSwitchWasOff = false;

// Brew values
uint8_t featureBrewControl = FEATURE_BREW_CONTROL; // enables control of pumpe and valve
double targetBrewTime = TARGET_BREW_TIME;          // brew time in s
double preinfusion = PRE_INFUSION_TIME;            // preinfusion time in s
double preinfusionPause = PRE_INFUSION_PAUSE_TIME; // preinfusion pause time in s
double totalTargetBrewTime = 0;                    // total target brew time including preinfusion and preinfusion pause
double currBrewTime = 0;                           // current running total brewed time
unsigned long startingTime = 0;                    // start time of brew
bool brewPIDDisabled = false;                      // is PID disabled for delay after brew has started?

// Backflush values
int backflushCycles = BACKFLUSH_CYCLES;
double backflushFillTime = BACKFLUSH_FILL_TIME;
double backflushFlushTime = BACKFLUSH_FLUSH_TIME;
int backflushOn = 0;
int currBackflushCycles = 1;

// Shot timer with or without scale
#if FEATURE_SCALE == 1
boolean scaleCalibrationOn = 0;
boolean scaleTareOn = 0;
int shottimerCounter = 10;
float calibrationValue = SCALE_CALIBRATION_FACTOR; // use calibration example to get value
float currReadingWeight = 0;                       // value from HX711
float prewBrewWeight = 0;                          // value of scale before brew started
float currBrewWeight = 0;                          // weight value of current brew
float scaleDelayValue = 2.5;                       // value in gramm that takes still flows onto the scale after brew is stopped
bool scaleFailure = false;
const unsigned long intervalWeight = 200;          // weight scale
unsigned long previousMillisScale;                 // initialisation at the end of init()
HX711_ADC LoadCell(PIN_HXDAT, PIN_HXCLK);

#if SCALE_TYPE == 0
HX711_ADC LoadCell2(PIN_HXDAT2, PIN_HXCLK);
#endif
#endif

/**
 * @brief Toggle or momentary input for Brew Switch
 */
void checkbrewswitch() {
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
    else {
        loggedEmptyWaterTank = false;
    }

    // Convert toggle brew switch input to brew switch state
    if (BREWSWITCH_TYPE == Switch::TOGGLE) {
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
                else if ((currBrewState == kBrewFinished) || (currBackflushState == kBackflushFinished)) {
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
    else if (BREWSWITCH_TYPE == Switch::MOMENTARY) {
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
                else if ((currBrewState == kBrewFinished) || (currBackflushState == kBackflushFinished)) { // Brew reached target and stopped or blackflush cycle done
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
 * @brief Brew process handeling including timer and state machine for brew-by-time and brew-by-weight
 * @return true if brew is running, false otherwise
 */
bool brew() {
    unsigned long currentMillisTemp = millis();
    checkbrewswitch();

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
            totalTargetBrewTime = (preinfusion * 1000) + (preinfusionPause * 1000) + (targetBrewTime * 1000);
        }
        else {
            // Stop by time deactivated --> totalTargetBrewTime = 0
            totalTargetBrewTime = 0;
        }

        // state machine for brew
        switch (currBrewState) {
            case kBrewIdle:           // waiting step for brew switch turning on
                if (currBrewSwitchState == kBrewSwitchShortPressed && brewSwitchWasOff && backflushOn == 0 && machineState != kBackflush) {
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
                valveRelay.on();
                pumpRelay.on();

                if (currBrewTime > (preinfusion * 1000)) {
                    LOG(INFO, "Preinfusion pause running");
                    currBrewState = kPreinfusionPause;
                }

                break;

            case kPreinfusionPause:
                valveRelay.on();
                pumpRelay.off();

                if (currBrewTime > ((preinfusion + preinfusionPause) * 1000)) {
                    LOG(INFO, "Brew running");
                    currBrewState = kBrewRunning;
                }

                break;

            case kBrewRunning:
                valveRelay.on();
                pumpRelay.on();

                // stop brew if target-time is reached --> No stop if stop by time is deactivated via Parameter (0)
                if ((currBrewTime > totalTargetBrewTime) && ((targetBrewTime > 0))) {
                    LOG(INFO, "Brew reached time target");
                    currBrewState = kBrewFinished;
                }
#if (FEATURE_SCALE == 1)
                // stop brew if target-weight is reached --> No stop if stop by weight is deactivated via Parameter (0)
                else if (((FEATURE_SCALE == 1) && (currBrewWeight > targetBrewWeight)) && (targetBrewWeight > 0)) {
                    LOG(INFO, "Brew reached weight target");
                    currBrewState = kBrewFinished;
                }
#endif

                break;

            case kBrewFinished:
                valveRelay.off();
                pumpRelay.off();
                currentMillisTemp = 0;
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
                currentMillisTemp = 0;
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
    return (currBrewState != kBrewIdle && currBrewState != kBrewFinished);
}

/**
 * @brief manual grouphead flush
 * @return true if manual flush is running, false otherwise
 */
bool manualFlush() {
    unsigned long currentMillisTemp = millis();
    checkbrewswitch();
    if (currManualFlushState == kManualFlushRunning) {
        currBrewTime = currentMillisTemp - startingTime;
    }

    switch (currManualFlushState) {
        case kManualFlushIdle:
            if (currBrewSwitchState == kBrewSwitchLongPressed) {
                startingTime = millis();
                valveRelay.on();
                pumpRelay.on();
                LOG(INFO, "Manual flush started");
                currManualFlushState = kManualFlushRunning;
            }
            break;

        case kManualFlushRunning:
            if (currBrewSwitchState != kBrewSwitchLongPressed) {
                valveRelay.off();
                pumpRelay.off();
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
    return (currManualFlushState == kManualFlushRunning);
}

/**
 * @brief Backflush
 */
void backflush() {
    checkbrewswitch();
    if (currBackflushState != kBackflushIdle && backflushOn == 0) {
        currBackflushState = kBackflushFinished; // Force reset in case backflushOn is reset during backflush!
        LOG(INFO, "Backflush: Disabled via webinterface");
    }
    else if (offlineMode == 1 || currBrewState > kBrewIdle || backflushCycles <= 0 || backflushOn == 0) {
        return;
    }

    // abort function for state machine from every state
    if (currBrewSwitchState == kBrewSwitchIdle && currBackflushState > kBackflushIdle && currBackflushState < kBackflushFinished) {
        currBackflushState = kBackflushFinished;
        if (currBackflushState != kBackflushFinished) {
            LOG(INFO, "Backflush stopped manually");
        }
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
                valveRelay.on();
                pumpRelay.on();
                LOGF(INFO, "Start backflush cycle %d", currBackflushCycles);
                LOG(INFO, "Backflush: filling portafilter");
                currBackflushState = kBackflushFilling;
            }

            break;

        case kBackflushFilling:
            if (millis() - startingTime > (backflushFillTime * 1000)) {
                startingTime = millis();
                valveRelay.off();
                pumpRelay.off();
                LOG(INFO, "Backflush: flushing into drip tray");
                currBackflushState = kBackflushFlushing;
            }
            break;

        case kBackflushFlushing:
            if (millis() - startingTime > (backflushFlushTime * 1000)) {
                if (currBackflushCycles < backflushCycles) {
                    startingTime = millis();
                    valveRelay.on();
                    pumpRelay.on();
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
            valveRelay.off();
            pumpRelay.off();
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
