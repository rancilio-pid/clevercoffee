/**
 * @file brewHandler.h
 *
 * @brief Handler for brewing
 *
 */
// TODO:
//  clean up backflush
//  Flush Timer configurable and seperated from shottimer?
//  move brewPIDDisabled to kBrew? new fuction to set PID State based on mashine state switching? if kBrew -> disable PID/wait/BDPID or NORMALPID
//  check all Scale stuff
//  check params website
//  check MQTT/HASSIO for all brew stuff
//  show heating logo if steam temp isn´t reached?
//  how handle brew, backflush, manualflush, hotwater if mashine is in steam mode

#pragma once

#include <hardware/pinmapping.h>

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
    kWaitPreinfusion = 21,
    kPreinfusionPause = 30,
    kWaitPreinfusionPause = 31,
    kBrewRunning = 40,
    kWaitBrew = 41,
    kBrewFinished = 42,
};

enum ManualFlushState {
    kManualFlushIdle = 10,
    kManualFlushRunning = 20,
};

enum BackflushState {
    kBackflushWaitBrewswitchOn = 10,
    kBackflushFillingStart = 20,
    kBackflushFilling = 21,
    kBackflushFlushingStart = 30,
    kBackflushFlushing = 31,
    kBackflushWaitBrewswitchOff = 43
};

// Brew control states
BrewSwitchState currBrewSwitchState = kBrewSwitchIdle;
BrewState currBrewState = kBrewIdle;
ManualFlushState currManualFlushState = kManualFlushIdle;

uint8_t brewSwitchReading = LOW;
uint8_t currReadingBrewSwitch = LOW;
boolean brewSwitchWasOff = false;

int brewOn = 0;                  // flag is set if brew was detected
int manualFlushOn = 0;           // flag is set if manual flush is detected
double totalBrewTime = 0;        // total brewtime set in software
double timeBrewed = 0;           // total brewed time
double lastBrewTimeMillis = 0;   // for shottimer delay after brew is finished
unsigned long startingTime = 0;  // start time of brew
boolean brewPIDDisabled = false; // is PID disabled for delay after brew has started?

// Shot timer with or without scale
#if FEATURE_SCALE == 1
boolean scaleCalibrationOn = 0;
boolean scaleTareOn = 0;
int shottimerCounter = 10;
float calibrationValue = SCALE_CALIBRATION_FACTOR; // use calibration example to get value
float weight = 0;                                  // value from HX711
float weightPreBrew = 0;                           // value of scale before wrew started
float weightBrew = 0;                              // weight value of brew
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
    brewSwitchReading = brewSwitch->isPressed();

    if (BREWSWITCH_TYPE == Switch::TOGGLE) {
        if (brewSwitchReading == HIGH && currBrewSwitchState != kBrewSwitchShortPressed) {
            currBrewSwitchState = kBrewSwitchShortPressed;
            LOG(DEBUG, "Toggle Brew switch is ON -> got to currBrewSwitchState = kBrewSwitchShortPressed");
        }
        else if (brewSwitchReading == LOW && currBrewSwitchState != kBrewSwitchIdle) {
            currBrewSwitchState = kBrewSwitchIdle;
            LOG(DEBUG, "Toggle Brew switch is OFF -> got to currBrewSwitchState = kBrewSwitchIdle");
        }
    }
    else if (BREWSWITCH_TYPE == Switch::MOMENTARY) {
        if (currReadingBrewSwitch != brewSwitchReading) {
            currReadingBrewSwitch = brewSwitchReading;
        }

        // Convert momentary brew switch input to brew switch state
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
                    LOG(DEBUG, "Brew switch short press detected -> got to currBrewSwitchState = kBrewSwitchWaitForRelease; brew stopped manually");
                }
                else if ((currBrewState == kBrewFinished) || (backflushState == kBackflushWaitBrewswitchOff)) { // Brew reached target and stopped or blackflush cycle done
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
        }
    }
}

/**
 * @brief Backflush
 */
