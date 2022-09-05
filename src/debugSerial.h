/**
 * @file debugSerial.h
 *
 * @brief Logging methods using either serial or network port
 */

#ifndef debugserial_h
#define debugserial_h

#include <ctime>
#include <cstdio>

#include <WiFiManager.h>

void startRemoteSerialServer();
void checkForRemoteSerialClients();
void debugPrint(const char *message);
void debugPrintln(const char *message);
size_t debugPrintf(const char *format, ...);
void getCurrentTimeString(char *output); 

#endif