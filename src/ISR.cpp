/**
 * @file ISR.cpp
 *
 * @brief TODO
 *
 */

#include <Arduino.h>
#include "ISR.h"
#include "pinmapping.h"
#include "userConfig.h"

unsigned int isrCounter = 0;  // counter for ISR
unsigned long windowStartTime;
unsigned int windowSize = 1000;
bool skipHeaterISR = false;


void IRAM_ATTR onTimer(){
    timerAlarmWrite(timer, 10000, true);

    if (pidOutput <= isrCounter) {
        digitalWrite(PIN_HEATER, LOW);
    } else {
        digitalWrite(PIN_HEATER, HIGH);
    }

    isrCounter += 10; // += 10 because one tick = 10ms

    //set PID output as relais commands
    if (isrCounter >= windowSize) {
        isrCounter = 0;
    }
}

#if (ROTARY_MENU) == 1 
void IRAM_ATTR onRotaryTimer() {
    timerAlarmWrite(encoderTimer, menuOpen ? 1000 : 10000, true); // 1ms = 1000 ticks at 1MHz
    encoder.service();
}
#endif

/**
 * @brief Initialize hardware timers
 */
void initTimer1(void) {
    timer = timerBegin(0, 80, true); //m
    timerAttachInterrupt(timer, &onTimer, true);//m
    timerAlarmWrite(timer, 10000, true);//m

    #if (ROTARY_MENU == 1)
        // Initializing another timer for encoder service
        Serial.println("Initializing another timer for encoder service");
        encoderTimer = timerBegin(1, 80, true);
        timerAttachInterrupt(encoderTimer, &onRotaryTimer, true);
        timerAlarmWrite(encoderTimer, 1000, true);
    #endif
}


void enableTimer1(void) {
    timerAlarmEnable(timer);
    #if ROTARY_MENU == 1
    timerAlarmEnable(encoderTimer);
    #endif
}


void disableTimer1(void) {
    timerAlarmDisable(timer);
    #if ROTARY_MENU == 1
    timerAlarmDisable(encoderTimer);
    #endif
}


bool isTimer1Enabled(void) {
    return timerAlarmEnabled(timer);
}
