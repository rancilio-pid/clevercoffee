/**
 * @file displayTemplateStandard.h
 *
 * @brief Standard display template
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
    u8g2->setFont(u8g2_font_profont11_tf); // set font

    displayStatusbar();

    u8g2->setCursor(34, 16);
    u8g2->print(langstring_current_temp);
    u8g2->setCursor(84, 16);
    u8g2->print(temperature, 1);
    u8g2->setCursor(115, 16);
    u8g2->print(static_cast<char>(176));
    u8g2->print("C");
    u8g2->setCursor(34, 26);
    u8g2->print(langstring_set_temp);
    u8g2->setCursor(84, 26);
    u8g2->print(setpoint, 1);
    u8g2->setCursor(115, 26);
    u8g2->print(static_cast<char>(176));
    u8g2->print("C");

    displayThermometerOutline(4, 62);

    // Draw current temp in thermometer
    bool nearSetpoint = fabs(temperature - setpoint) <= config.get<float>("display.blinking.delta");

    if (!(isrCounter < 500 && ((nearSetpoint && config.get<int>("display.blinking.mode") == 1) || (!nearSetpoint && config.get<int>("display.blinking.mode") == 2)))) {
        drawTemperaturebar(8, 30);
    }

    // Brew and flush time
    if (config.get<bool>("hardware.switches.brew.enabled")) {
        // Show flush time
        if (machineState == kManualFlush) {
            displayBrewTime(34, 36, langstring_manual_flush, currBrewTime);
        }
        // Show hot water time
        else if (machineState == kHotWater) {
            displayBrewTime(34, 36, langstring_hot_water, currPumpOnTime);
        }
        else {
            if (shouldDisplayBrewTimer()) {
                if (config.get<bool>("brew.by_time.enabled") && config.get<int>("brew.mode") == 1) {
                    displayBrewTime(34, 36, langstring_brew, currBrewTime, totalTargetBrewTime);
                }
                else {
                    displayBrewTime(34, 36, langstring_brew, currBrewTime);
                }
            }
        }
    }

    // PID values over heat bar
    u8g2->setCursor(38, 47);

    u8g2->print(bPID.GetKp(), 0);
    u8g2->print("|");

    if (bPID.GetKi() != 0) {
        u8g2->print(bPID.GetKp() / bPID.GetKi(), 0);
    }
    else {
        u8g2->print("0");
    }

    u8g2->print("|");
    u8g2->print(bPID.GetKd() / bPID.GetKp(), 0);
    u8g2->setCursor(96, 47);

    if (pidOutput < 99) {
        u8g2->print(pidOutput / 10, 1);
    }
    else {
        u8g2->print(pidOutput / 10, 0);
    }

    u8g2->print("%");

    // Show heater output in %
    displayProgressbar(pidOutput / 10, 30, 60, 98);

    displayBufferReady = true;
}
