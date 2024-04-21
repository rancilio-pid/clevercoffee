/**
 * @file TempSensorDallas.h
 *
 * @brief Handler for Dallas DS18B20 temperature sensor
 */

#pragma once

#include "TempSensor.h"
#include <DallasTemperature.h>

class TempSensorDallas : public TempSensor {
    public:
        TempSensorDallas(int GPIOPin);

    protected:
        bool sample_temperature(double& temperature) const override;

    private:
        OneWire* oneWire_;
        DallasTemperature* dallasSensor_;
        DeviceAddress sensorDeviceAddress_;
};
