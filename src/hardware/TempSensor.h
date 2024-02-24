/**
 * @file TempSensor.h
 * 
 * @brief Interface that all temperature sensors have to implement
 */

#pragma once

class TempSensor {
    public:
        virtual float getTempinCelsius() const = 0;
        virtual int getSamplingInterval() const = 0;
        virtual ~TempSensor() {}
};
