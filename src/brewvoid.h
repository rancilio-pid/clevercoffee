/**
 * @file brewvoid.h
 *
 * @brief TODO
 *
 */

#pragma once


/**
 * @brief Digtalswitch OR Read analog input pin for BREW SWITCH
 */
void checkbrewswitch() {
    #if BREWSWITCHTYPE == 1
        #if (PIN_BREWSWITCH > 0)
            // Digital GIPO
            brewswitch = digitalRead(PIN_BREWSWITCH);
        #endif
    #endif

    #if BREWSWITCHTYPE == 2
        #if (PIN_BREWSWITCH > 0)
            int reading = digitalRead(PIN_BREWSWITCH);

            if (reading != lastButtonStateBrew) {
                // restart the debouncing timer
                lastDebounceTimeBrewTrigger = millis();
                // set new button state 
                lastButtonStateBrew = reading;
            }
            else if ((millis() - lastDebounceTimeBrewTrigger) > debounceDelayBrewTrigger) {
                // whatever the reading is at, it's been there for longer than the debounce
                // delay, so take it as the actual current state:
                
                if (brewswitchTrigger != reading)
                {
                    brewswitchTrigger = reading;
                }
            }
        #endif

        // Convert trigger signal to brew switch state
        switch (brewswitchTriggerCase) {
            case 10:
                if (brewswitchTrigger == HIGH) {
                    brewswitchTriggermillis = millis();
                    brewswitchTriggerCase = 20;
                    debugPrintln("brewswitchTriggerCase 10: HIGH");
                }
                break;

            case 20:
                // Only one push, brew
                if (brewswitchTrigger == LOW) {
                    // Brew trigger
                    brewswitch = HIGH;
                    brewswitchTriggerCase = 30;
                    debugPrintln("brewswitchTriggerCase 20: Brew Trigger HIGH");
                }

                // Button more than one 1sec pushed
                if (brewswitchTrigger == HIGH && (brewswitchTriggermillis + 1000 <= millis())) {
                    debugPrintln("brewswitchTriggerCase 20: Manual Trigger - brewing");
                    brewswitchTriggerCase = 31;
                    digitalWrite(PIN_VALVE, relayON);
                    digitalWrite(PIN_PUMP, relayON);
                }
                break;
            case 30:
                // Stop brew trigger (one push) brewswitch == HIGH
                if ((brewswitchTrigger == HIGH && brewswitch == HIGH) || (machineState == kShotTimerAfterBrew) ) {
                    brewswitch = LOW;
                    brewswitchTriggerCase = 40;
                    debugPrintln("brewswitchTriggerCase 30: Brew Trigger LOW");
                }
                break;
            case 31:
                // Stop Manual brewing, button goes low:
                if (brewswitchTrigger == LOW && brewswitch == LOW) {
                    brewswitchTriggerCase = 40;
                    debugPrintln("brewswitchTriggerCase 31: Manual Trigger - brewing stop");
                    digitalWrite(PIN_VALVE, relayOFF);
                    digitalWrite(PIN_PUMP, relayOFF);
                }
            break;

            case 40:
                // Go back to start and wait for brew button press
                brewswitchTriggerCase = 10;
                debugPrintln("brewswitchTriggerCase 40: Brew Trigger Next Loop");
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
    if (backflushState != 10 && backflushON == 0) {
        backflushState = 43;  // Force reset in case backflushON is reset during backflush!
    } else if (offlineMode == 1 || brewcounter > kBrewIdle || maxflushCycles <= 0 || backflushON == 0) {
        return;
    }

    if (pidMode == 1) {  // Deactivate PID
        pidMode = 0;
        bPID.SetMode(pidMode);
        pidOutput = 0;
    }

    digitalWrite(PIN_HEATER, LOW);  // Stop heating

    checkbrewswitch();

    if (brewswitch == LOW && backflushState > 10) {  // Abort function for state machine from every state
        backflushState = 43;
    }

    // State machine for brew
    switch (backflushState) {
        case 10:  // waiting step for brew switch turning on
            if (brewswitch == HIGH && backflushON) {
                startingTime = millis();
                backflushState = 20;
            }

            break;

        case 20:  // portafilter filling
            debugPrintln("portafilter filling");
            digitalWrite(PIN_VALVE, relayON);
            digitalWrite(PIN_PUMP, relayON);
            backflushState = 21;

            break;

        case 21:  // waiting time for portafilter filling
            if (millis() - startingTime > FILLTIME) {
                startingTime = millis();
                backflushState = 30;
            }

            break;

        case 30:  // flushing
            debugPrintln("flushing");
            digitalWrite(PIN_VALVE, relayOFF);
            digitalWrite(PIN_PUMP, relayOFF);
            flushCycles++;
            backflushState = 31;

            break;

        case 31:  // waiting time for flushing
            if (millis() - startingTime > flushTime && flushCycles < maxflushCycles) {
                startingTime = millis();
                backflushState = 20;
            } else if (flushCycles >= maxflushCycles) {
                backflushState = 43;
            }

            break;

        case 43:  // waiting for brewswitch off position
            if (brewswitch == LOW) {
                debugPrintln("backflush finished");
                digitalWrite(PIN_VALVE, relayOFF);
                digitalWrite(PIN_PUMP, relayOFF);
                flushCycles = 0;
                backflushState = 10;
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
        unsigned long currentMillistemp = millis();
        checkbrewswitch();

        if (brewswitch == LOW && brewcounter > kBrewIdle) {
            // abort function for state machine from every state
            debugPrintln("Brew stopped manually");
            brewcounter = kWaitBrewOff;
        }

        if (brewcounter > kBrewIdle && brewcounter < kWaitBrewOff) {
            timeBrewed = currentMillistemp - startingTime;
        }

        if (brewswitch == LOW && movingAverageInitialized) {
            // check if brewswitch was turned off at least once, last time,
            brewswitchWasOFF = true;
        }

        totalBrewTime = (preinfusion * 1000) + (preinfusionpause * 1000) +
            (brewtime * 1000);  // running every cycle, in case changes are done during brew

        // state machine for brew
        switch (brewcounter) {
            case kBrewIdle: // waiting step for brew switch turning on
                if (brewswitch == HIGH && backflushState == 10 && backflushON == 0 && brewswitchWasOFF) {
                    startingTime = millis();

                    if (preinfusionpause == 0 || preinfusion == 0) {
                        brewcounter = kBrewRunning;
                    } else {
                        brewcounter = kPreinfusion;
                    }

                    coldstart = false;  // force reset coldstart if shot is pulled
                } else {
                    backflush();
                }

                break;

            case kPreinfusion:  // preinfusioon
                debugPrintln("Preinfusion");
                digitalWrite(PIN_VALVE, relayON);
                digitalWrite(PIN_PUMP, relayON);
                brewcounter = kWaitPreinfusion;

                break;

            case kWaitPreinfusion:  // waiting time preinfusion
                if (timeBrewed > (preinfusion * 1000)) {
                    brewcounter = kPreinfusionPause;
                }

                break;

            case kPreinfusionPause:  // preinfusion pause
                debugPrintln("Preinfusion pause");
                digitalWrite(PIN_VALVE, relayON);
                digitalWrite(PIN_PUMP, relayOFF);
                brewcounter = kWaitPreinfusionPause;

                break;

            case kWaitPreinfusionPause:  // waiting time preinfusion pause
                if (timeBrewed > ((preinfusion * 1000) + (preinfusionpause * 1000))) {
                    brewcounter = kBrewRunning;
                }

                break;

            case kBrewRunning:  // brew running
                debugPrintln("Brew started");
                digitalWrite(PIN_VALVE, relayON);
                digitalWrite(PIN_PUMP, relayON);
                brewcounter = kWaitBrew;

                break;

            case kWaitBrew:  // waiting time brew
                lastbrewTime = timeBrewed;

                if (timeBrewed > totalBrewTime) {
                    brewcounter = kBrewFinished;
                }

                break;

            case kBrewFinished:  // brew finished
                debugPrintln("Brew stopped");
                digitalWrite(PIN_VALVE, relayOFF);
                digitalWrite(PIN_PUMP, relayOFF);
                brewcounter = kWaitBrewOff;
                timeBrewed = 0;

                break;

            case kWaitBrewOff:  // waiting for brewswitch off position
                if (brewswitch == LOW) {
                    digitalWrite(PIN_VALVE, relayOFF);
                    digitalWrite(PIN_PUMP, relayOFF);

                    // disarmed button
                    currentMillistemp = 0;
                    brewDetected = 0;  // rearm brewDetection
                    brewcounter = kBrewIdle;
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
        unsigned long currentMillistemp = millis();

        if (brewswitch == LOW && brewcounter > kBrewIdle) {
            // abort function for state machine from every state
            brewcounter = kWaitBrewOff;
        }

        if (brewcounter > kBrewIdle && brewcounter < kWaitBrewOff) {
            timeBrewed = currentMillistemp - startingTime;
            weightBrew = weight - weightPreBrew;
        }

        if (brewswitch == LOW && movingAverageInitialized) {
            // check if brewswitch was turned off at least once, last time,
            brewswitchWasOFF = true;
        }

        totalBrewTime = ((preinfusion * 1000) + (preinfusionpause * 1000) +
            (brewtime * 1000));  // running every cycle, in case changes are done during brew

        // state machine for brew
        switch (brewcounter) {
            case 10:  // waiting step for brew switch turning on
                if (brewswitch == HIGH && backflushState == 10 && backflushON == 0 && brewswitchWasOFF) {
                    startingTime = millis();
                    brewcounter = kPreinfusion;

                    if (preinfusionpause == 0 || preinfusion == 0) {
                        brewcounter = kBrewRunning;
                    }

                    coldstart = false;  // force reset coldstart if shot is pulled
                    weightPreBrew = weight;
                } else {
                    backflush();
                }

                break;

            case 20:  // preinfusioon
                debugPrintln("Preinfusion");
                digitalWrite(PIN_VALVE, relayON);
                digitalWrite(PIN_PUMP, relayON);
                brewcounter = kWaitPreinfusion;

                break;

            case 21:  // waiting time preinfusion
                if (timeBrewed > (preinfusion * 1000)) {
                    brewcounter = kPreinfusionPause;
                }

                break;

            case 30:  // preinfusion pause
                debugPrintln("preinfusion pause");
                digitalWrite(PIN_VALVE, relayON);
                digitalWrite(PIN_PUMP, relayOFF);
                brewcounter = kWaitPreinfusionPause;

                break;

            case 31:  // waiting time preinfusion pause
                if (timeBrewed > ((preinfusion * 1000) + (preinfusionpause * 1000))) {
                    brewcounter = kBrewRunning;
                }

                break;

            case 40:  // brew running
                debugPrintln("Brew started");
                digitalWrite(PIN_VALVE, relayON);
                digitalWrite(PIN_PUMP, relayON);
                brewcounter = kWaitBrew;

                break;

            case 41:  // waiting time brew
                if (timeBrewed > totalBrewTime || (weightBrew > (weightSetpoint - scaleDelayValue))) {
                    brewcounter = kBrewFinished;
                }

                if (timeBrewed > totalBrewTime) {
                    brewcounter = kBrewFinished;
                }

                break;

            case 42:  // brew finished
                debugPrintln("Brew stopped");
                digitalWrite(PIN_VALVE, relayOFF);
                digitalWrite(PIN_PUMP, relayOFF);
                brewcounter = kWaitBrewOff;

                break;

            case 43:  // waiting for brewswitch off position
                if (brewswitch == LOW) {
                    digitalWrite(PIN_VALVE, relayOFF);
                    digitalWrite(PIN_PUMP, relayOFF);

                    // disarmed button
                    currentMillistemp = 0;
                    timeBrewed = 0;
                    brewDetected = 0;  // rearm brewDetection
                    brewcounter = kBrewIdle;
                }

                weightBrew = weight - weightPreBrew;  // always calculate weight to show on display

                break;
        }
    }
}
#endif
