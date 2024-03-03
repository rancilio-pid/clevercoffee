/**
 * @file RgbMultipleLED.cpp
 * 
 * @brief Subgroup of adressable LEDs on the strip (all LEDs which are not the status or brew LED).
 * 
 * @param ledStrip The constructor saves the reference to the RGBLED object.
 */

#include "RgbMultipleLED.h"

RgbMultipleLED::RgbMultipleLED(RGBLEDstrip& ledStrip) 
    : ledStrip(ledStrip),
      startLEDid (0),
      numLEDs (0) {
    startLEDid = ledStrip.getNumAssignedLEDs(); 
    numLEDs = ledStrip.getNumLEDs(); 
}

void RgbMultipleLED::turnOn() {
    setBrightness(255); 
}

void RgbMultipleLED::turnOff() {
    setBrightness(0); 
}

void RgbMultipleLED::setBrightness(int value) {
    setColor(0, 0, value); 
}

void RgbMultipleLED::setColor(int hue, int saturation, int value) {
    ledStrip.setLEDRangeColor(startLEDid, numLEDs, hue, saturation, value);
}

void RgbMultipleLED::setRainbow(int initialHue, int deltaHue) {
    ledStrip.setLEDRangeRainbow(startLEDid, numLEDs, initialHue, deltaHue);
}

void RgbMultipleLED::setColorTwoValues(int hue, int saturation, int value1, int value2) {
    ledStrip.setLEDRangeColorTwoValues(startLEDid, numLEDs, hue, saturation, value1, value2);
}
