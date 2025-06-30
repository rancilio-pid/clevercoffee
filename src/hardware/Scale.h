
/**
 * @file Scale.h
 * @brief Scale interface and implementations
 */

#pragma once

/**
 * @brief Abstract base class for scale implementations
 */
class Scale {
    public:
        virtual ~Scale() = default;

        /**
         * @brief Initialize the scale
         * @return true if initialization successful, false otherwise
         */
        virtual bool init() = 0;

        /**
         * @brief Check if scale data is available and update readings
         * @return true if new data is available, false otherwise
         */
        virtual bool update() = 0;

        /**
         * @brief Get the current weight reading
         * @return Weight in grams
         */
        virtual float getWeight() const = 0;

        /**
         * @brief Tare the scale (set current weight as zero point)
         */
        virtual void tare() = 0;

        /**
         * @brief Start calibration process
         * @param knownWeight The known weight to use for calibration
         * @return true if calibration started successfully
         */
        virtual bool startCalibration(float knownWeight) = 0;

        /**
         * @brief Set the number of samples to use for readings
         * @param samples Number of samples
         */
        virtual void setSamples(int samples) = 0;
};
