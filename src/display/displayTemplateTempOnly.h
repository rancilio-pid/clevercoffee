/**
 * @file displayTemplateTempOnly.h
 *
 * @brief Temp-only display template
 *
 */

#pragma once

#include "DisplayManager.h"
extern DisplayManager display;  // declare the extern DisplayManager object to use the same instance everywhere

// Define some Displayoptions
int blinkingtemp = 1;           // 0: blinking near setpoint, 1: blinking far away from setpoint
float blinkingtempoffset = 0.3; // offset for blinking

#include "displayCommon.h"

/**
 * @brief Send data to display
 */
void printScreen() {

    // Show shot timer:
    if (displayShottimer()) {
        // Display was updated, end here
        return;
    }

    // Print the machine state
    if (displayMachineState()) {
        // Display was updated, end here
        return;
    }

    // If no specific machine state was printed, print default:
    display.clearBuffer();

    // draw (blinking) temp
    if (((fabs(temperature - setpoint) < blinkingtempoffset && blinkingtemp == 0) || (fabs(temperature - setpoint) >= blinkingtempoffset && blinkingtemp == 1)) && !FEATURE_STATUS_LED) {
        if (isrCounter < 500) {
            if (temperature < 99.999) {
                display.setCursor(8, 22);
                display.setFont(FontType::fup35);
                display.print(temperature, 1);
                display.drawCircle(116, 27, 4);
            }
            else {
                display.setCursor(24, 22);
                display.setFont(FontType::fup35);
                display.print(temperature, 0);
                display.drawCircle(116, 27, 4);
            }
        }
    }
    else {
        if (temperature < 99.999) {
            display.setCursor(8, 22);
            display.setFont(FontType::fup35);
            display.print(temperature, 1);
            display.drawCircle(116, 27, 4);
        }
        else {
            display.setCursor(24, 22);
            display.setFont(FontType::fup35);
            display.print(temperature, 0);
            display.drawCircle(116, 27, 4);
        }
    }

    displayStatusbar();

    display.sendBuffer();
}
