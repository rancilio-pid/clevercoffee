/**
 * @file CurrentSensor.h
 *
 * @brief Interface that all current sensors have to implement
 */

#pragma once

#include <array>
#include <cmath>
#include <numeric>

#include "Logger.h"
#include "utils/Timer.h"

class CurrentSensor {
    public:
        /**
         * @brief Abstract current sensor class
         * @details This class controls a current sensor. It updates the value in regular intervals of 400ms, controlled
         *          by a timer. The current sampling result is checked for errors and an internal error counter is kept
         *          to detect consecutive reading errors
         */
        CurrentSensor() :
            update_current(std::bind(&CurrentSensor::update_current_reading, this), 400) {
        }

        /**
         * @brief Returns the current 
         * @details Requests sampling from attached sensor and returns reading.
         * @return Current in Ampere
         */
        double getCurrentCurrent() {
            // Trigger the timer to update the current:
            update_current();
            return last_current_;
        }

        double getAverageCurrentRate() {
            // Trigger the timer to update the current:
            update_current();
            return average_current_rate_;
        }

        /**
         * @brief Default destructor
         */
        virtual ~CurrentSensor() = default;

        /**
         * @brief Returns error state of the current sensor
         * @return true if the sensor is in error, false otherwise
         */
        bool hasError() {
            return error_;
        }

    protected:
        /**
         * @brief Samples the current current from the sensor
         * @details Requests sampling from attached sensor and returns reading. This method is purely virtual and must be
         *          implemented by every child class. The argument is passed by reference and is updated if the return value
         *          of the function is true.
         * @return Boolean indicating whether the reading has been successful
         */
        virtual bool sample_current(double& current) const = 0;

    private:
        /**
         * @brief Small helper method to be wrapped in a timer for updating the current from the sensors
         */
        void update_current_reading() {
            // Update current and detect errors:
            auto updated = sample_current(last_current_);
            if (updated) {
                LOGF(TRACE, "Current reading successful: %.1f", last_current_);

                // Reset error counter and error state
                bad_readings_ = 0;
                error_ = false;

                // Update moving average
                update_moving_average();
            }
            else if (!error_) {
                // Increment error counter
                LOGF(DEBUG, "Error during current reading, incrementing error counter to %i", bad_readings_);
                bad_readings_++;
            }

            if (bad_readings_ >= max_bad_treadings_ && !error_) {
                error_ = true;
                LOGF(ERROR, "Current sensor malfunction, %i consecutive errors", bad_readings_);
            }
        }

        Timer update_current;
        double last_current_{};
        int bad_readings_{0};
        int max_bad_treadings_{10};
        bool error_{false};

        /**
         * @brief FIR moving average filter for software brew detection
         */
        void update_moving_average() {
            if (valueIndex < 0) {
                for (int index = 0; index < numValues; index++) {
                    currentValues[index] = last_current_;
                    timeValues[index] = 0;
                    currentChangeRates[index] = 0;
                }
            }

            timeValues[valueIndex] = millis();
            currentValues[valueIndex] = last_current_;

            // local change rate of current
            double currentChangeRate = 0;

            if (valueIndex == numValues - 1) {
                currentChangeRate = (currentValues[numValues - 1] - currentValues[0]) / (timeValues[numValues - 1] - timeValues[0]) * 10000;
            }
            else {
                currentChangeRate = (currentValues[valueIndex] - currentValues[valueIndex + 1]) / (timeValues[valueIndex] - timeValues[valueIndex + 1]) * 10000;
            }

            currentChangeRates[valueIndex] = currentChangeRate;

            double totalCurrentChangeRateSum = std::accumulate(std::begin(currentChangeRates), std::end(currentChangeRates), 0, std::plus<double>());
            average_current_rate_ = totalCurrentChangeRateSum / numValues * 100;

            if (valueIndex >= numValues - 1) {
                // ...wrap around to the beginning:
                valueIndex = 0;
            }
            else {
                valueIndex++;
            }
        }

        // Moving average:
        double average_current_rate_{};
        constexpr static size_t numValues = 15;
        std::array<double, numValues> currentValues{};        // array of current values
        std::array<unsigned long, numValues> timeValues{}; // array of time values
        std::array<double, numValues> currentChangeRates{};
        int valueIndex{-1};                                // the index of the current value
};
