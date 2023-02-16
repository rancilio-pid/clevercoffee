/**
 * @file powerswitchvoid.h
 *
 * @brief
 */

/**
 * @brief Digtalswitch input pin for POWER SWITCH
 */

int lastpowerswitchTrigger = LOW;                   // the previous reading from the input pin
int buttonStatePowerTrigger;                        // the current reading from the input pin
unsigned long lastDebounceTimePowerTrigger = 0;     // the last time the output pin was toggled
unsigned long debounceDelayPowerTrigger = 50;       // the debounce time; increase if the output flickers

void checkpowerswitch() {
    #if POWERSWITCHTYPE == 1
        // Set pidON to 1 when powerswitch is HIGH
        if (digitalRead(PIN_POWERSWITCH) == HIGH) {
            pidON = 1;
        } else {
            // Set pidON to 0 when powerswitch is not HIGH
            pidON = 0;
        }
    #endif

    #if POWERSWITCHTYPE == 2  // TRIGGER
        int reading = digitalRead(PIN_POWERSWITCH);

        if (reading != lastpowerswitchTrigger) {
            // reset the debouncing timer
            lastDebounceTimePowerTrigger = millis();
        }

        if ((millis() - lastDebounceTimePowerTrigger) > debounceDelayPowerTrigger) {
            // whatever the reading is at, it's been there for longer than the debounce
            // delay, so take it as the actual current state:

            // if the button state has changed:
            if (reading != buttonStatePowerTrigger) {
                buttonStatePowerTrigger = reading;

                // only toggle heating power if the new button state is HIGH
                if (buttonStatePowerTrigger == HIGH) {
                    if (pidON == 0) {
                        Serial.println("Turn Heating ON");
                        pidON = 1;
                    } else {
                        Serial.println("Turn Heating OFF");
                        pidON = 0;
                    }
                }
            }
        }

        lastpowerswitchTrigger = reading;

    #endif
}
