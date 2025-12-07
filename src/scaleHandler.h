/**
 * @file scaleHandler.h
 *
 * @brief Implementation of scale initialization and weight measurement with Bluetooth support
 */

#pragma once

#include "brewStates.h"
#include "display/languages.h"
#include "hardware/scales/BluetoothScale.h"
#include "hardware/scales/HX711Scale.h"

void displayScaleFailed();
void displayWrappedMessage(const String& message, int x = 0, int startY = 0, int spacing = 2, boolean clearSend = true, boolean wrapWord = false);

inline bool scaleCalibrationOn = false;
inline bool scaleTareOn = false;
inline int shottimerCounter = 10;
inline float currReadingWeight = 0; // current weight reading
inline float preBrewWeight = 0;     // weight before brew started
inline float currBrewWeight = 0;    // weight of current brew
inline float scaleDelayValue = 2.5; // delay compensation in grams
inline bool scaleFailure = false;
inline bool autoTareInProgress = false;
inline unsigned long autoTareStartTime = 0;

// Bluetooth scale connection handling
inline unsigned long lastScaleConnectionCheck = 0;
inline unsigned long scaleConnectionFailureTime = 0;
inline bool scaleConnectionLost = false;
inline float lastValidWeight = 0;
inline bool brewByWeightFallbackActive = false;

// Scale connection constants
constexpr unsigned long SCALE_CONNECTION_CHECK_INTERVAL = 500; // Check every 500 milliseconds
constexpr unsigned long SCALE_CONNECTION_TIMEOUT = 5000;       // 5 seconds timeout
constexpr unsigned long SCALE_RECONNECTION_TIMEOUT = 30000;    // 30 seconds before giving up

inline Scale* scale = nullptr;
inline bool isBluetoothScale = false;

extern BrewState currBrewState;

/**
 * @brief Check Bluetooth scale connection status and handle failures
 */
inline void checkBluetoothScaleConnection() {
    if (!isBluetoothScale || !scale) {
        return;
    }

    // Check connection status periodically for logging/fallback logic
    if (const unsigned long currentTime = millis(); currentTime - lastScaleConnectionCheck > SCALE_CONNECTION_CHECK_INTERVAL) {
        static_cast<BluetoothScale*>(scale)->updateConnection();

        lastScaleConnectionCheck = currentTime;

        if (const bool connected = scale->isConnected(); !connected) {
            if (!scaleConnectionLost) {
                // Connection just lost
                scaleConnectionLost = true;
                scaleConnectionFailureTime = currentTime;

                LOG(WARNING, "Bluetooth scale connection lost");

                // During active brew, activate fallback mechanism
                if (currBrewState != kBrewIdle && currBrewState != kBrewFinished) {
                    const bool brewByWeightEnabled = config.get<bool>("brew.by_weight.enabled");
                    const bool brewByTimeEnabled = config.get<bool>("brew.by_time.enabled");

                    if (brewByWeightEnabled && brewByTimeEnabled) {
                        LOG(INFO, "Activating brew-by-time fallback due to scale connection loss");
                        brewByWeightFallbackActive = true;
                    }
                    else if (brewByWeightEnabled) {
                        LOG(WARNING, "BLE Scale connection lost during brew-by-weight only mode, stopping brew");
                        currBrewState = kBrewFinished;
                    }
                }
            }

            // Check if we should give up reconnecting
            if (currentTime - scaleConnectionFailureTime > SCALE_RECONNECTION_TIMEOUT) {
                if (!scaleFailure) {
                    LOG(ERROR, "Bluetooth scale connection timeout - marking as failed");
                    scaleFailure = true;
                }
            }
        }
        else {
            // Connection restored
            if (scaleConnectionLost) {
                scaleConnectionLost = false;
                scaleFailure = false;
                brewByWeightFallbackActive = false;
                LOG(INFO, "Bluetooth scale connection restored");
            }
        }
    }
}

/**
 * @brief Get weight with connection error handling
 */
inline float getScaleWeight() {
    if (!scale) {
        return lastValidWeight;
    }

    if (isBluetoothScale) {
        // Always check connection - even if we've marked it as failed
        checkBluetoothScaleConnection();

        if (scaleConnectionLost) {
            // Return last valid weight during connection issues
            return lastValidWeight;
        }
    }

    if (scale->update()) {
        const float weight = scale->getWeight();
        lastValidWeight = weight;
        return weight;
    }

    return lastValidWeight;
}

/**
 * @brief Check if brew-by-weight should be used (considering fallback state)
 */
inline bool shouldUseBrewByWeight() {
    const bool brewByWeightEnabled = config.get<bool>("brew.by_weight.enabled");
    return brewByWeightEnabled && !brewByWeightFallbackActive && !scaleConnectionLost;
}

