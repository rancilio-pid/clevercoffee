/**
 * @file BluetoothScale.cpp
 * @brief Bluetooth scale implementation
 */

#include "BluetoothScale.h"
#include "Logger.h"
#include <Arduino.h>

BluetoothScale::BluetoothScale() :
    currentWeight(0.0), lastUpdateTime(0), connected(false), bleInitialized(false), lastConnectionAttempt(0), connectionAttemptInterval(5000) {
    bleScale = new AcaiaArduinoBLE(false);
}

BluetoothScale::~BluetoothScale() {
    delete bleScale;
}

bool BluetoothScale::init() {
    LOG(INFO, "Starting Bluetooth scale initialization");

    const bool success = bleScale->init();

    if (success) {
        bleInitialized = true;
        lastConnectionAttempt = millis();
        LOG(INFO, "BLE Scale initialization successful");
    }
    else {
        LOG(ERROR, "BLE Scale initialization failed");
        bleInitialized = false;
    }

    return success;
}

void BluetoothScale::updateConnection() {
    if (!bleInitialized) {
        return;
    }

    bleScale->updateConnection();

    if (const bool newConnected = bleScale->isConnected(); newConnected != connected) {
        connected = newConnected;

        if (connected) {
            LOG(INFO, "Bluetooth scale connected");
        }
        else {
            LOG(INFO, "Bluetooth scale disconnected");
        }
    }
}

bool BluetoothScale::isConnecting() const {
    if (!bleInitialized) {
        return false;
    }

    return bleScale->isConnecting();
}

bool BluetoothScale::update() {
    if (!bleInitialized) {
        return false;
    }

    if (connected) {
        if (bleScale->heartbeatRequired()) {
            bleScale->heartbeat();
        }

        if (bleScale->newWeightAvailable()) {
            if (const float newWeight = bleScale->getWeight(); newWeight >= 0 && newWeight < 10000) {
                currentWeight = newWeight;
                return true;
            }
        }
    }

    return false;
}

float BluetoothScale::getWeight() const {
    return currentWeight;
}

void BluetoothScale::tare() {
    if (connected) {
        bleScale->tare();
    }
}

void BluetoothScale::setSamples(int samples) {
    // Most BLE scales handle sampling internally
}

bool BluetoothScale::isConnected() const {
    return connected;
}
