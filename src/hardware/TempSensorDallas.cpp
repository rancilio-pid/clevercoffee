/**
 * @file TempSensorDallas.cpp
 *
 * @brief Handler for Dallas DS18B20 temperature sensor
 */

#include "TempSensorDallas.h"

TempSensorDallas::TempSensorDallas(int GPIOPin) {
    oneWire_ = new OneWire(GPIOPin);
    dallasSensor_ = new DallasTemperature(oneWire_);
    dallasSensor_->begin();
    dallasSensor_->getAddress(sensorDeviceAddress_, 0);
    dallasSensor_->setResolution(sensorDeviceAddress_, 10);

    // Request first temperature conversion directly:
    dallasSensor_->requestTemperaturesByAddress(sensorDeviceAddress_);
}

bool TempSensorDallas::sample_temperature(double& temperature) const {

    // Read temperature from device
    auto temp = dallasSensor_->getTempC(sensorDeviceAddress_);

    // Check error codes:
    if (temp == DEVICE_DISCONNECTED_C) {
        LOG(WARNING, "Temperature sensor not connected");
        return false;
    }
    else if (temp == DEVICE_FAULT_OPEN_C || temp == DEVICE_FAULT_SHORTGND_C || temp == DEVICE_FAULT_SHORTVDD_C) {
        LOG(WARNING, "Issue with temperature sensor connection, check wiring");
        return false;
    }

    // All fine, return temp:
    temperature = temp;

    // Request next temperature conversion from device, to be read the next time
    // For 10-bit resolution the conversion takes around 188ms and our temperature reading timer clocks at 400ms - checks out!
    dallasSensor_->requestTemperaturesByAddress(sensorDeviceAddress_);

    return true;
}