inline void scaleCalibrate(const int cellNumber, const int pin) {
    if (isBluetoothScale) {
        // Bluetooth scales handle calibration internally
        displayWrappedMessage("Bluetooth scales\nhandle calibration\ninternally");
        delay(2000);
        return;
    }

    const int scaleSamples = config.get<int>("hardware.sensors.scale.samples");

    auto* hx711Scale = static_cast<HX711Scale*>(scale);
    HX711_ADC* loadCell = hx711Scale->getLoadCell(cellNumber);

    if (!loadCell) {
        return;
    }

    loadCell->setCalFactor(1.0);

    String msg = langstring_calibrate_start + String(cellNumber) + "\n";
    displayWrappedMessage(msg);
    delay(2000);

    LOGF(INFO, "Taking scale %d, pin %d to zero point", cellNumber, pin);

    loadCell->update();
    loadCell->tare();

    LOGF(INFO, "Put load on scale %d within the next 10 seconds", pin);

    const auto scaleKnownWeight = ParameterRegistry::getInstance().getParameterById("hardware.sensors.scale.known_weight")->getValueAs<float>();

    msg = langstring_calibrate_in_progress + String(scaleKnownWeight, 2) + "g\n";
    displayWrappedMessage(msg);
    delay(10000);

    LOG(INFO, "Taking scale load point");

    // increase scale samples temporarily to ensure a stable reading
    loadCell->setSamplesInUse(128);
    loadCell->refreshDataSet();
    const float calibration = loadCell->getNewCalibration(scaleKnownWeight);
    loadCell->setSamplesInUse(scaleSamples);

    LOGF(INFO, "New calibration: %f", calibration);

    hx711Scale->setCalibrationFactor(calibration, cellNumber);

    // Save calibration to parameter registry
    if (cellNumber == 2) {
        ParameterRegistry::getInstance().setParameterValue("hardware.sensors.scale.calibration2", calibration);
    }
    else {
        ParameterRegistry::getInstance().setParameterValue("hardware.sensors.scale.calibration", calibration);
    }

    msg = langstring_calibrate_complete + String(calibration, 2) + "\n";
    displayWrappedMessage(msg);
    delay(2000);
}

inline float w1 = 0.0;
inline float w2 = 0.0;

inline void checkWeight() {
    if (!scale) {
        return;
    }

    currReadingWeight = getScaleWeight();

    if (scaleFailure) {
        return;
    }

    if (scaleCalibrationOn && !isBluetoothScale) {
        scaleCalibrate(1, PIN_HXDAT);

        // Calibrate second cell
        if (const int scaleType = config.get<int>("hardware.sensors.scale.type"); scaleType == 0) {
            scaleCalibrate(2, PIN_HXDAT2);
        }

        scaleCalibrationOn = false;
    }

    if (scaleTareOn) {
        scaleTareOn = false;
        displayWrappedMessage("Taring scale,\nremove any load!\n....", 0, 2);
        delay(2000);
        scale->tare();
        displayWrappedMessage("Taring scale,\nremove any load!\n....\ndone", 0, 2);
        delay(2000);
    }
}

inline void initScale() {
    const int scaleType = config.get<int>("hardware.sensors.scale.type");
    const int scaleSamples = config.get<int>("hardware.sensors.scale.samples");

    // Clean up existing scale
    if (scale) {
        delete scale;
        scale = nullptr;
    }

    if (scaleType == 2) { // Bluetooth scale

        const bool bleDebug = config.get<int>("system.log_level") == static_cast<int>(Logger::Level::TRACE);
        scale = new BluetoothScale(bleDebug);

        isBluetoothScale = true;

        LOG(INFO, "Initializing Bluetooth scale");

        scale->init();
    }
    else {
        // HX711 scale types
        const float cal1 = ParameterRegistry::getInstance().getParameterById("hardware.sensors.scale.calibration")->getValueAs<float>();
        const float cal2 = ParameterRegistry::getInstance().getParameterById("hardware.sensors.scale.calibration2")->getValueAs<float>();

        if (scaleType == 0) { // Dual load cell
            scale = new HX711Scale(PIN_HXDAT, PIN_HXDAT2, PIN_HXCLK, cal1, cal2);
        }
        else {                // Single load cell
            scale = new HX711Scale(PIN_HXDAT, PIN_HXCLK, cal1);
        }

        isBluetoothScale = false;
        LOG(INFO, "Initializing HX711 scale");

        if (!scale->init()) {
            LOG(ERROR, "Scale initialization failed");
            displayScaleFailed();
            delay(5000);
            scaleFailure = true;
            delete scale;
            scale = nullptr;
            return;
        }

        // Set samples for HX711 scales
        scale->setSamples(scaleSamples);
    }

    // Reset connection state
    scaleConnectionLost = false;
    scaleFailure = false;
    brewByWeightFallbackActive = false;
    lastScaleConnectionCheck = 0;
    scaleConnectionFailureTime = 0;
    lastValidWeight = 0;

    scaleCalibrationOn = false;

    LOG(INFO, "Scale initialized successfully");
}

/**
 * @brief Scale with shot timer and connection handling
 */
/**
 * @brief Scale with shot timer and connection handling
 */
inline void shotTimerScale() {
    switch (shottimerCounter) {
        case 10: // waiting step for brew switch turning on
            if (currBrewState != kBrewIdle) {
                // For Bluetooth scales with auto-tare, wait a bit before capturing pre-brew weight
                if (isBluetoothScale && autoTareInProgress) {
                    // Wait at least 2 seconds for Bluetooth tare to complete
                    if (millis() - autoTareStartTime < 2000) {
                        break;
                    }

                    autoTareInProgress = false;
                }

                preBrewWeight = currReadingWeight;
                shottimerCounter = 20;

                // Reset fallback state at start of new brew
                brewByWeightFallbackActive = false;
            }
            break;

        case 20:
            currBrewWeight = currReadingWeight - preBrewWeight;

            if (currBrewState == kBrewIdle) {
                shottimerCounter = 10;

                // Reset fallback state when brew ends
                brewByWeightFallbackActive = false;
            }
            break;

        default:;
    }
}

/**
 * @brief Get scale connection status for display
 */
inline bool getScaleConnectionStatus() {
    if (!isBluetoothScale || !scale) {
        return true; // Not applicable for HX711 scales
    }

    return scale->isConnected();
}

/**
 * @brief Check if scale is in fallback mode
 */
inline bool isScaleInFallbackMode() {
    return brewByWeightFallbackActive;
}

/**
 * @brief Check if Bluetooth scale is currently trying to connect
 */
inline bool isBluetoothScaleConnecting() {
    if (!isBluetoothScale || !scale) {
        return false;
    }

    return static_cast<BluetoothScale*>(scale)->isConnecting();
}
