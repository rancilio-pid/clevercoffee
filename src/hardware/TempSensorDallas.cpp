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
}

float TempSensorDallas::getTemperatureCelsius() const {
    dallasSensor_->requestTemperatures();
    return dallasSensor_->getTempCByIndex(0);
}
