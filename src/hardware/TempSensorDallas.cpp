/**
 * @file TempSensorDallas.cpp
 * 
 * @brief Handler for Dallas DS18B20 temperature sensor
 */

#include "TempSensorDallas.h"
#include <DallasTemperature.h> 

OneWire* oneWire;
DallasTemperature* dallasSensor;
DeviceAddress sensorDeviceAddress;

TempSensorDallas::TempSensorDallas(int GPIOPin) {
    oneWire = new OneWire(GPIOPin);
    dallasSensor = new DallasTemperature(oneWire);
    dallasSensor->begin();
    dallasSensor->getAddress(sensorDeviceAddress, 0);
    dallasSensor->setResolution(sensorDeviceAddress, 10);
}

float TempSensorDallas::getTempinCelsius() {
    dallasSensor->requestTemperatures();
    float temperature = dallasSensor->getTempCByIndex(0);

    return temperature;
}
