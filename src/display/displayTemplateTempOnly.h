/**
 * @file displayTemplateTempOnly.h
 *
 * @brief Temp-only display template
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

    bool nearSetpoint = fabs(temperature - setpoint) <= config.get<float>("display.blinking.delta");

    if (!(isrCounter < 500 && ((nearSetpoint && config.get<int>("display.blinking.mode") == 1) || (!nearSetpoint && config.get<int>("display.blinking.mode") == 2)))) {
        u8g2->setFont(u8g2_font_fub35_tn);
        u8g2->drawCircle(116, 27, 4);

        if (temperature < 99.95) {
            u8g2->setCursor(8, 22);
            u8g2->print(temperature, 1);
        }
        else {
            u8g2->setCursor(24, 22);
            u8g2->print(temperature, 0);
        }
    }

    displayStatusbar();

    displayBufferReady = true;
}
