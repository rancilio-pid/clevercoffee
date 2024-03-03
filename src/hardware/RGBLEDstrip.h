/**
 * @file RGBLEDstrip.h
 * 
 * @brief A strip with adressable LEDs connected to a GPIO pin
 */

#pragma once

#include "LED.h"
#include "pinmapping.h"
#include <FastLED.h>

class RGBLEDstrip  : public LED {
public:
    RGBLEDstrip(int numLEDs, Type type); 
    virtual ~RGBLEDstrip();

    void turnOn() override;
    void turnOff() override;
    void setColor(int hue, int saturation, int value) override; 
    void setBrightness(int value) override;

    void setLEDindexColor(int index, int hue, int saturation, int value); // Single LED with HSV
    void setLEDRangeColor(int start, int end, int hue, int saturation, int value); // Color for a group of LEDs
    void prepareLEDRangeColor(int start, int end, int hue, int saturation, int value);
    void setLEDRangeRainbow(int start, int end, int initialHue, int deltaHue); // Fill range with rainbow colors
    void setLEDRangeColorTwoValues(int start, int end, int hue, int saturation, int value1, int value2);

    int assignLED(); // return lowest not assigned LED and count number of assigned LEDs one up
    int getNumAssignedLEDs(); // return lowest not assigned LED
    int getNumLEDs(); // return total number of LEDs in strip

private:
    CRGB* leds; // Pointer to the array of LEDs
    int numLEDs; // Number of LEDs in the strip
    int numAssignedLEDs; // Number of assigned LEDs (not available for context LED settings)
    Type ledType;
};
