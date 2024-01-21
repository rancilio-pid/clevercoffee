/**
 * @file brewvoid.h
 *
 * @brief TODO
 *
 */

#pragma once

#include <pinmapping.h>

enum BrewState {
    kBrewIdle = 10,
    kPreinfusion = 20,
    kWaitPreinfusion = 21,
    kPreinfusionPause = 30,
    kWaitPreinfusionPause = 31,
    kBrewRunning = 40,
    kWaitBrew = 41,
    kBrewFinished = 42,
    kWaitBrewOff = 43
};

enum BackflushState {
    kBackflushWaitBrewswitchOn = 10,
    kBackflushFillingStart = 20,
    kBackflushFilling = 21,
    kBackflushFlushingStart = 30,
    kBackflushFlushing = 31,
    kBackflushWaitBrewswitchOff = 43
};

// Normal Brew
BrewState brewCounter = kBrewIdle;
int brewSwitch = 0;
int brewSwitchTrigger = LOW;
int lastStateBrewTrigger;                        // the last valid reading from the input pin (debounced)
unsigned long lastDebounceTimeBrewTrigger = 0;  // the last time the output pin was toggled
unsigned long debounceDelayBrewTrigger = 20;    // >20ms when switches are "flicked"
unsigned long brewSwitchTriggerMillis = 0;
const unsigned long brewTriggerLongPress = 500;     // time in ms until brew trigger will be interpreted as manual brewing
int brewSwitchTriggerCase = 10;
boolean brewSwitchWasOff = false;
double totalBrewTime = 0;                           // total brewtime set in software
double timeBrewed = 0;                              // total brewed time
double lastBrewTimeMillis = 0;                      // for shottimer delay after disarmed button
double lastBrewTime = 0 ;
unsigned long startingTime = 0;                     // start time of brew
boolean brewPIDDisabled = false;                    // is PID disabled for delay after brew has started?

// Shot timer with or without scale
#if (ONLYPIDSCALE == 1 || BREWMODE == 2)
    int shottimerCounter = 10 ;
    float calibrationValue = SCALE_CALIBRATION_FACTOR;  // use calibration example to get value
    float weight = 0;                                   // value from HX711
    float weightPreBrew = 0;                            // value of scale before wrew started
    float weightBrew = 0;                               // weight value of brew
    float scaleDelayValue = 2.5;                        // value in gramm that takes still flows onto the scale after brew is stopped
    bool scaleFailure = false;
    const unsigned long intervalWeight = 200;           // weight scale
    unsigned long previousMillisScale;                  // initialisation at the end of init()
    HX711_ADC LoadCell(PIN_HXDAT, PIN_HXCLK);
#endif

/**
 * @brief Switch or trigger input for BREW SWITCH
 */
