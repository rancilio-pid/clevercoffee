/**
 * @file RgbSingleLED.cpp
 * 
 * @brief Single adressable LED, like status or brew LED
 * 
 * @param ledStrip The constructor saves the reference to the RGBLED object.
 */

#include "RgbSingleLED.h"

RgbSingleLED::RgbSingleLED(RGBLEDstrip& ledStrip) 
    : ledStrip(ledStrip),
      LEDid (0) {
    LEDid = ledStrip.assignLED(); 
}

void RgbSingleLED::turnOn() {
    // Set a single LED to 100% white.
    setBrightness(255); 
}

void RgbSingleLED::turnOff() {
    // Turn off a single LED to indicate "off" status.
    setBrightness(0); 
}

void RgbSingleLED::setColor(int hue, int saturation, int value) {
    ledStrip.setLEDindexColor(LEDid, hue, saturation, value); 
}

void RgbSingleLED::setBrightness(int value) {
    setColor(0, 0, value); 
}
