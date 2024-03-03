/**
 * @file RgbSingleLED.h
 * 
 * @brief Single adressable LED, like status or brew LED
 */

#pragma once

#include "RGBLEDstrip.h"

class RgbSingleLED : public LED {
public:
    // Constructor: Accepts a reference to an existing RGBLED object.
    RgbSingleLED(RGBLEDstrip& ledStrip);

    void turnOn() override;
    void turnOff() override;
    void setColor(int hue, int saturation, int value) override;
    void setBrightness(int value) override;

private:
    RGBLEDstrip& ledStrip; // Reference to the RGBLED object.
    int LEDid; 
};
