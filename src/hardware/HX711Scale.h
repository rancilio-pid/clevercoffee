
/**
 * @file HX711Scale.h
 * @brief HX711-based scale implementation
 */

#pragma once

#include "Scale.h"

#define HX711_ADC_config_h
#define SAMPLES                32
#define IGN_HIGH_SAMPLE        1
#define IGN_LOW_SAMPLE         1
#define SCK_DELAY              1
#define SCK_DISABLE_INTERRUPTS 0

#include <HX711_ADC.h>

#define HIGH_ACCURACY

/**
 * @brief HX711-based scale implementation
 * Supports single or dual load cell setup with shared clock pin
 */
class HX711Scale : public Scale {
    public:
        /**
         * @brief Constructor for single HX711 scale
         * @param dataPin Data pin for HX711
         * @param clkPin Clock pin for HX711
         * @param calibrationFactor Initial calibration factor
         */
        HX711Scale(int dataPin, int clkPin, float calibrationFactor = 1.0);

        /**
         * @brief Constructor for dual HX711 scale setup (shared clock)
         * @param dataPin1 Data pin for first HX711
         * @param dataPin2 Data pin for second HX711
         * @param clkPin Shared clock pin for both HX711s
         * @param calibrationFactor1 Initial calibration factor for first cell
         * @param calibrationFactor2 Initial calibration factor for second cell
         */
        HX711Scale(int dataPin1, int dataPin2, int clkPin, float calibrationFactor1 = 1.0, float calibrationFactor2 = 1.0);

        ~HX711Scale() override;

        bool init() override;
        bool update() override;
        [[nodiscard]] float getWeight() const override;
        void tare() override;
        void setSamples(int samples) override;

        /**
         * @brief Get calibration factor for specific cell
         * @param cellNumber Cell number (1 or 2)
         * @return Calibration factor
         */
        [[nodiscard]] float getCalibrationFactor(int cellNumber = 1) const;

        /**
         * @brief Set calibration factor for specific cell
         * @param factor Calibration factor
         * @param cellNumber Cell number (1 or 2)
         */
        void setCalibrationFactor(float factor, int cellNumber = 1);

        /**
         * @brief Get access to underlying HX711_ADC object for advanced operations
         * @param cellNumber Cell number (1 or 2)
         * @return Pointer to HX711_ADC object or nullptr if invalid
         */
        HX711_ADC* getLoadCell(int cellNumber = 1);

    private:
        HX711_ADC* loadCell1;
        HX711_ADC* loadCell2;

        float currentWeight;
        float calibrationFactor1;
        float calibrationFactor2;
        bool isDualCell;

        // For alternating reads in dual scale setup
        bool readSecondScale;
        float weight1;
        float weight2;
};
