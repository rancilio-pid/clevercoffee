/**
 * @file ISR.h
 *
 * @brief Timer - ISR for PID calculation and heat realay output
 */

#ifndef _ISR_H_
#define _ISR_H_

extern unsigned long windowStartTime;
extern double Output; 
extern unsigned int isrCounter;
extern unsigned int windowSize;
extern bool skipHeaterISR;

void initTimer1(void);
void enableTimer1(void);
void disableTimer1(void);
bool isTimer1Enabled(void);

#endif