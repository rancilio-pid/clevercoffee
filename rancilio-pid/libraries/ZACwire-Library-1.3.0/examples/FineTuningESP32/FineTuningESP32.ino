#include <ZACwire.h>

ZACwire<14> Sensor(206,130,0);		// set pin "14" to receive signal from the TSic "206" with an expected bitWindow of "130µs". ISR executed on CPU1

void setup() {
  Serial.begin(500000);
  
  if (Sensor.begin() == true) {     //check if a sensor is connected to the pin
    Serial.println("Signal found on pin 14");
  }
  else Serial.println("No Signal");
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
