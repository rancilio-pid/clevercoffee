/**
 * @file TempSensor.h
 *
 * @brief Interface that all temperature sensors have to implement
 */

#pragma once

#include <array>
#include <cmath>
#include <numeric>

#include "Logger.h"

class TempSensor {
    public:
        /**
         * @brief Returns the current temperature
         * @details Requests sampling from attached sensor and returns reading.
         * @return Temperature in degrees Celsius
         */
        virtual float getCurrentTemperature() {
            auto temperature = sample_temperature();
            last_temperature_ = temperature;
            update_moving_average();
            return temperature;
        }

        float getAverageTemperatureRate() {
            return average_temp_rate_;
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
        /**
         * @brief FIR moving average filter for software brew detection
         */
        void update_moving_average() {
            if (valueIndex < 0) {
                for (int index = 0; index < numValues; index++) {
                    tempValues[index] = last_temperature_;
                    timeValues[index] = 0;
                    tempChangeRates[index] = 0;
                }
            }

            timeValues[valueIndex] = millis();
            tempValues[valueIndex] = last_temperature_;

            // local change rate of temperature
            double tempChangeRate = 0;

            if (valueIndex == numValues - 1) {
                tempChangeRate = (tempValues[numValues - 1] - tempValues[0]) / (timeValues[numValues - 1] - timeValues[0]) * 10000;
            }
            else {
                tempChangeRate = (tempValues[valueIndex] - tempValues[valueIndex + 1]) / (timeValues[valueIndex] - timeValues[valueIndex + 1]) * 10000;
            }

            tempChangeRates[valueIndex] = tempChangeRate;

            double totalTempChangeRateSum = std::accumulate(std::begin(tempChangeRates), std::end(tempChangeRates), 0, std::plus<double>());
            average_temp_rate_ = totalTempChangeRateSum / numValues * 100;

            if (valueIndex >= numValues - 1) {
                // ...wrap around to the beginning:
                valueIndex = 0;
            }
            else {
                valueIndex++;
            }
        }

        float last_temperature_{};
        float average_temp_rate_{};

        int bad_readings_{0};
        int max_bad_treadings_{10};

        bool error_{false};

        // Moving average:
        constexpr static size_t numValues = 15;
        std::array<double, numValues> tempValues{};        // array of temp values
        std::array<unsigned long, numValues> timeValues{}; // array of time values
        std::array<double, numValues> tempChangeRates{};
        int valueIndex{-1};                                // the index of the current value
};
