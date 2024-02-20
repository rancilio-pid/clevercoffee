/**
 * @file TempSensor.h
 * 
 * @brief Interface that all temperature sensors have to implement
 */

#pragma once

class TempSensor {
    public:
        virtual float getTempinCelsius() = 0;
        virtual ~TempSensor() {}
};