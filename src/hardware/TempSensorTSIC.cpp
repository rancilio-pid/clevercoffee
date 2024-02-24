/**
 * @file TempSensorTSIC.cpp
 * 
 * @brief Handler for TSIC 306 temperature sensor
 */

#include "TempSensorTSIC.h"
#include <ZACwire.h>

#define MAX_CHANGERATE 15

ZACwire* tsicSensor;

TempSensorTSIC::TempSensorTSIC(int GPIOPin) {
    tsicSensor = new ZACwire(GPIOPin, 306);    // set pin to receive signal from the TSic 306
}

float TempSensorTSIC::getTempinCelsius() const {
    return tsicSensor->getTemp(MAX_CHANGERATE);
}
