/*
 *  TestScript für den TSIC Sensor
 *  Anzuschließen am PIN2
 */



#include "Arduino.h"

#include "TSIC.h"       // include the library
TSIC Sensor1(2);    // only Signalpin, VCCpin unused by default
uint16_t temperature = 0;
float Temperatur_C,Input = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // set up the serial port
}

void loop() {
  temperature = 0;
  Sensor1.getTemperature(&temperature);
  Temperatur_C = Sensor1.calc_Celsius(&temperature);
  Input = Temperatur_C;

  Serial.println("Temp :");Serial.println(Input);

}
