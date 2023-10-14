/**
 * @file ISR.h
 *
 * @brief Timer - ISR for PID calculation and heat realay output
 */

#ifndef _ISR_H_
#define _ISR_H_

#include <ClickEncoder.h>

extern unsigned long windowStartTime;
extern double pidOutput; 
extern unsigned int isrCounter;
extern unsigned int windowSize;
extern bool skipHeaterISR;

#if defined(ESP32)
extern hw_timer_t *timer;
extern hw_timer_t *encoderTimer;
extern ClickEncoder encoder;
#endif

// #if (ROTARY_MENU == 1) and defined(ESP32)
// extern ClickEncoder *encoder;
// #endif

void initTimer1(void);
void enableTimer1(void);
void disableTimer1(void);
bool isTimer1Enabled(void);

#endif
