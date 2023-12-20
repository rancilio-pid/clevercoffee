/**
 * @file isr.h
 *
 * @brief Timer - ISR for PID calculation and heater relay output
 *
 */

#pragma once

#include "hardware/Relay.h"

extern double pidOutput;
extern hw_timer_t *timer;
extern Relay heaterRelay;

unsigned int isrCounter = 0;  // counter for ISR
unsigned long windowStartTime;
unsigned int windowSize = 1000;


void IRAM_ATTR onTimer(){
    timerAlarmWrite(timer, 10000, true);

    if (pidOutput <= isrCounter) {
        heaterRelay.off();
    }
    else {
        heaterRelay.on();
    }

    isrCounter += 10; // += 10 because one tick = 10ms

    // set PID output as relay commands
    if (isrCounter >= windowSize) {
        isrCounter = 0;
    }
}

/**
 * @brief Initialize hardware timers
 */
void initTimer1(void) {
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 10000, true);
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