/**
 * @file TempSensor.h
 *
 * @brief Interface that all temperature sensors have to implement
 */

#pragma once

#include <cmath>

#include "Logger.h"

class TempSensor {
    public:
        /**
         * @brief Returns the current temperature
         * @details Requests sampling from attached sensor and returns reading.
         * @return Temperature in degrees Celsius
         */
        virtual float getTemperatureCelsius() {
            auto temperature = sample_temperature();
            last_temperature_ = temperature;
            return temperature;
        }

        /**
         * @brief Returns sampling interval for the sensor
         * @details Returns a value to control how often the sensor is sampled. This is a static value, typically fixed for
         *          a given sensor hardware type and is used to avoid polling the sensor too often without new readings
         *          available.
         * @return Sampling interval in milliseconds
         */
        virtual int getSamplingInterval() const {
            return 400;
        };

        /**
         * @brief Default destructor
         */
        virtual ~TempSensor() {
        }

        /**
         * @brief check sensor value.
         * @return If < 0 or difference between old and new >25, then increase error.
         *      If error is equal to maxErrorCounter, then set sensorError
         */
        bool check(float temperature, float offset) {
            bool sensorOK = false;
            bool badCondition = (temperature < 0 || temperature > 150 || std::fabs(temperature - last_temperature_) > (5 + offset));

            if (badCondition && !error_) {
                bad_readings_++;
                sensorOK = false;

                LOGF(WARNING, "temperature sensor reading: consec_errors = %i, temp_current = %.1f, temp_prev = %.1f", bad_readings_, temperature, last_temperature_);
            }
            else if (badCondition == false && sensorOK == false) {
                bad_readings_ = 0;
                sensorOK = true;
            }

            if (bad_readings_ >= max_bad_treadings_ && !error_) {
                error_ = true;
                LOGF(ERROR, "temperature sensor malfunction: temp_current = %.1f", temperature);
            }
            else if (bad_readings_ == 0 && error_) {
                error_ = false;
            }

            return sensorOK;
        }

        bool hasError() {
            return error_;
        }

    protected:
        /**
         * @brief Samples the current temperature from the sensor
         * @details Requests sampling from attached sensor and returns reading. This method is purely virtual and must be
         *          implemented by every child class.
         * @return Temperature in degrees Celsius
         */
        virtual float sample_temperature() const = 0;

    private:
        float last_temperature_{};

        int bad_readings_{0};
        int max_bad_treadings_{10};

        bool error_{false};
};
