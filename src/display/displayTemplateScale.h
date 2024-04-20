/**
 * @file displayTemplateScale.h
 *
 * @brief Display template with brew scale
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

    displayThermometerOutline(4, 62);

    // Draw current temp in thermometer
    if (fabs(temperature - setpoint) < 0.3) {
        if (isrCounter < 500) {
            drawTemperaturebar(8, 50, 30);
        }
    }
    else {
        drawTemperaturebar(8, 50, 30);
    }

    display.setFont(FontType::Normal);

    display.setCursor(32, 16);
    display.print("T: ");
    display.print(temperature, 1);

    display.print("/");
    display.print(setpoint, 1);

    display.setCursor(32, 26);
    display.print("W: ");

    if (scaleFailure) {
        display.print("fault");
    }
    else {
        if (machineState == kBrew) {
            display.print(weightBrew, 0);
        }
        else {
            display.print(weight, 0);
        }

        if (weightSetpoint > 0) {
            display.print("/");
            display.print(weightSetpoint, 0);
        }

        display.print(" (");
        display.print(weightBrew, 1);
        display.print(")");
    }

    // Brew
    display.setCursor(32, 36);
    display.print("t: ");
    display8g2.print(timeBrewed / 1000, 0);

    if (FEATURE_BREWCONTROL == 0) {
        u8g2.print("/");
        display.print(brewtimesoftware, 0);
    }
    else {
        if (brewTime > 0) {
            display.print("/");
            display.print(totalBrewTime / 1000, 0);
        }

        display.print(" (");
        display.print(lastBrewTime / 1000, 1);
        display.print(")");
    }

#if (FEATURE_PRESSURESENSOR == 1)
    display.setCursor(32, 46);
    display.print("P: ");
    display.print(inputPressure, 1);
#endif

    // Show heater output in %
    displayProgressbar(pidOutput / 10, 30, 60, 98);

    display.sendBuffer();
}
