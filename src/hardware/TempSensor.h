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
#include "utils/Timer.h"

class TempSensor {
    public:
        /**
         * @brief Abstract temperature sensor class
         * @details This class controls a temperature sensor. It updates the value in regular intervals of 400ms, controlled
         *          by a timer. The temperature sampling result is checked for errors and an internal error counter is kept
         *          to detect consecutive reading errors
         */
        TempSensor() :
            update_temperature(std::bind(&TempSensor::update_temperature_reading, this), 400) {
        }

        /**
         * @brief Returns the current temperature
         * @details Requests sampling from attached sensor and returns reading.
         * @return Temperature in degrees Celsius
         */
        double getCurrentTemperature() {
            // Trigger the timer to update the temperature:
            update_temperature();
            return last_temperature_;
        }

        double getAverageTemperatureRate() {
            // Trigger the timer to update the temperature:
            update_temperature();
            return average_temp_rate_;
        }

        /**
         * @brief Default destructor
         */
        virtual ~TempSensor() = default;

        /**
         * @brief Returns error state of the temperature sensor
         * @return true if the sensor is in error, false otherwise
         */
        bool hasError() {
            return error_;
        }

    protected:
        /**
         * @brief Samples the current temperature from the sensor
         * @details Requests sampling from attached sensor and returns reading. This method is purely virtual and must be
         *          implemented by every child class. The argument is passed by reference and is updated if the return value
         *          of the function is true.
         * @return Boolean indicating whether the reading has been successful
         */
        virtual bool sample_temperature(double& temperature) const = 0;

    private:
        /**
         * @brief Small helper method to be wrapped in a timer for updating the temperature from the sensors
         */
        void update_temperature_reading() {
            LOG(TRACE, "Attempting temperature reading");
            // Update temperature and detect errors:
            auto updated = sample_temperature(last_temperature_);
            if (updated) {
                LOGF(DEBUG, "Temperature reading successful: %.1f", last_temperature_);

                // Reset error counter and error state
                bad_readings_ = 0;
                error_ = false;

                // Update moving average
                update_moving_average();
            }
            else if (!error_) {
                // Increment error counter
                bad_readings_++;
            }

            if (bad_readings_ >= max_bad_treadings_ && !error_) {
                error_ = true;
                LOGF(ERROR, "Temperature sensor malfunction, %i consecutive errors", bad_readings_);
            }
        }

        Timer update_temperature;
        double last_temperature_{};
        int bad_readings_{0};
        int max_bad_treadings_{10};
        bool error_{false};

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

        // Moving average:
        double average_temp_rate_{};
        constexpr static size_t numValues = 15;
        std::array<double, numValues> tempValues{};        // array of temp values
        std::array<unsigned long, numValues> timeValues{}; // array of time values
        std::array<double, numValues> tempChangeRates{};
        int valueIndex{-1};                                // the index of the current value
};
