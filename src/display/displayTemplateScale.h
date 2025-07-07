/**
 * @file displayTemplateScale.h
 *
 * @brief Display template with brew scale
 *
 */

#pragma once

/**
 * @brief Send data to display
 */
inline void printScreen() {

    // Show fullscreen brew timer:
    if (displayFullscreenBrewTimer()) {
        // Display was updated, end here
        return;
    }

    // Show fullscreen manual flush timer:
    if (displayFullscreenManualFlushTimer()) {
        // Display was updated, end here
        return;
    }

    // Show fullscreen hot water timer:
    if (displayFullscreenHotWaterTimer()) {
        // Display was updated, end here
        return;
    }

    // Print the machine state
    if (displayMachineState()) {
        // Display was updated, end here
        return;
    }

    // If no specific machine state was printed, print default:
    u8g2->clearBuffer();

    displayStatusbar();

    displayThermometerOutline(4, 62);

    // Draw current temp in thermometer
    if (fabs(temperature - setpoint) < 0.3) {
        if (isrCounter < 500) {
            drawTemperaturebar(8, 30);
        }
    }
    else {
        drawTemperaturebar(8, 30);
    }

    // Draw current temp and temp setpoint
    u8g2->setFont(u8g2_font_profont11_tf);

    u8g2->setCursor(32, 16);
    u8g2->print("T: ");
    u8g2->print(temperature, 1);
    u8g2->print("/");
    u8g2->print(setpoint, 1);
    u8g2->print(static_cast<char>(176));
    u8g2->print("C");

    const bool scaleEnabled = config.get<bool>("hardware.sensors.scale.enabled");

    if (scaleEnabled) {
        // Show current weight if scale has no error
        displayBrewWeight(32, 26, currReadingWeight, -1, scaleFailure);
    }

    if (config.get<bool>("hardware.switches.brew.enabled")) {
        // Show flush time
        if (machineState == kManualFlush) {
            displayBrewTime(32, 36, langstring_manual_flush, currBrewTime);
        }
        // Show hot water time
        else if (machineState == kHotWater) {
            displayBrewTime(32, 36, langstring_hot_water, currPumpOnTime);
        }
        else if (shouldDisplayBrewTimer()) {
            const bool automaticBrewingEnabled = config.get<bool>("brew.mode") == 1;

            // Time
            if (automaticBrewingEnabled && config.get<bool>("brew.by_time")) {
                displayBrewTime(32, 36, langstring_brew, currBrewTime, totalTargetBrewTime);
            }
            else {
                displayBrewTime(32, 36, langstring_brew, currBrewTime);
            }

            // Weight
            if (scaleEnabled) {
                if (automaticBrewingEnabled && config.get<bool>("brew.by_weight")) {
                    displayBrewWeight(32, 26, currBrewWeight, targetBrewWeight, scaleFailure);
                }
                else {
                    displayBrewWeight(32, 26, currBrewWeight, -1, scaleFailure);
                }
            }
        }
    }

    if (config.get<bool>("hardware.sensors.pressure.enabled")) {
        u8g2->setCursor(32, 46);
        u8g2->print(langstring_pressure);
        u8g2->print(inputPressure, 1);
    }

    // Show heater output in %
    displayProgressbar(pidOutput / 10, 30, 60, 98);

    displayBufferReady = true;
}
