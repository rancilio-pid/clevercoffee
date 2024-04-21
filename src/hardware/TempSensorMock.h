/**
 * @file TempSensorMock.h
 *
 * @brief Handler for temperature sensor mock for testing without hardware
 */

#pragma once

#include "TempSensor.h"

class TempSensorMock : public TempSensor {
    public:
        TempSensorMock(int function);
        float getTemperatureCelsius() const override;

    private:
        int function_;

        float getTemperature1(float maxValue);
};
