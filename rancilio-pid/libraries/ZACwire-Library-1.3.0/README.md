# ZACwire™ Library to read TSic sensors
[![Only 32 Kb](https://badge-size.herokuapp.com/lebuni/ZACwire-Library/master/ZACwire.h)](https://github.com/lebuni/ZACwire-Library/blob/master/ZACwire.h) 
[![GitHub issues](https://img.shields.io/github/issues/lebuni/ZACwire-Library.svg)](https://github.com/lebuni/ZACwire-Library/issues/) 
[![GitHub license](https://img.shields.io/github/license/lebuni/ZACwire-Library.svg)](https://github.com/lebuni/ZACwire-Library/blob/master/LICENSE)


Arduino Library to read the ZACwire protocol, wich is used by TSic temperature sensors 206, 306 and 506 on their signal pin.

`ZACwire<int pin> obj(int Sensor)` tells the library which input pin of the controller (eg. 2) and type of sensor (eg. 306) it should use. Please pay attention that the selected pin supports external interrupts!

`.begin()` returns true if a signal is detected on the specific pin and starts the reading via ISRs. It should be started at least 120ms before the first .getTemp().

`.getTemp()` returns the temperature in °C and gets usually updated every 100ms. In case of a failed reading, it returns `222`. In case of no incoming signal it returns `221`.

`.end()` stops the reading for time sensititive tasks, which shouldn't be interrupted.


## Benefits compared to former TSic libraries
- saves a lot of controller time, because no delay() is used and calculations are done by bit manipulation
- low memory consumption
- misreading rate lower than 0.001%
- reading an unlimited number of TSic simultaneously
- higher accuracy (0.1°C offset corrected)
- simple use






## Example
```c++

#include <ZACwire.h>

ZACwire<2> Sensor(306);		// set pin "2" to receive signal from the TSic "306"

void setup() {
  Serial.begin(500000);
  
  if (Sensor.begin() == true) {     //check if a sensor is connected to the pin
    Serial.println("Signal found on pin 2");
  }
  delay(120);
}

void loop() {
  float Input = Sensor.getTemp();     //get the Temperature in °C
  
  if (Input == 222) {
    Serial.println("Reading failed");
  }
  
  else if (Input == 221) {
    Serial.println("Sensor not connected");
  }
  
  else {
    Serial.print("Temp: ");
    Serial.println(Input);
  }
  delay(100);
}
```






## Fine-Tuning
In case of failed readings, there might be some fine-tuning necessary.

```c++
ZACwire<int pin> obj(int Sensor, byte defaultBitWindow, bool core)
```

`byte defaultBitWindow` is the expected BitWindow in µs. According to the datasheet it should be around 125µs, but it varies with temperature.
Change this, if the **first few readings** of the sensor fail (t = 222°C).

`bool core` can only be used on a dual core ESP32. You can decide on which core the ISR should run, default is Core1. Using Core0 might cause some corrupted readings (up to 0.1%), but can be the better option if Core1 is very busy.
 
If `.getTemp()` gives you **221** as an output, the library detected an unusual long period without new signals. Please check your cables or try using the RC filter, that is mentioned in the datasheet of the TSic.
