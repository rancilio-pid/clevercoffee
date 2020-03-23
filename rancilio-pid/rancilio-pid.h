#ifndef RancilioPid_h
#define RancilioPid_h

#define LIBRARY_VERSION	0.0.1

#define SHOW_HELP false
#define MAX_TIME_INACTIVE 1800000
#ifdef MAX_TIME_INACTIVE
#undef MAX_TIME_INACTIVE
#define MAX_TIME_INACTIVE 1800000  // RemoteDebug: 30min inactivity time. default is 10min(600000)
#endif
#ifdef SHOW_HELP
#undef SHOW_HELP
#define SHOW_HELP false
#endif
#include "RemoteDebug.h"  //https://github.com/JoaoLopesF/RemoteDebug

//RemoteDebug Debug;

// Debug mode is active if "#define DEBUGMODE" is not commented
#define DEBUGMODE

#ifndef DEBUGMODE
#define DEBUG_print(fmt, ...)
#define DEBUG_println(a)
#define ERROR_print(fmt, ...)
#define ERROR_println(a)
#define DEBUGSTART(a)
#else
#define DEBUG_print(fmt, ...) if (Debug.isActive(Debug.DEBUG)) Debug.printf("%0u " fmt, millis()/1000, ##__VA_ARGS__)
#define DEBUG_println(a) if (Debug.isActive(Debug.DEBUG)) Debug.printf("%0u %s\n", millis()/1000, a)
#define ERROR_print(fmt, ...) if (Debug.isActive(Debug.ERROR)) Debug.printf("%0u " fmt, millis()/1000, ##__VA_ARGS__)
#define ERROR_println(a) if (Debug.isActive(Debug.ERROR)) Debug.printf("%0u %s\n", millis()/1000, a)
#define DEBUGSTART(a) Serial.begin(a);
#endif

//returns heater utilization in percent
double convertOutputToUtilisation(double);

//returns heater utilization in Output
double convertUtilisationToOutput(double);

double pastTemperatureChange(int);

bool almostEqual(float, float);

#endif
