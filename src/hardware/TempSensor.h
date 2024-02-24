/**
 * @file TempSensor.h
 * 
 * @brief Interface that all temperature sensors have to implement
 */

#pragma once

class TempSensor {
    public:
        /**
         * @brief Returns the current temperature
         * @details Requests sampling from attached sensor and returns reading. This method is purely virtual and must be
         *          implemented by every child class.
         * @return Temperature in degrees Celsius
         */
        virtual float getTempinCelsius() const = 0;

        /**
         * @brief Returns sampling interval for the sensor
         * @details Returns a value to control how often the sensor is sampled. This is a static value, typically fixed for
         *          a given sensor hardware type and is used to avoid polling the sensor too often without new readings
         *          available.
         * @return Sampling interval in milliseconds
         */
        virtual int getSamplingInterval() const { return 400; };

        /**
         * @brief Default destructor
         */
        virtual ~TempSensor() {}
};
