/**
 * @file steamswitchvoid.h
 *
 * @brief
 */

/**
 * @brief Digtalswitch input pin for STEAM SWITCH
 */

int lastSteamSwitchTrigger = LOW;                   // the previous reading from the input pin
int buttonStateSteamTrigger;                        // the current reading from the input pin
unsigned long lastDebounceTimeSteamTrigger = 0;     // the last time the output pin was toggled
unsigned long debounceDelaySteamTrigger = 20;       // the debounce time; increase if the output flickers

void checkSteamSwitch() {
    #if STEAMSWITCHTYPE == 0
        return;
    #elif STEAMSWITCHTYPE == 1
        // Set steamON to 1 when steamswitch is HIGH
        if (digitalRead(PIN_STEAMSWITCH) == HIGH) {
            steamON = 1;
        } 
        
         // if activated via web interface then steamFirstON == 1, prevent override
        if (digitalRead(PIN_STEAMSWITCH) == LOW && steamFirstON == 0) {
            steamON = 0;
        }

        // monitor QuickMill thermoblock steam-mode
        if (machine == QuickMill) {
            if (steamQM_active == true) {
                if (checkSteamOffQM() == true) {  // if true: steam-mode can be turned off
                    steamON = 0;
                    steamQM_active = false;
                    lastTimePVSwasON = 0;
                } else {
                    steamON = 1;
                }
            }
        }

    #elif STEAMSWITCHTYPE == 2  // TRIGGER
        int reading = digitalRead(PIN_STEAMSWITCH);

        if (reading != lastSteamSwitchTrigger) {
            // reset the debouncing timer
            lastDebounceTimeSteamTrigger = millis();
        }

        if ((millis() - lastDebounceTimeSteamTrigger) > debounceDelaySteamTrigger) {
            // whatever the reading is at, it's been there for longer than the debounce
            // delay, so take it as the actual current state:

            // if the button state has changed:
            if (reading != buttonStateSteamTrigger) {
                buttonStateSteamTrigger = reading;

                // only toggle heating power if the new button state is HIGH
                if (buttonStateSteamTrigger == HIGH) {
                    if (steamON == 0) {
                        Serial.println("Turn Steam ON");
                        steamON = 1;
                    } else {
                        Serial.println("Turn Steam OFF");
                        steamON = 0;
                    }
                }
            }
        }

        lastSteamSwitchTrigger = reading;
    #endif

}
