/**
 * @file displayTemplateMinimal.h
 *
 * @brief Minimal display template
 *
 */

#pragma once

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

    displayStatusbar();

    int numDecimalsInput = 1;

    if (temperature > 99.999) {
        numDecimalsInput = 0;
    }

    int numDecimalsSetpoint = 1;

    if (setpoint > 99.999) {
        numDecimalsSetpoint = 0;
    }

    // Draw temp, blink if feature STATUS_LED is not enabled
    if ((fabs(temperature - setpoint) < 0.3) && !FEATURE_STATUS_LED) {
        if (isrCounter < 500) {
            // limit to 4 characters
            display.setCursor(2, 20);
            display.setFont(FontType::Big);
            display.print(temperature, numDecimalsInput);
            display.setFont(FontType::OpenIconicArrow2x);
            display.print(char(78));
            display.setCursor(78, 20);
            display.setFont(FontType::Big);
            display.print(setpoint, numDecimalsSetpoint);
        }
    }
    else {
        display.setCursor(2, 20);
        display.setFont(FontType::Big);
        display.print(temperature, numDecimalsInput);
        display.setFont(FontType::OpenIconicArrow2x);
        display.setCursor(56, 24);

        if (bPID.GetMode() == 1) {
            display.print(char(74));
        }
        else {
            display.print(char(70));
        }

        display.setCursor(79, 20);
        display.setFont(FontType::Big);
        display.print(setpoint, numDecimalsSetpoint);
    }

    display.setFont(FontType::Normal);

    if (isBrewDetected == 1 && currBrewState == kBrewIdle) {
        display.setCursor(38, 44);
        display.print("BD: ");
        display.print((millis() - timeBrewDetection) / 1000, 1);
        display.print("/");
        display.print(brewtimesoftware, 0);
    }
    else {
        display.setCursor(34, 44);
        display.print(langstring_brew);
        display.print(timeBrewed / 1000, 0);
        display.print("/");

        if (FEATURE_BREWCONTROL == 0) {
            display.print(brewtimesoftware, 0);
        }
        else {
            display.print(totalBrewTime / 1000, 0);
        }
    }

    // Show heater output in %
    displayProgressbar(pidOutput / 10, 15, 60, 100);

    display.sendBuffer();
}
