/**
 * @file TempSensorTSIC.cpp
 *
 * @brief Handler for TSIC 306 temperature sensor
 */

#include "TempSensorTSIC.h"
#include "Logger.h"

#define INITIAL_CHANGERATE 200
#define RUNTIME_CHANGERATE 5

TempSensorTSIC::TempSensorTSIC(const int GPIOPin) {
    // ZACwire 2.0.0 has uninitialized member variables (like backUP) on ESP32,
    // which causes its ISR to write out of bounds and corrupt the heap (e.g. mDNS).
    // Allocate zeroed memory to prevent this buffer overflow.
    void* mem = calloc(1, sizeof(ZACwire));
    tsicSensor_ = new (mem) ZACwire(GPIOPin, 306);
    // Start sampling the TSic sensor
    tsicSensor_->begin();
}

bool TempSensorTSIC::sample_temperature(double& temperature) const {
    static bool validTemps = false;
    float temp = 0.0;

    if (!validTemps) {
        temp = tsicSensor_->getTemp(INITIAL_CHANGERATE);

        // if current and previous reading is in range and their difference is low then reduce changerate
        if (temp > 0.0 && temp < 180.0) {
            if (temperature > 0.0 && temperature < 180.0 && abs(temperature - temp) < RUNTIME_CHANGERATE) {
                validTemps = true;
            }
            else {
                LOGF(WARNING, "Temperature not stable");
            }
        }
        else if (temp != 221 && temp != 222) {
            LOGF(WARNING, "Temperature reading not within 0 - 180°C range: %0.01f°C", temp);
        }
    }
    else {
        temp = tsicSensor_->getTemp(RUNTIME_CHANGERATE);
    }

    if (temp == 222) {
        LOG(WARNING, "Temperature reading failed");
        return false;
    }

    if (temp == 221) {
        LOG(WARNING, "Temperature sensor not connected");
        return false;
    }

    temperature = temp;

    return true;
}
