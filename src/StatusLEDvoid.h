/**
 * @file StatusLED.h
 *
 * @brief Implementation of LED behaviour
 */

#pragma once

void statusLEDSetup() {
    if (STATUS_LED == 1) {
        pinMode(PIN_STATUSLED, OUTPUT);
    }

    if (STATUS_LED == 2) {
        FastLED.addLeds<RGB_LED_TYPE, PIN_STATUSLED>(leds, NUM_LEDS);
    }
}

void loopLED() {
#if STATUS_LED == 1
    //analog LED
    if ((machineState == kPidNormal && (fabs(temperature  - setpoint) < 0.3)) || (temperature > 115 && fabs(temperature - setpoint) < 5)) {
        digitalWrite(PIN_STATUSLED, HIGH);
    }
    else {
        digitalWrite(PIN_STATUSLED, LOW);
    }
#endif

#if STATUS_LED == 2
    unsigned long currentMillisLED = millis();

    if (currentMillisLED - previousMillisLED >= intervalLED) {
        previousMillisLED = currentMillisLED;

        // Color cycle variable
        cycleLED++;
        
        if(cycleLED>=255)
        {
            cycleLED=0;
        }

        // Heating to setpoint or Standby -> Switch LEDs off, but overwrite with other states
        fill_solid(leds, NUM_LEDS, CRGB::Black); 

        // Correct temp -> Green in back of machine
        if ((machineState == kPidNormal && (fabs(temperature  - setpoint) < 0.3)) || (temperature > 115 && fabs(temperature - setpoint) < 5)) 
        {
            leds[0] = CRGB::DarkGreen; //middle back is switched to be green
        }

        // [For LED_BEHAVIOUR 2]:
        if (LED_BEHAVIOUR == 2){
            
            // Brew coffee -> Blue
            if (machineState == kBrew) {
                fill_solid(leds, NUM_LEDS, CRGB::Navy); 
            }

            // Set higher temperature for Steam -> dark yellow
            if (machineState == kSteam) {
                fill_solid(leds, NUM_LEDS, CHSV(45,255,100)); 
            }

            // Backflush -> White
            if (machineState == kBackflush) {
                fill_solid(leds, NUM_LEDS, CRGB::White); 
            }        
        
            // Initialize & Heat up --> Rainbow
            if (machineState == kInit || machineState == kColdStart) {
                for (int i = 0; i < NUM_LEDS; i++) {
                    leds[i] = CHSV(i*32 - (cycleLED), 255, 255); /* Hue, saturation, brightness */ 
                }
            }   
         }

        // After coffee is brewed -> White light to show ready coffee for ~15s
        if (BrewFinishedLEDon > 0) {
            BrewFinishedLEDon--;
            fill_solid(leds, NUM_LEDS, CRGB::White); 
        }
        
        // Error message: Red (Water empty, sensor error, etc) - overrides all other states in LED color
        if (!waterFull || machineState == kSensorError || machineState ==  kEepromError || machineState ==  kEmergencyStop) {
            if (LED_BEHAVIOUR == 2) {
                // [For LED_BEHAVIOUR 2]: Red heartbeat
                fill_solid(leds, NUM_LEDS, CHSV(0, 250, cubicwave8(cycleLED))); //Hue of 0 is red
            }
            else {
                // [For LED_BEHAVIOUR 1]: Solid red color
                fill_solid(leds, NUM_LEDS, CRGB::Red); 
            }
        }
        
        FastLED.show(); 
    }
#endif

}