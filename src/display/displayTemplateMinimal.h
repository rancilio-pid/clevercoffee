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
    
    Viewport temp = display.getView(Area::Temperature);
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

    char brewString[20];
    if (isBrewDetected == 1 && currBrewState == kBrewIdle) {
        const char* format = "BD: %.1f/%.0f";
        snprintf(brewString, sizeof(brewString), format, ((millis() - timeBrewDetection) / 1000), brewtimesoftware);
        display.printCentered(Area::BrewTime, (char*)brewString);
    }
    else {
        const char* format = "%s%.0f/%.0f";
        if (FEATURE_BREWCONTROL == 0) {
            snprintf(brewString, sizeof(brewString), format, langstring_brew, (timeBrewed / 1000), brewtimesoftware);
        }
        else {
            snprintf(brewString, sizeof(brewString), format, langstring_brew, (timeBrewed / 1000), (totalBrewTime / 1000));
        }
        display.printCentered(Area::BrewTime, (char*)brewString);
    }

    // Show heater output in %
    Viewport pg = display.getView(Area::Progressbar);
    displayProgressbar(pidOutput / 10, pg.getUpperLeft().X + 15, pg.getUpperLeft().Y, 100);

    display.sendBuffer();
}
