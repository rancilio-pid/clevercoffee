
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
        [[nodiscard]] virtual float getWeight() const = 0;

        /**
         * @brief Tare the scale (set current weight as zero point)
         */
        virtual void tare() = 0;

        /**
         * @brief Set the number of samples to use for readings
         * @param samples Number of samples
         */
        virtual void setSamples(int samples) = 0;

        /**
         * @brief Check if scale is connected (for Bluetooth scales)
         * @return true if connected, false for wired scales or if not connected
         */
        [[nodiscard]] virtual bool isConnected() const {
            return true;
        }
};