void checkbrewswitch() {
    #if BREWSWITCHTYPE == 1
            // Digital 
            brewSwitch = digitalRead(PIN_BREWSWITCH);
    #endif

    #if BREWSWITCHTYPE == 2
            int reading = digitalRead(PIN_BREWSWITCH);

            if (reading != lastStateBrewTrigger) {
                // restart the debouncing timer
                lastDebounceTimeBrewTrigger = millis();
                // set new button state 
            }
            else if ((millis() - lastDebounceTimeBrewTrigger) > debounceDelayBrewTrigger) {
                // whatever the reading is at, it's been there for longer than the debounce
                // delay, so take it as the actual current state:
                
                if (brewSwitchTrigger != reading) {
                    brewSwitchTrigger = reading;
                }
            }
            lastStateBrewTrigger = reading;

        // Convert trigger signal to brew switch state
        switch (brewSwitchTriggerCase) {
            case 10:
                if (brewSwitchTrigger == HIGH) {
                    brewSwitchTriggerMillis = millis();
                    brewSwitchTriggerCase = 20;
                    debugPrintln("brewSwitchTriggerCase 10: HIGH");
                }
                break;

            case 20:
                // Only one push, brew
                if (brewSwitchTrigger == LOW) {
                    // Brew trigger
                    brewSwitch = HIGH;
                    brewSwitchTriggerCase = 30;
                    debugPrintln("brewSwitchTriggerCase 20: Brew Trigger HIGH");
                }

                // Button more than brewTriggerLongPress pushed
                if (brewSwitchTrigger == HIGH && (brewSwitchTriggerMillis + brewTriggerLongPress  <= millis())) {
                    debugPrintln("brewSwitchTriggerCase 20: Manual Trigger - flushing");
                    brewSwitchTriggerCase = 31;
                    digitalWrite(PIN_VALVE, relayOn);
                    digitalWrite(PIN_PUMP, relayOn);
                }
                break;

            case 30:
                // Stop brew trigger (one push) brewswitch == HIGH
                if ((brewSwitchTrigger == HIGH && brewSwitch == HIGH) || (machineState == kShotTimerAfterBrew) ) {
                    brewSwitch = LOW;
                    brewSwitchTriggerCase = 40;
                    debugPrintln("brewSwitchTriggerCase 30: Brew Trigger LOW");
                }
                break;

            case 31:
                // Stop Manual brewing, button goes low:
                if (brewSwitchTrigger == LOW && brewSwitch == LOW) {
                    brewSwitchTriggerCase = 40;
                    debugPrintln("brewswitchTriggerCase 31: Manual Trigger - brewing stop");
                    digitalWrite(PIN_VALVE, relayOff);
                    digitalWrite(PIN_PUMP, relayOff);
                }
            break;

            case 40:
                // Once Toggle-Button is released, go back to start and wait for brew button press
                if (brewSwitchTrigger == LOW) {
                    brewSwitchTriggerCase = 10;
                    debugPrintln("brewSwitchTriggerCase 40: Brew Trigger Next Loop");
                }
                // Once Toggle-Button is released, go back to start and wait for brew button press
                if (brewSwitchTrigger == LOW) {
                    brewSwitchTriggerCase = 10;
                    debugPrintln("brewSwitchTriggerCase 40: Brew Trigger Next Loop");
                }
                break;

            case 50:
                break;
        }
    #endif
}

/**
 * @brief Backflush
 */
void backflush() {
    if (backflushState != kBackflushWaitBrewswitchOn && backflushOn == 0) {
        backflushState = kBackflushWaitBrewswitchOff;  // Force reset in case backflushOn is reset during backflush!
    }
    else if (offlineMode == 1 || brewCounter > kBrewIdle || maxflushCycles <= 0 || backflushOn == 0) {
        return;
    }

    if (pidMode == 1) {  // Deactivate PID
        pidMode = 0;
        bPID.SetMode(pidMode);
        pidOutput = 0;
    }

    digitalWrite(PIN_HEATER, LOW);  // Stop heating

    checkbrewswitch();

    if (brewSwitch == LOW && backflushState != kBackflushWaitBrewswitchOn) {  // Abort function for state machine from every state
        backflushState = kBackflushWaitBrewswitchOff;
    }

    // State machine for backflush
    switch (backflushState) {
        case kBackflushWaitBrewswitchOn:
            if (brewSwitch == HIGH && backflushOn) {
                startingTime = millis();
                backflushState = kBackflushFillingStart;
            }

            break;

        case kBackflushFillingStart:
            debugPrintln("Backflush: Portafilter filling...");
            digitalWrite(PIN_VALVE, relayOn);
            digitalWrite(PIN_PUMP, relayOn);
            backflushState = kBackflushFilling;

            break;

        case kBackflushFilling:
            if (millis() - startingTime > FILLTIME) {
                startingTime = millis();
                backflushState = kBackflushFlushingStart;
            }

            break;

        case kBackflushFlushingStart:
            debugPrintln("Backflush: Flushing to drip tray...");
            digitalWrite(PIN_VALVE, relayOff);
            digitalWrite(PIN_PUMP, relayOff);
            flushCycles++;
            backflushState = kBackflushFlushing;

            break;

        case kBackflushFlushing:
            if (millis() - startingTime > flushTime && flushCycles < maxflushCycles) {
                startingTime = millis();
                backflushState = kBackflushFillingStart;
            }
            else if (flushCycles >= maxflushCycles) {
                backflushState = kBackflushWaitBrewswitchOff;
            }

            break;

        case kBackflushWaitBrewswitchOff:
            if (brewSwitch == LOW) {
                debugPrintln("Backflush: Finished!");
                digitalWrite(PIN_VALVE, relayOff);
                digitalWrite(PIN_PUMP, relayOff);
                flushCycles = 0;
                backflushState = kBackflushWaitBrewswitchOn;
            }

            break;
    }
}

