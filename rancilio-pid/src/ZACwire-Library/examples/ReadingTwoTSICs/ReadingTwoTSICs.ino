
/*
	Reads two sensors, where TSIC signal pins are connected to pin 2 and 10 on the board
 */


#include <ZACwire.h>

ZACwire<2> Sensor1(306);		// set pin "2" to receive signal from the TSIC "306"

ZACwire<10> Sensor2(506);		// set pin "10" as INPUT to receive signal from the TSIC "506"



void setup() {
  Serial.begin(500000); // set up the serial port
  if (!Sensor1.begin()) Serial.println("No digital pin with signal found for Sensor1");		//.begin() checks for signal and returns false if initializing failed
  if (!Sensor2.begin()) Serial.println("No digital pin with signal found for Sensor2");
  delay(100);
}

void loop() {
  float Input1 = Sensor1.getTemp();
  float Input2 = Sensor2.getTemp();


  Serial.print("Temp1: ");
  Serial.print(Input1);
  Serial.print(" Temp2: ");
  Serial.println(Input2);
  Serial.println("");
  
  delay(500);

}
