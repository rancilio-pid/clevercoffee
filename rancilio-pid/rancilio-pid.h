#ifndef RancilioPid_h
#define RancilioPid_h

#define LIBRARY_VERSION	0.0.1

#include "userConfig.h"

#include "src/RemoteDebug/RemoteDebug.h" //https://github.com/JoaoLopesF/RemoteDebug
//#include <RemoteDebug.h>  // uncomment this line AND delete src/RemoteDebug/ folder, if you want to use system lib
extern RemoteDebug Debug;

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

#define LCDWidth                        u8g2.getDisplayWidth()
#define ALIGN_CENTER(t)                 ((LCDWidth - (u8g2.getUTF8Width(t))) / 2)
#define ALIGN_RIGHT(t)                  (LCDWidth - u8g2.getUTF8Width(t))
#define ALIGN_RIGHT_2(t1,t2)            (LCDWidth - u8g2.getUTF8Width(t1) - u8g2.getUTF8Width(t2))
#define ALIGN_LEFT                      0

//returns heater utilization in percent
double convertOutputToUtilisation(double);

//returns heater utilization in Output
double convertUtilisationToOutput(double);

double pastTemperatureChange(int);

bool almostEqual(float, float);

#endif
