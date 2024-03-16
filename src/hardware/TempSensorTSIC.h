/**
 * @file TempSensorTSIC.h
 *
 * @brief Handler for TSIC 306 temperature sensor
 */

#pragma once

#include "TempSensor.h"
#include <ZACwire.h>

class TempSensorTSIC : public TempSensor {
    public:
        TempSensorTSIC(int GPIOPin);
        float getTemperatureCelsius() const override;

    private:
        ZACwire* tsicSensor_;
};
