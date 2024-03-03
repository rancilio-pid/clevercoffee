/**
 * @file RGBLEDstrip.cpp
 * 
 * @brief A strip with adressable LEDs connected to a GPIO pin
 */

#include "RGBLEDstrip.h"
#include "chipsets.h"

RGBLEDstrip::RGBLEDstrip(int numLEDs, Type type) 
    : ledType(type),
      numLEDs(numLEDs), 
      numAssignedLEDs(0) {

    leds = new CRGB[numLEDs];

    switch(type) {
        case LED::WS2811: 
            FastLED.addLeds<::WS2811, PIN_STATUSLED>(leds, numLEDs);  
            break;

        case LED::WS2812: 
            FastLED.addLeds<::WS2812, PIN_STATUSLED>(leds, numLEDs);             
            break;

        case LED::WS2812B: 
            FastLED.addLeds<::WS2812B, PIN_STATUSLED>(leds, numLEDs);
            break;
    }

    turnOff();
}

RGBLEDstrip::~RGBLEDstrip() {
    delete[] leds;
}

void RGBLEDstrip::turnOn() {
    setBrightness(255); 
}

void RGBLEDstrip::turnOff() {
    setBrightness(0); 
}

void RGBLEDstrip::setColor(int hue, int saturation, int value) {
    setLEDRangeColor(0, numLEDs-1, hue, saturation, value);
}

void RGBLEDstrip::setBrightness(int value) {
    setColor(0, 0, value);
}

void RGBLEDstrip::setLEDindexColor(int index, int hue, int saturation, int value) {
    setLEDRangeColor(index, index + 1, hue, saturation, value);
}

void RGBLEDstrip::prepareLEDRangeColor(int start, int endPlusOne, int hue, int saturation, int value) {
    if(start < 0 || endPlusOne > numLEDs) 
        return; 
    
    for(int i = start; i < endPlusOne; i++) { 
        leds[i].setHSV(hue, saturation, value);
    };    
}

void RGBLEDstrip::setLEDRangeColor(int start, int endPlusOne, int hue, int saturation, int value) {
    prepareLEDRangeColor(start, endPlusOne, hue, saturation, value);
    FastLED.show();
}

void RGBLEDstrip::setLEDRangeRainbow(int start, int endPlusOne, int initialHue, int deltaHue) {
    if(start < 0 || endPlusOne > numLEDs) return; 
    CHSV hsv;
    hsv.hue = initialHue;
    hsv.val = 255;
    hsv.sat = 240;
    for( int i = 0; i < endPlusOne; ++i) {
        leds[i] = hsv;
        hsv.hue += deltaHue;
    }
    FastLED.show();
}

void RGBLEDstrip::setLEDRangeColorTwoValues(int start, int endPlusOne, int hue, int saturation, int value1, int value2) {
    if(start < 0 || endPlusOne > numLEDs) return; 
    int numFirstHalf = ( endPlusOne - start ) / 2;
    int idSecondHalf = start + numFirstHalf;
    prepareLEDRangeColor(start, idSecondHalf, hue, saturation, value1);
    prepareLEDRangeColor(idSecondHalf, endPlusOne, hue, saturation, value2);
    FastLED.show();
}

int RGBLEDstrip::assignLED() {
    numAssignedLEDs++;
    return numAssignedLEDs-1;
}

int RGBLEDstrip::getNumAssignedLEDs() {
    return numAssignedLEDs;
}

int RGBLEDstrip::getNumLEDs() {
    return numLEDs;
}