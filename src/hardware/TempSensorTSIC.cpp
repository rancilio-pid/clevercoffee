/**
 * @file TempSensorTSIC.cpp
 *
 * @brief Handler for TSIC 306 temperature sensor
 */

#include "TempSensorTSIC.h"

#define MAX_CHANGERATE 15

TempSensorTSIC::TempSensorTSIC(int GPIOPin) {
    // Set pin to receive signal from the TSic 306
    tsicSensor_ = new ZACwire(GPIOPin, 306);
    // Start sampling the TSic sensor
    tsicSensor_->begin();
}

float TempSensorTSIC::getTemperatureCelsius() const { return tsicSensor_->getTemp(MAX_CHANGERATE); }
