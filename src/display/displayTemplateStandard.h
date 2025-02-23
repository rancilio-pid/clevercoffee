/**
 * @file displayTemplateStandard.h
 *
 * @brief Standard display template
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
    display.setFont(FontType::Normal); // set font

    displayStatusbar();

    display.setCursor(35, 16);
    display.print(langstring_current_temp);
    display.setCursor(84, 16);
    display.print(temperature, 1);
    display.setCursor(114, 16);
    display.print((char)176);
    display.print("C");
    display.setCursor(35, 26);
    display.print(langstring_set_temp);
    display.setCursor(84, 26);
    display.print(setpoint, 1);
    display.setCursor(114, 26);
    display.print((char)176);
    display.print("C");

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

    // Brew time
    display.setCursor(35, 36);

    // Shot timer shown if machine is brewing and after the brew
    if (machineState == kBrew || machineState == kShotTimerAfterBrew) {
        display.print(langstring_brew);
        display.setCursor(84, 36);
        display.print(timeBrewed / 1000, 0);
        display.print("/");

        if (FEATURE_BREWCONTROL == 0) {
            display.print(brewtimesoftware, 0);
        }
        else {
            display.print(totalBrewTime / 1000, 1);
        }
    }

    // PID values over heat bar
    display.setCursor(38, 47);

    display.print(bPID.GetKp(), 0);
    display.print("|");

    if (bPID.GetKi() != 0) {
        display.print(bPID.GetKp() / bPID.GetKi(), 0);
    }
    else {
        display.print("0");
    }

    display.print("|");
    display.print(bPID.GetKd() / bPID.GetKp(), 0);
    display.setCursor(96, 47);

    if (pidOutput < 99) {
        display.print(pidOutput / 10, 1);
    }
    else {
        display.print(pidOutput / 10, 0);
    }

    display.print("%");

    // Show heater output in %
    displayProgressbar(pidOutput / 10, 30, 60, 98);

    display.sendBuffer();
}
