/**
 * @file BluetoothScale.h
 * @brief Bluetooth scale implementation using AcaiaArduinoBLE library
 */

#pragma once

#include "Scale.h"
#include <AcaiaArduinoBLE.h>

/**
 * @brief Bluetooth scale implementation for Acaia and compatible scales
 */
class BluetoothScale : public Scale {
    public:
        explicit BluetoothScale(bool debug = false);

        ~BluetoothScale() override;

        bool init() override;
        bool update() override;
        [[nodiscard]] float getWeight() const override;
        void tare() override;
        void setSamples(int samples) override;
        [[nodiscard]] bool isConnected() const override;
        void startTimer() const;
        void stopTimer() const;
        void resetTimer() const;

        void updateConnection();
        [[nodiscard]] bool isConnecting() const;

    private:
        AcaiaArduinoBLE* bleScale;
        float currentWeight;
        unsigned long lastUpdateTime;
        bool connected;

        // Connection retry mechanism
        bool bleInitialized;
        unsigned long lastConnectionAttempt;
        unsigned long connectionAttemptInterval;

        bool isUpdatingConnection;
        unsigned long maxConnectionAttemptInterval;
};
