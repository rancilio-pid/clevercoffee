/**
 *  test ard-tsiclib by reading the temperature every second
 *  and output the measured data to the serial port
 */

#include "TSIC.h"       // include the library

// instantiate the library, representing the sensor
TSIC Sensor1(4, 2);    // Signalpin, VCCpin, Sensor Type
//TSIC Sensor1(4);    // only Signalpin, VCCpin unused by default
//TSIC Sensor1(4, NO_VCC_PIN, TSIC_50x);    // external powering of 50x-Sensor
//TSIC Sensor1(4, 2, TSIC_30x);    // Signalpin, VCCpin, Sensor Type
//TSIC Sensor2(5, 2, TSIC_50x);  // Signalpin, VCCpin, Sensor Type NOTE: we can use the same VCCpin to power several sensors

uint16_t temperature = 0;
float Temperatur_C = 0;

void setup() {
    Serial.begin(9600); // set up the serial port
}

void loop() {
  
  if (Sensor1.getTemperature(&temperature)) {
    Serial.print("uint_16: ");
    Serial.println(temperature);
    Temperatur_C = Sensor1.calc_Celsius(&temperature);
    Serial.print("Temperature: ");
    Serial.print(Temperatur_C);
    Serial.println(" °C");
  }
  
  delay(1000);    // wait 1 seconds
}