void backflush() {
    if (backflushState != kBackflushWaitBrewswitchOn && backflushOn == 0) {
        backflushState = kBackflushWaitBrewswitchOff; // Force reset in case backflushOn is reset during backflush!
        LOG(INFO, "Backflush: Disabled via Webinterface");
    }
    else if (offlineMode == 1 || currBrewState > kBrewIdle || backflushCycles <= 0 || backflushOn == 0) {
        return;
    }

    if (bPID.GetMode() == 1) { // Deactivate PID
        bPID.SetMode(0);
        pidOutput = 0;
    }

    heaterRelay.off(); // Stop heating

    checkbrewswitch();

    if (currStateBrewSwitch == LOW && backflushState != kBackflushWaitBrewswitchOn) { // Abort function for state machine from every state
        backflushState = kBackflushWaitBrewswitchOff;
    }

    // State machine for backflush
    switch (backflushState) {
        case kBackflushWaitBrewswitchOn:
            if (currStateBrewSwitch == HIGH && backflushOn) {
                startingTime = millis();
                backflushState = kBackflushFillingStart;
            }

            break;

        case kBackflushFillingStart:
            LOG(INFO, "Backflush: Portafilter filling...");
            valveRelay.on();
            pumpRelay.on();
            backflushState = kBackflushFilling;

            break;

        case kBackflushFilling:
            if (millis() - startingTime > (backflushFillTime * 1000)) {
                startingTime = millis();
                backflushState = kBackflushFlushingStart;
            }

            break;

        case kBackflushFlushingStart:
            LOG(INFO, "Backflush: Flushing to drip tray...");
            valveRelay.off();
            pumpRelay.off();
            currBackflushCycles++;
            backflushState = kBackflushFlushing;

            break;

        case kBackflushFlushing:
            if (millis() - startingTime > (backflushFlushTime * 1000) && currBackflushCycles < backflushCycles) {
                startingTime = millis();
                backflushState = kBackflushFillingStart;
            }
            else if (currBackflushCycles >= backflushCycles) {
                backflushState = kBackflushWaitBrewswitchOff;
            }

            break;

        case kBackflushWaitBrewswitchOff:
            if (currStateBrewSwitch == LOW) {
                LOG(INFO, "Backflush: Finished!");
                valveRelay.off();
                pumpRelay.off();
                currBackflushCycles = 0;
                backflushState = kBackflushWaitBrewswitchOn;
            }

            break;
    }
}

#if (FEATURE_BREWCONTROL == 0)
/**
 * @brief Brew timer
 */
void brewTimer() {
    unsigned long currentMillisTemp = millis();
    checkbrewswitch();

    // Start the timer when the brew switch is turned on
    if (currBrewSwitchState == kBrewSwitchShortPressed && currBrewState == kBrewIdle) {
        brewOn = 1;
        startingTime = currentMillisTemp;
        timeBrewed = 0; // reset timeBrewed, last brew is still stored
        LOG(INFO, "Brew timer started");
        currBrewState = kBrewRunning;
    }

    // Update the brewed time if the brew switch is still on
    if (currBrewSwitchState == kBrewSwitchShortPressed && currBrewState == kBrewRunning) {
        timeBrewed = currentMillisTemp - startingTime;
    }

    // Stop the timer when the brew switch is turned off
    if (currBrewSwitchState == kBrewSwitchIdle && currBrewState == kBrewRunning) {
        brewOn = 0;
        lastBrewTimeMillis = millis(); // time brew finished for shottimer delay
        LOG(INFO, "Brew timer stopped");
        LOGF(INFO, "Shot time: %4.1f s", timeBrewed / 1000);
        currBrewState = kBrewIdle;
    }
}
#endif

#if (FEATURE_BREWCONTROL == 1)
/**
 * @brief Backflush
 */
void backflush() {
    if (backflushState != kBackflushWaitBrewswitchOn && backflushOn == 0) {
        backflushState = kBackflushWaitBrewswitchOff; // Force reset in case backflushOn is reset during backflush!
        LOG(INFO, "Backflush: Disabled via Webinterface");
    }
    else if (offlineMode == 1 || currBrewState > kBrewIdle || backflushCycles <= 0 || backflushOn == 0) {
        return;
    }

    if (bPID.GetMode() == 1) { // Deactivate PID
        bPID.SetMode(0);
        pidOutput = 0;
    }

    heaterRelay.off(); // Stop heating

    checkbrewswitch();

    if (currBrewSwitchState == kBrewSwitchIdle && backflushState != kBackflushWaitBrewswitchOn) { // Abort function for state machine from every state
        backflushState = kBackflushWaitBrewswitchOff;
    }

    // State machine for backflush
    switch (backflushState) {
        case kBackflushWaitBrewswitchOn:
            if (currBrewSwitchState == kBrewSwitchShortPressed && backflushOn && machineState != kWaterEmpty) {
                startingTime = millis();
                backflushState = kBackflushFillingStart;
            }

            break;

        case kBackflushFillingStart:
            LOG(INFO, "Backflush: Portafilter filling...");
            valveRelay.on();
            pumpRelay.on();
            backflushState = kBackflushFilling;

            break;

        case kBackflushFilling:
            if (millis() - startingTime > (backflushFillTime * 1000)) {
                startingTime = millis();
                backflushState = kBackflushFlushingStart;
            }

            break;

        case kBackflushFlushingStart:
            LOG(INFO, "Backflush: Flushing to drip tray...");
            valveRelay.off();
            pumpRelay.off();
            currBackflushCycles++;
            backflushState = kBackflushFlushing;

            break;

        case kBackflushFlushing:
            if (millis() - startingTime > (backflushFlushTime * 1000) && currBackflushCycles < backflushCycles) {
                startingTime = millis();
                backflushState = kBackflushFillingStart;
            }
            else if (currBackflushCycles >= backflushCycles) {
                backflushState = kBackflushWaitBrewswitchOff;
            }

            break;

        case kBackflushWaitBrewswitchOff:
            if (currBrewSwitchState == kBrewSwitchIdle) {
                LOG(INFO, "Backflush: Finished!");
                valveRelay.off();
                pumpRelay.off();
                currBackflushCycles = 0;
                backflushState = kBackflushWaitBrewswitchOn;
            }

            break;
    }
}

