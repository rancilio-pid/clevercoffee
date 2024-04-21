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

    protected:
        float sample_temperature() const override;

    private:
        ZACwire* tsicSensor_;
};
