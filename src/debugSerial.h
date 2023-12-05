/**
 * @file debugSerial.h
 *
 * @brief Debug logging
 */

#ifndef debugserial_h
#define debugserial_h

#include <ctime>
#include <cstdio>
#include <Arduino.h>

void debugPrint(const char *message);
void debugPrintln(const char *message);
size_t debugPrintf(const char *format, ...);
void getCurrentTimeString(char *output); 

#endif