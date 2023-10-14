/**
 * @file ISR.h
 *
 * @brief Timer - ISR for PID calculation and heat realay output
 *
 */

#pragma once

#include "userConfig.h"
#include <ClickEncoder.h>

extern unsigned long windowStartTime;
extern double pidOutput;
extern unsigned int isrCounter;
extern unsigned int windowSize;
extern bool skipHeaterISR;

extern hw_timer_t *timer;
#if (ROTARY_MENU == 1)
extern hw_timer_t *encoderTimer;
extern ClickEncoder encoder;
#endif

void initTimer1(void);
void enableTimer1(void);
void disableTimer1(void);
bool isTimer1Enabled(void);
