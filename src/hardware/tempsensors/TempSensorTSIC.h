/**
 * @file TempSensorTSIC.h
 *
 * @brief Handler for TSIC 306 temperature sensor
 */

#pragma once

#include "TempSensor.h"
#include <ZACwire.h>

class TempSensorTSIC final : public TempSensor {
    public:
        explicit TempSensorTSIC(int GPIOPin);

    protected:
        bool sample_temperature(double& temperature) const override;

    private:
        ZACwire* tsicSensor_;
};
