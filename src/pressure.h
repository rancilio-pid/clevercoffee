/**
 * @file pressure.h
 *
 * @brief Honeywell ABP2
 */
#include <Arduino.h>
#include <Wire.h>

/*
    Read Pressure Sensor ABP2LANT010BG2A3XX, cf. https://mou.sr/477clc7 
*/
float ABP2_measurePressure();