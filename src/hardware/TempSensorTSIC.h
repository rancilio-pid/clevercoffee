/**
 * @file TempSensorTSIC.h
 * 
 * @brief Handler for TSIC 306 temperature sensor
 */

#pragma once

#include "TempSensor.h"


class TempSensorTSIC : public TempSensor {
    public:
        TempSensorTSIC(int GPIOPin);
        int getSamplingInterval() const override { return 400; }
        float getTempinCelsius() const override;

    private:

};
