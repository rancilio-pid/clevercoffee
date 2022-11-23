/**
 * @file brewvoid.h
 *
 * @brief
 */

/**
 * @brief Digtalswitch OR Read analog input pin for BREW SWITCH
 */
void checkbrewswitch() {
    #if BREWSWITCHTYPE == 1
        #if (PINBREWSWITCH > 0)
            // Digital GIPO
            brewswitch = digitalRead(PINBREWSWITCH);
        #endif

        // Digital Analog
        #if (PINBREWSWITCH == 0)
            unsigned long currentMillistemp = millis();

            if (currentMillistemp - previousMillistempanalogreading >= analogreadingtimeinterval) {
                previousMillistempanalogreading = currentMillistemp;

                if (filterPressureValue(analogRead(analogPin)) > 1000) {
                    brewswitch = HIGH;
                }

                if (filterPressureValue(analogRead(analogPin)) < 1000) {
                    brewswitch = LOW;
                }
            }
        #endif
    #endif

    #if BREWSWITCHTYPE == 2  // TRIGGER
        #if (PINBREWSWITCH > 0)
            int reading = digitalRead(PINBREWSWITCH);

            if (reading != brewswitchTrigger) {
                // reset the debouncing timer
                lastDebounceTimeBrewTrigger = millis();
            }

            if ((millis() - lastDebounceTimeBrewTrigger) > debounceDelayBrewTrigger) {
                // whatever the reading is at, it's been there for longer than the debounce
                // delay, so take it as the actual current state:

                // if the button state has changed:
                if (reading != buttonStateBrewTrigger) {
                    buttonStateBrewTrigger = reading;
                }
            }

            brewswitchTrigger = reading;
           // debugPrintln(brewswitchTrigger);
        #endif

        // Digital Analog
        #if (PINBREWSWITCH == 0)
            unsigned long currentMillistemp = millis();

            if (currentMillistemp - previousMillistempanalogreading >= analogreadingtimeinterval) {
                previousMillistempanalogreading = currentMillistemp;

                if (filter(analogRead(analogPin)) > 1000) {
                    brewswitchTrigger = HIGH;
                }

                if (filter(analogRead(analogPin)) < 1000) {
                    brewswitchTrigger = LOW;
                }
            }
        #endif

        // Triggersignal umsetzen in brewswitch
        switch (brewswitchTriggerCase) {
            case 10:
                if (brewswitchTrigger == HIGH) {
                    brewswitchTriggermillis = millis();
                    brewswitchTriggerCase = 20;
                    debugPrintln("brewswitchTriggerCase 10: HIGH");
                }
                break;

            case 20:
                // only one push, brew
                if (brewswitchTrigger == LOW) {
                    // Brew trigger
                    brewswitch = HIGH;
                    brewswitchTriggerCase = 30;
                    debugPrintln("brewswitchTriggerCase 20: Brew Trigger HIGH");
                }

                // Button more than one 1sec pushed
                if (brewswitchTrigger == HIGH && (brewswitchTriggermillis + 1000 <= millis())) {
                    // DO something
                    debugPrintln("brewswitchTriggerCase 20: Manual Trigger - brewing");
                    brewswitchTriggerCase = 31;
                    digitalWrite(PINVALVE, relayON);
                    digitalWrite(PINPUMP, relayON);
                }
                break;
            case 30:
                // Stop Brew trigger (one push) brewswitch == HIGH
                if ((brewswitchTrigger == HIGH && brewswitch == HIGH) || (machineState == kShotTimerAfterBrew) ) {
                    brewswitch = LOW;
                    brewswitchTriggerCase = 40;
                    brewswitchTriggermillis = millis();
                    debugPrintln("brewswitchTriggerCase 30: Brew Trigger LOW");
                }
                break;
            case 31:
                // Stop Manual brewing, button goes low:
                if (brewswitchTrigger == LOW && brewswitch == LOW) {
                    brewswitchTriggerCase = 40;
                    brewswitchTriggermillis = millis();
                    debugPrintln("brewswitchTriggerCase 31: Manual Trigger - brewing stop");
                    digitalWrite(PINVALVE, relayOFF);
                    digitalWrite(PINPUMP, relayOFF);
                }
            break;

            case 40:
                // wait 5 Sec until next brew, detection
                if (brewswitchTriggermillis + 5000 <= millis()) {
                    brewswitchTriggerCase = 10;
                    debugPrintln("brewswitchTriggerCase 40: Brew Trigger Next Loop");
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
    if (backflushState != 10 && backflushON == 0) {
        backflushState = 43;  // force reset in case backflushON is reset during backflush!
    } else if (offlineMode == 1 || brewcounter > 10 || maxflushCycles <= 0 || backflushON == 0) {
        return;
    }

    if (pidMode == 1) {  // Deactivate PID
        pidMode = 0;
        bPID.SetMode(pidMode);
        pidOutput = 0;
    }

    digitalWrite(PINHEATER, LOW);  // Stop heating

    checkbrewswitch();

    if (brewswitch == LOW && backflushState > 10) {  // abort function for state machine from every state
        backflushState = 43;
    }

    // state machine for brew
    switch (backflushState) {
        case 10:  // waiting step for brew switch turning on
            if (brewswitch == HIGH && backflushON) {
                startingTime = millis();
                backflushState = 20;
            }

            break;

        case 20:  // portafilter filling
            debugPrintln("portafilter filling");
            digitalWrite(PINVALVE, relayON);
            digitalWrite(PINPUMP, relayON);
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
            digitalWrite(PINVALVE, relayOFF);
            digitalWrite(PINPUMP, relayOFF);
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
                digitalWrite(PINVALVE, relayOFF);
                digitalWrite(PINPUMP, relayOFF);
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

        if (brewswitch == LOW && brewcounter > 10) {
            // abort function for state machine from every state
            debugPrintln("Brew stopped manually");
            brewcounter = 43;
        }

        if (brewcounter > 10 && brewcounter < 43) {
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
            case 10:
                // waiting step for brew switch turning on
                if (brewswitch == HIGH && backflushState == 10 && backflushON == 0 && brewswitchWasOFF) {
                    startingTime = millis();

                    if (preinfusionpause == 0 || preinfusion == 0) {
                        brewcounter = 40;
                    } else {
                        brewcounter = 20;
                    }

                    coldstart = false;  // force reset coldstart if shot is pulled
                } else {
                    backflush();
                }

                break;

            case 20:  // preinfusioon
                debugPrintln("Preinfusion");
                digitalWrite(PINVALVE, relayON);
                digitalWrite(PINPUMP, relayON);
                brewcounter = 21;

                break;

            case 21:  // waiting time preinfusion
                if (timeBrewed > (preinfusion * 1000)) {
                    brewcounter = 30;
                }

                break;

            case 30:  // preinfusion pause
                debugPrintln("Preinfusion pause");
                digitalWrite(PINVALVE, relayON);
                digitalWrite(PINPUMP, relayOFF);
                brewcounter = 31;

                break;

            case 31:  // waiting time preinfusion pause
                if (timeBrewed > ((preinfusion * 1000) + (preinfusionpause * 1000))) {
                    brewcounter = 40;
                }

                break;

            case 40:  // brew running
                debugPrintln("Brew started");
                digitalWrite(PINVALVE, relayON);
                digitalWrite(PINPUMP, relayON);
                brewcounter = 41;

                break;

            case 41:  // waiting time brew
                lastbrewTime = timeBrewed;

                if (timeBrewed > totalBrewTime) {
                    brewcounter = 42;
                }

                break;

            case 42:  // brew finished
                debugPrintln("Brew stopped");
                digitalWrite(PINVALVE, relayOFF);
                digitalWrite(PINPUMP, relayOFF);
                brewcounter = 43;
                timeBrewed = 0;

                break;

            case 43:  // waiting for brewswitch off position
                if (brewswitch == LOW) {
                    digitalWrite(PINVALVE, relayOFF);
                    digitalWrite(PINPUMP, relayOFF);

                    // disarmed button
                    currentMillistemp = 0;
                    brewDetected = 0;  // rearm brewDetection
                    brewcounter = 10;
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

        if (brewswitch == LOW && brewcounter > 10) {
            // abort function for state machine from every state
            brewcounter = 43;
        }

        if (brewcounter > 10 && brewcounter < 43) {
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
                    brewcounter = 20;

                    if (preinfusionpause == 0 || preinfusion == 0) {
                        brewcounter = 40;
                    }

                    coldstart = false;  // force reset coldstart if shot is pulled
                    weightPreBrew = weight;
                } else {
                    backflush();
                }

                break;

            case 20:  // preinfusioon
                debugPrintln("Preinfusion");
                digitalWrite(PINVALVE, relayON);
                digitalWrite(PINPUMP, relayON);
                brewcounter = 21;

                break;

            case 21:  // waiting time preinfusion
                if (timeBrewed > (preinfusion * 1000)) {
                    brewcounter = 30;
                }

                break;

            case 30:  // preinfusion pause
                debugPrintln("preinfusion pause");
                digitalWrite(PINVALVE, relayON);
                digitalWrite(PINPUMP, relayOFF);
                brewcounter = 31;

                break;

            case 31:  // waiting time preinfusion pause
                if (timeBrewed > ((preinfusion * 1000) + (preinfusionpause * 1000))) {
                    brewcounter = 40;
                }

                break;

            case 40:  // brew running
                debugPrintln("Brew started");
                digitalWrite(PINVALVE, relayON);
                digitalWrite(PINPUMP, relayON);
                brewcounter = 41;

                break;

            case 41:  // waiting time brew
                if (timeBrewed > totalBrewTime || (weightBrew > (weightSetpoint - scaleDelayValue))) {
                    brewcounter = 42;
                }

                if (timeBrewed > totalBrewTime) {
                    brewcounter = 42;
                }

                break;

            case 42:  // brew finished
                debugPrintln("Brew stopped");
                digitalWrite(PINVALVE, relayOFF);
                digitalWrite(PINPUMP, relayOFF);
                brewcounter = 43;

                break;

            case 43:  // waiting for brewswitch off position
                if (brewswitch == LOW) {
                    digitalWrite(PINVALVE, relayOFF);
                    digitalWrite(PINPUMP, relayOFF);

                    // disarmed button
                    currentMillistemp = 0;
                    timeBrewed = 0;
                    brewDetected = 0;  // rearm brewDetection
                    brewcounter = 10;
                }

                weightBrew = weight - weightPreBrew;  // always calculate weight to show on display

                break;
        }
    }
}
#endif
