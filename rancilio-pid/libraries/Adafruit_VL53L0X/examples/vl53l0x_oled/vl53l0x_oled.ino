/* This example shows how to take
range measurements with the VL53L0X and display on a SSD1306 OLED.

The range readings are in units of mm. */

#include <Wire.h>
#include "Adafruit_VL53L0X.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display = Adafruit_SSD1306();

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

#if (SSD1306_LCDHEIGHT != 32)
 #error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup()
{
  Serial.begin(9600);
    
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done
  display.display();
  delay(1000);
    
  
  Wire.begin();

  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }

  // text display big!
  display.setTextSize(4);
  display.setTextColor(WHITE);
}

void loop()
{
  VL53L0X_RangingMeasurementData_t measure;

  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
      display.clearDisplay();
      display.setCursor(0,0);
      display.print(measure.RangeMilliMeter);
      display.print("mm");
      display.display();
      Serial.println();
      delay(50);
  } else {
    display.display();
    display.clearDisplay();
    return;
  }
}