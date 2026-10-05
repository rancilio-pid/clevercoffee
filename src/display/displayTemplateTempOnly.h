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

    // Print the machine state
    if (displayMachineState()) {
        // Display was updated, end here
        return;
    }

    // If no specific machine state was printed, print default:
    u8g2->clearBuffer();

    bool nearSetpoint = fabs(temperature - setpoint) <= config.get<float>("display.blinking.delta");

    if (!(isrCounter < 500 && ((nearSetpoint && config.get<int>("display.blinking.mode") == 1) || (!nearSetpoint && config.get<int>("display.blinking.mode") == 2)))) {
        u8g2->setFont(custom_helvB24);

        const int decimals = (temperature >= 99.95) ? 0 : 1;
        const int startX = decimals ? 26 : 30;

        u8g2->setCursor(startX, 24);
        u8g2->print(temperature, decimals);
        u8g2->print(static_cast<char>(176));
    }

    displayStatusbar();

    displayBufferReady = true;
}
