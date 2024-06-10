/**
 * @file CurrentSensorAnalog.h
 *
 * @brief Handler for analog current sensor
 */

#pragma once

#include "CurrentSensor.h"
#include "GPIOPin.h"

class CurrentSensorAnalog : public CurrentSensor {
    public:
        CurrentSensorAnalog(int pinNumber);

    protected:
        bool sample_current(double& current) const override;

    private:
        const GPIOPin sensorPin_;

};
