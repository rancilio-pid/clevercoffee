/**
 * @file BluetoothScale.cpp
 * @brief Bluetooth scale implementation
 */

#include "BluetoothScale.h"

#include "Logger.h"

#include <Arduino.h>
#include <ArduinoBLE.h>

BluetoothScale::BluetoothScale() :
    currentWeight(0.0), lastUpdateTime(0), connected(false), bleInitialized(false), lastConnectionAttempt(0), connectionAttemptInterval(5000) {
    bleScale = new AcaiaArduinoBLE(false);
}

BluetoothScale::~BluetoothScale() {
    if (bleScale) {
        delete bleScale;
    }
}

bool BluetoothScale::attemptConnection() {
    if (bleScale->init()) {
        connected = true;
        LOG(INFO, "Bluetooth scale connected successfully");

        // Double tare as shown in the library example
        bleScale->tare();
        bleScale->tare();

        return true;
    }

    return false;
}

bool BluetoothScale::init() {
    if (!BLE.begin()) {
        LOG(ERROR, "BLE initialization failed");
        return false;
    }

    bleInitialized = true;

    LOG(INFO, "Attempting to connect to Bluetooth scale...");

    if (attemptConnection()) {
        return true;
    }

    LOG(WARNING, "Initial Bluetooth scale connection failed, will retry in update()");
    lastConnectionAttempt = millis();

    return false;
}

bool BluetoothScale::update() {
    if (!bleInitialized) {
        return false;
    }

    // If not connected, try to reconnect periodically
    if (const unsigned long currentTime = millis(); !connected && (currentTime - lastConnectionAttempt > connectionAttemptInterval)) {
        lastConnectionAttempt = currentTime;

        LOG(INFO, "Attempting to reconnect to Bluetooth scale...");

        if (attemptConnection()) {
            // Reset connection attempt interval to normal
            connectionAttemptInterval = 5000;
        }
        else {
            LOG(DEBUG, "Bluetooth scale reconnection attempt failed");

            // Increase interval slightly to avoid spam, but cap it
            connectionAttemptInterval = min(connectionAttemptInterval + 1000, 10000UL);
        }
    }

    if (connected) {
        // Send heartbeat if required to maintain connection
        if (bleScale->heartbeatRequired()) {
            bleScale->heartbeat();
        }

        if (bleScale->newWeightAvailable()) {
            if (const float newWeight = bleScale->getWeight(); newWeight >= 0 && newWeight < 10000) {
                currentWeight = newWeight;
                return true;
            }
        }

        // Check connection status
        if (!bleScale->isConnected()) {
            connected = false;

            LOG(WARNING, "Bluetooth scale connection lost");

            // Reset connection attempt interval for quicker retry
            connectionAttemptInterval = 1000;
            lastConnectionAttempt = 0;
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
    return connected && bleScale->isConnected();
}
