/**
 * @file CurrentSensorAnalog.cpp
 *
 * @brief Handler for analog current sensor
 */

#include "CurrentSensorAnalog.h"
#include "Logger.h"
#include "GPIOPin.h"

#define CONVERSION_FACTOR 66 // in mV/A
#define OFFSET 2.5 // in V
#define ADC_RESOLUTION 1024
#define SUPPLYVOLTAGE 5.0 //in V

CurrentSensorAnalog::CurrentSensorAnalog(int pinNumber):sensorPin_(pinNumber,GPIOPin::IN_ANALOG) {
   // sensorPin_ = GPIOPin(pinNumber,GPIOPin::IN_ANALOG);
}
/**
 * @brief Samples the current from the sensor
 * @details Reads the voltage of the analog input pin and converts it to the current value
 * @param current A reference to a double where the sampled current value will be stored.
 * @return Boolean indicating whether the reading has been successful
 */
bool CurrentSensorAnalog::sample_current(double& current) const {
   
    double sensorValue = (double)sensorPin_.read()*(SUPPLYVOLTAGE/ADC_RESOLUTION); //in V
    current =  (1000/CONVERSION_FACTOR) * (sensorValue - OFFSET); //in Ampere

    if (sensorValue < OFFSET) {
        LOG(WARNING, "Sensor not connected or in wrong direction. ");
        return false;
    }

    return true;
}