#if (BREWMODE == 1)  // normal brew mode (based on time)
/**
 * @brief PreInfusion, Brew Normal
 */
void brew() {
    if (OnlyPID == 0) {
        unsigned long currentMillisTemp = millis();
        checkbrewswitch();

        if (brewSwitch == LOW && brewCounter > kBrewIdle) {
            // abort function for state machine from every state
            debugPrintln("Brew stopped manually");
            brewCounter = kWaitBrewOff;
        }

        if (brewCounter > kBrewIdle && brewCounter < kWaitBrewOff) {
            timeBrewed = currentMillisTemp - startingTime;
        }

        if (brewSwitch == LOW && movingAverageInitialized) {
            // check if brewswitch was turned off at least once, last time,
            brewSwitchWasOff = true;
        }

        totalBrewTime = (preinfusion * 1000) + (preinfusionPause * 1000) +
            (brewTime * 1000);  // running every cycle, in case changes are done during brew

        // state machine for brew
        switch (brewCounter) {
            case kBrewIdle: // waiting step for brew switch turning on
                if (brewSwitch == HIGH && backflushState == 10 && backflushOn == 0 && brewSwitchWasOff) {
                    startingTime = millis();

                    if (preinfusionPause == 0 || preinfusion == 0) {
                        brewCounter = kBrewRunning;
                    } else {
                        brewCounter = kPreinfusion;
                    }

                    coldstart = false;  // force reset coldstart if shot is pulled
                } else {
                    backflush();
                }

                break;

            case kPreinfusion:  // preinfusioon
                debugPrintln("Preinfusion");
                digitalWrite(PIN_VALVE, relayOn);
                digitalWrite(PIN_PUMP, relayOn);
                brewCounter = kWaitPreinfusion;

                break;

            case kWaitPreinfusion:  // waiting time preinfusion
                if (timeBrewed > (preinfusion * 1000)) {
                    brewCounter = kPreinfusionPause;
                }

                break;

            case kPreinfusionPause:  // preinfusion pause
                debugPrintln("Preinfusion pause");
                digitalWrite(PIN_VALVE, relayOn);
                digitalWrite(PIN_PUMP, relayOff);
                brewCounter = kWaitPreinfusionPause;

                break;

            case kWaitPreinfusionPause:  // waiting time preinfusion pause
                if (timeBrewed > ((preinfusion * 1000) + (preinfusionPause * 1000))) {
                    brewCounter = kBrewRunning;
                }

                break;

            case kBrewRunning:  // brew running
                debugPrintln("Brew started");
                digitalWrite(PIN_VALVE, relayOn);
                digitalWrite(PIN_PUMP, relayOn);
                brewCounter = kWaitBrew;

                break;

            case kWaitBrew:  // waiting time brew
                lastBrewTime = timeBrewed;

                if (timeBrewed > totalBrewTime) {
                    brewCounter = kBrewFinished;
                }

                break;

            case kBrewFinished:  // brew finished
                debugPrintln("Brew stopped");
                digitalWrite(PIN_VALVE, relayOff);
                digitalWrite(PIN_PUMP, relayOff);
                brewCounter = kWaitBrewOff;
                timeBrewed = 0;

                break;

            case kWaitBrewOff:  // waiting for brewswitch off position
                if (brewSwitch == LOW) {
                    digitalWrite(PIN_VALVE, relayOff);
                    digitalWrite(PIN_PUMP, relayOff);

                    // disarmed button
                    currentMillisTemp = 0;
                    brewDetected = 0;  // rearm brewDetection
                    brewCounter = kBrewIdle;
                    timeBrewed = 0;
                }

                break;
        }
    }
}
#endif

