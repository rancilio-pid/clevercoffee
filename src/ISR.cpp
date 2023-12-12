/**
 * @file ISR.cpp
 *
 * @brief TODO
 *
 */

#include <Arduino.h>
#include "ISR.h"
#include "pinmapping.h"

unsigned int isrCounter = 0;  // counter for ISR
unsigned long windowStartTime;
unsigned int windowSize = 1000;


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

/**
 * @brief Initialize hardware timers
 */
void initTimer1(void) {
    timer = timerBegin(0, 80, true); //m
    timerAttachInterrupt(timer, &onTimer, true);//m
    timerAlarmWrite(timer, 10000, true);//m
}


void enableTimer1(void) {
    timerAlarmEnable(timer);
}


void disableTimer1(void) {
    timerAlarmDisable(timer);
}


bool isTimer1Enabled(void) {
    return timerAlarmEnabled(timer);
}