/**
 * @brief Time or weight based brew mode
 */
void brew() {
    unsigned long currentMillisTemp = millis();
    checkbrewswitch();

    if (currBrewSwitchState == kBrewSwitchIdle && currBrewState > kBrewIdle && currBrewState < kBrewFinished) {
        // abort function for state machine from every state
        LOG(INFO, "Brew stopped manually");
        currBrewState = kBrewFinished;
    }

    if (currBrewSwitchState == kBrewSwitchIdle) {
        // check if brewswitch was turned off at least once, last time,
        brewSwitchWasOff = true;
    }

    if (currBrewState > kBrewIdle && currBrewState < kBrewFinished) {
        timeBrewed = currentMillisTemp - startingTime;
    }

    if (brewTime > 0) {
        totalBrewTime = (preinfusion * 1000) + (preinfusionPause * 1000) + (brewTime * 1000); // running every cycle, in case changes are done during brew
    }
    else {
        // Stop by time deactivated --> brewTime = 0
        totalBrewTime = 0;
    }

    // state machine for brew
    switch (currBrewState) {
        case kBrewIdle: // waiting step for brew switch turning on
            if (currBrewSwitchState == kBrewSwitchShortPressed && backflushState == 10 && backflushOn == 0 && brewSwitchWasOff && machineState != kWaterEmpty) {
                startingTime = millis();
                timeBrewed = 0;
                LOG(INFO, "Brew started");

                if (preinfusionPause == 0 || preinfusion == 0) {
                    brewOn = 1;
                    currBrewState = kBrewRunning;
                }
                else {
                    brewOn = 1;
                    currBrewState = kPreinfusion;
                }
            }
            else {
                backflush();
            }

            break;

        case kPreinfusion: // preinfusioon
            valveRelay.on();
            pumpRelay.on();
            LOG(INFO, "Preinfusion");
            currBrewState = kWaitPreinfusion;

            break;

        case kWaitPreinfusion: // waiting time preinfusion
            if (timeBrewed > (preinfusion * 1000)) {
                currBrewState = kPreinfusionPause;
            }

            break;

        case kPreinfusionPause: // preinfusion pause
            valveRelay.on();
            pumpRelay.off();
            LOG(INFO, "Preinfusion pause");
            currBrewState = kWaitPreinfusionPause;

            break;

        case kWaitPreinfusionPause: // waiting time preinfusion pause
            if (timeBrewed > ((preinfusion * 1000) + (preinfusionPause * 1000))) {
                currBrewState = kBrewRunning;
            }

            break;

        case kBrewRunning: // brew running
            valveRelay.on();
            pumpRelay.on();
            LOG(INFO, "Brew running");
            currBrewState = kWaitBrew;

            break;

        case kWaitBrew: // waiting time or weight brew

            // stop brew if target-time is reached --> No stop if stop by time is deactivated via Parameter (0)
            if ((timeBrewed > totalBrewTime) && ((brewTime > 0))) {
                LOG(INFO, "Brew reached time target");
                currBrewState = kBrewFinished;
            }
#if (FEATURE_SCALE == 1)
            // stop brew if target-weight is reached --> No stop if stop by weight is deactivated via Parameter (0)
            else if (((FEATURE_SCALE == 1) && (weightBrew > weightSetpoint)) && (weightSetpoint > 0)) {
                LOG(INFO, "Brew reached weight target");
                currBrewState = kBrewFinished;
            }
#endif

            break;

        case kBrewFinished: // brew finished
            valveRelay.off();
            pumpRelay.off();
            brewOn = 0;
            currentMillisTemp = 0;
            lastBrewTimeMillis = millis(); // time brew finished for shottimer delay
            brewSwitchWasOff = false;
            LOG(INFO, "Brew finished");
            LOGF(INFO, "Shot time: %4.1f s", timeBrewed / 1000);
            currBrewState = kBrewIdle;

            break;
    }
}

/**
 * @brief manual grouphead flush
 */
void manualFlush() {
    unsigned long currentMillisTemp = millis();
    checkbrewswitch();
    if (currManualFlushState == kManualFlushRunning) {
        timeBrewed = currentMillisTemp - startingTime;
    }

    switch (currManualFlushState) {
        case kManualFlushIdle:
            if (currBrewSwitchState == kBrewSwitchLongPressed && machineState != kWaterEmpty) {
                startingTime = millis();
                valveRelay.on();
                pumpRelay.on();
                manualFlushOn = 1;
                LOG(INFO, "Manual flush started");
                currManualFlushState = kManualFlushRunning;
            }
            break;

        case kManualFlushRunning:
            if (currBrewSwitchState != kBrewSwitchLongPressed) {
                valveRelay.off();
                pumpRelay.off();
                manualFlushOn = 0;
                LOG(INFO, "Manual flush stopped");
                LOGF(INFO, "Manual flush time: %4.1f s", timeBrewed / 1000);
                currManualFlushState = kManualFlushIdle;
            }
            break;
    }
}

#endif
