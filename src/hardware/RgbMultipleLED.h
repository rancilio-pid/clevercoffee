/**
 * @file RgbMultipleLED.h
 * 
 * @brief Subgroup of adressable LEDs on the strip (all LEDs which are not the status or brew LED).
 */

#pragma once

#include "RGBLEDstrip.h"

class RgbMultipleLED : public LED {
public:
    RgbMultipleLED(RGBLEDstrip& ledStrip);

    void turnOn() override;
    void turnOff() override;
    void setColor(int hue, int saturation, int value) override;
    void setBrightness(int value) override;
    void setRainbow(int initialHue, int deltaHu);
    void setColorTwoValues(int hue, int saturation, int value1, int value2);

private:
    RGBLEDstrip& ledStrip; // Reference to the RGBLED object.
    int startLEDid; 
    int numLEDs;
};
