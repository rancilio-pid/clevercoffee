/**
 * @file StatusLED.h
 *
 * @brief Implementation of LED behaviour
 */

#pragma once

unsigned long brightnessLED;

void statusLEDSetup() {
    #if FEATURE_STATUS_LED == 1
        #if STATUS_LED_TYPE == 0
            pinMode(PIN_STATUSLED, OUTPUT);
        #elif STATUS_LED_TYPE == 1
            FastLED.addLeds<RGB_LED_TYPE, PIN_STATUSLED>(leds, RGB_LED_NUM);
        #endif
    #endif
}

void loopLED() {
#if STATUS_LED_TYPE == 0
    //analog LED
    if ((machineState == kPidNormal && (fabs(temperature  - setpoint) < 0.3)) || (temperature > 115 && fabs(temperature - setpoint) < 5)) {
        digitalWrite(PIN_STATUSLED, HIGH);
    }
    else {
        digitalWrite(PIN_STATUSLED, LOW);
    }
#endif

#if STATUS_LED_TYPE == 1
    unsigned long currentMillisLED = millis();

    if (currentMillisLED - previousMillisLED >= intervalLED) {
        previousMillisLED = currentMillisLED;

        // Color cycle variable
        cycleLED++;
        
        if (cycleLED >= 255) {
            cycleLED = 0;
        }

        // Standby -> Switch LEDs off, but overwrite with other states
        fill_solid(leds, RGB_LED_NUM, CRGB::Black); 

        if (machineState == kPidNormal) {        
            // PID heating (no standby) - white 20% low brightness 
            fill_solid(leds, RGB_LED_NUM, CHSV(0,0,RGB_LED_PIDON_BRIGHTNESS));

            if ((fabs(temperature  - setpoint) < 0.3) || (temperature > 115 && fabs(temperature - setpoint) < 5)) { 
                // Correct temp for both Brew and Steam -> Green in back of machine
                leds[0] = CRGB::DarkGreen; 
            }            
        }

        // [For LED_BEHAVIOUR 2]:
        if (RGB_LED_BEHAVIOUR == 2) {

            // Brew coffee -> White, preinfusion a bit toned down
            if (machineState == kBrew || brewSwitchTriggerCase == 31) {

                if (brewCounter >= kBrewRunning) {
                    fill_solid(leds, RGB_LED_NUM, CRGB::White); 
                }
                else {
                    // during preinfusion, put 40% brightness white
                    fill_solid(leds, RGB_LED_NUM, CHSV(0, 0, 100));
                }
            }

            // Set higher temperature for Steam -> dark yellow (if not at correct temperature)
            if (machineState == kSteam && !(temperature > 115 && fabs(temperature - setpoint) < 5) ) {
                fill_solid(leds, RGB_LED_NUM, CHSV(45, 255, 100)); // 45 is yellow, 100 is 40% brighteness
            }

            // Initialize & Heat up --> Rainbow
            if (machineState == kInit || machineState == kColdStart) {
                for (int i = 0; i < RGB_LED_NUM; i++) {
                    leds[i] = CHSV(i * 32 - (cycleLED * 2), 255, 255); // Hue, saturation, brightness
                }
            }   
        }

        // Backflush -> White
        if (machineState == kBackflush) {
            fill_solid(leds, RGB_LED_NUM, CRGB::White); 
        }        

        // After coffee is brewed -> White light to show ready coffee for ~15s
        if (brewFinishedLEDon > 0) {
            brewFinishedLEDon--;
            // for the last 10% of the time, smoothly dim down white light on cup
            brightnessLED = (brewFinishedLEDon > (BREW_FINISHED_LEDON_DURATION / 10) ? 200 : 150 * brewFinishedLEDon * 10 / BREW_FINISHED_LEDON_DURATION + RGB_LED_PIDON_BRIGHTNESS);
            fill_solid(leds, RGB_LED_NUM, CHSV(0, 0, brightnessLED)); 
        }
        
        // Error message: Red (Water empty, sensor error, etc) - overrides all other states in LED color
        if (!waterFull || machineState == kSensorError || machineState ==  kEepromError || machineState ==  kEmergencyStop) {
            if (RGB_LED_BEHAVIOUR == 2) {
                // [For LED_BEHAVIOUR 2]: Red heartbeat
                fill_solid(leds, RGB_LED_NUM, CHSV(0, 250, cubicwave8(cycleLED * 4))); //Hue of 0 is red

                if (RGB_LED_NUM > 1) {
                    fill_solid(leds + RGB_LED_NUM / 2, RGB_LED_NUM - RGB_LED_NUM / 2, CHSV(0, 250, cubicwave8(128 + cycleLED * 4))); //Hue of 0 is red
                }
            }
            else {
                // [For LED_BEHAVIOUR 1]: Solid red color
                fill_solid(leds, RGB_LED_NUM, CRGB::Red); 
            }
        }
        
        FastLED.show(); 
    }
#endif

}