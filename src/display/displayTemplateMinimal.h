/**
 * @file displayTemplateMinimal.h
 *
 * @brief Minimal display template
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

    // If no specific machine state was printed, print default:
    u8g2->clearBuffer();

    displayStatusbar();

    int numDecimalsInput = 1;

    if (temperature >= 99.95) {
        numDecimalsInput = 0;
    }

    int numDecimalsSetpoint = 1;

    if (setpoint >= 99.95) {
        numDecimalsSetpoint = 0;
    }

    // Draw temp, blink if feature STATUS_LED is not enabled
    bool nearSetpoint = fabs(temperature - setpoint) <= config.get<float>("display.blinking.delta");

    if (!(isrCounter < 500 && ((nearSetpoint && config.get<int>("display.blinking.mode") == 1) || (!nearSetpoint && config.get<int>("display.blinking.mode") == 2)))) {
        // limit to 4 characters
        u8g2->setCursor(2, 20);
        u8g2->setFont(u8g2_font_profont22_tr);
        u8g2->print(temperature, numDecimalsInput);
        u8g2->setFont(u8g2_font_open_iconic_arrow_2x_t);
        u8g2->setCursor(56, 24);

        if (nearSetpoint) {
            u8g2->print(static_cast<char>(78));
        }
        else if (bPID.GetMode() == 1) {
            u8g2->print(static_cast<char>(74));
        }
        else {
            u8g2->print(static_cast<char>(70));
        }

        u8g2->setCursor(78, 20);
        u8g2->setFont(u8g2_font_profont22_tr);
        u8g2->print(setpoint, numDecimalsSetpoint);
    }

    u8g2->setFont(u8g2_font_profont11_tf);

    // Brew time
    if (config.get<bool>("hardware.switches.brew.enabled")) {
        // Show flush time
        if (machineState == kManualFlush) {
            u8g2->setCursor(34, 44);
            u8g2->print(langstring_manual_flush);
            u8g2->print(currBrewTime / 1000, 0);
        }
        // Show hot water time
        else if (machineState == kHotWater) {
            u8g2->setCursor(34, 44);
            u8g2->print(langstring_hot_water);
            u8g2->print(currPumpOnTime / 1000, 0);
        }
        else {
            if (shouldDisplayBrewTimer()) {
                u8g2->setCursor(34, 44);
                u8g2->print(langstring_brew);
                u8g2->print(currBrewTime / 1000, 0);

                if (config.get<bool>("brew.by_time.enabled") && config.get<int>("brew.mode") == 1) {
                    u8g2->print("/");
                    u8g2->print(totalTargetBrewTime / 1000, 0);
                }
            }
        }
    }

    // Show heater output in %
    displayProgressbar(pidOutput / 10, 15, 60, 100);

    displayBufferReady = true;
}
