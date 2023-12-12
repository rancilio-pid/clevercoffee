/**
 * @file ISR.h
 *
 * @brief Timer - ISR for PID calculation and heat realay output
 *
 */

#pragma once

extern unsigned long windowStartTime;
extern double pidOutput;
extern unsigned int isrCounter;
extern unsigned int windowSize;

extern hw_timer_t *timer;

void initTimer1(void);
void enableTimer1(void);
void disableTimer1(void);
bool isTimer1Enabled(void);
