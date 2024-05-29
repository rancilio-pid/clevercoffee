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
    u8g2.clearBuffer();

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

    u8g2.setFont(u8g2_font_profont11_tf);

    u8g2.setCursor(32, 16);
    u8g2.print("T: ");
    u8g2.print(temperature, 1);

    u8g2.print("/");
    u8g2.print(setpoint, 1);

    u8g2.setCursor(32, 26);
    u8g2.print("W: ");

    if (scaleFailure) {
        u8g2.print("fault");
    }
    else {
        if (machineState == kBrew) {
            u8g2.print(weightBrew, 0);
        }
        else {
            u8g2.print(weight, 0);
        }

        if (weightSetpoint > 0) {
            u8g2.print("/");
            u8g2.print(weightSetpoint, 0);
        }

        u8g2.print(" (");
        u8g2.print(weightBrew, 1);
        u8g2.print(")");
    }

    // Brew
    u8g2.setCursor(32, 36);
    u8g2.print("t: ");
    u8g2.print(timeBrewed / 1000, 0);

    if (FEATURE_BREWCONTROL == 0) {
        u8g2.print("/");
        u8g2.print(brewtimesoftware, 0);
    }
    else {
        if (brewTime > 0) {
            u8g2.print("/");
            u8g2.print(totalBrewTime / 1000, 0);
        }

        u8g2.print(" (");
        u8g2.print(lastBrewTime / 1000, 1);
        u8g2.print(")");
    }

#if (FEATURE_PRESSURESENSOR == 1)
    u8g2.setCursor(32, 46);
    u8g2.print("P: ");
    u8g2.print(inputPressure, 1);
#endif

    // Show heater output in %
    displayProgressbar(pidOutput / 10, 30, 60, 98);

    u8g2.sendBuffer();
}