#if (BREWMODE == 2)
/**
 * @brief Weight based brew mode
 */
void brew() {
    if (OnlyPID == 0) {
        checkbrewswitch();
        unsigned long currentMillisTemp = millis();

        if (brewSwitch == LOW && brewCounter > kBrewIdle) {
            // abort function for state machine from every state
            brewCounter = kWaitBrewOff;
        }

        if (brewCounter > kBrewIdle && brewCounter < kWaitBrewOff) {
            timeBrewed = currentMillisTemp - startingTime;
            weightBrew = weight - weightPreBrew;
        }

        if (brewSwitch == LOW && movingAverageInitialized) {
            // check if brewswitch was turned off at least once, last time,
            brewSwitchWasOff = true;
        }

        totalBrewTime = ((preinfusion * 1000) + (preinfusionPause * 1000) +
            (brewTime * 1000));  // running every cycle, in case changes are done during brew

        // state machine for brew
        switch (brewCounter) {
            case 10:  // waiting step for brew switch turning on
                if (brewSwitch == HIGH && backflushState == 10 && backflushOn == 0 && brewSwitchWasOff) {
                    startingTime = millis();
                    brewCounter = kPreinfusion;

                    if (preinfusionPause == 0 || preinfusion == 0) {
                        brewCounter = kBrewRunning;
                    }

                    coldstart = false;  // force reset coldstart if shot is pulled
                    weightPreBrew = weight;
                } else {
                    backflush();
                }

                break;

            case 20:  // preinfusioon
                debugPrintln("Preinfusion");
                digitalWrite(PIN_VALVE, relayOn);
                digitalWrite(PIN_PUMP, relayOn);
                brewCounter = kWaitPreinfusion;

                break;

            case 21:  // waiting time preinfusion
                if (timeBrewed > (preinfusion * 1000)) {
                    brewCounter = kPreinfusionPause;
                }

                break;

            case 30:  // preinfusion pause
                debugPrintln("preinfusion pause");
                digitalWrite(PIN_VALVE, relayOn);
                digitalWrite(PIN_PUMP, relayOff);
                brewCounter = kWaitPreinfusionPause;

                break;

            case 31:  // waiting time preinfusion pause
                if (timeBrewed > ((preinfusion * 1000) + (preinfusionPause * 1000))) {
                    brewCounter = kBrewRunning;
                }

                break;

            case 40:  // brew running
                debugPrintln("Brew started");
                digitalWrite(PIN_VALVE, relayOn);
                digitalWrite(PIN_PUMP, relayOn);
                brewCounter = kWaitBrew;

                break;

            case 41:  // waiting time brew
                if (timeBrewed > totalBrewTime || (weightBrew > (weightSetpoint - scaleDelayValue))) {
                    brewCounter = kBrewFinished;
                }

                if (timeBrewed > totalBrewTime) {
                    brewCounter = kBrewFinished;
                }

                break;

            case 42:  // brew finished
                debugPrintln("Brew stopped");
                digitalWrite(PIN_VALVE, relayOff);
                digitalWrite(PIN_PUMP, relayOff);
                brewCounter = kWaitBrewOff;

                break;

            case 43:  // waiting for brewswitch off position
                if (brewSwitch == LOW) {
                    digitalWrite(PIN_VALVE, relayOff);
                    digitalWrite(PIN_PUMP, relayOff);

                    // disarmed button
                    currentMillisTemp = 0;
                    timeBrewed = 0;
                    brewDetected = 0;  // rearm brewDetection
                    brewCounter = kBrewIdle;
                }

                weightBrew = weight - weightPreBrew;  // always calculate weight to show on display

                break;
        }
    }
}
#endif
