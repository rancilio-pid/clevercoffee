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
void printScreen() {

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

    // If no specific machine state was printed, print default:
    u8g2.clearBuffer();

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
            u8g2.setCursor(2, 20);
            u8g2.setFont(u8g2_font_profont22_tf);
            u8g2.print(temperature, numDecimalsInput);
            u8g2.setFont(u8g2_font_open_iconic_arrow_2x_t);
            u8g2.print(char(78));
            u8g2.setCursor(78, 20);
            u8g2.setFont(u8g2_font_profont22_tf);
            u8g2.print(setpoint, numDecimalsSetpoint);
        }
    }
    else {
        u8g2.setCursor(2, 20);
        u8g2.setFont(u8g2_font_profont22_tf);
        u8g2.print(temperature, numDecimalsInput);
        u8g2.setFont(u8g2_font_open_iconic_arrow_2x_t);
        u8g2.setCursor(56, 24);

        if (bPID.GetMode() == 1) {
            u8g2.print(char(74));
        }
        else {
            u8g2.print(char(70));
        }

        u8g2.setCursor(79, 20);
        u8g2.setFont(u8g2_font_profont22_tf);
        u8g2.print(setpoint, numDecimalsSetpoint);
    }

    u8g2.setFont(u8g2_font_profont11_tf);

// Brew time
#if (FEATURE_BREWSWITCH == 1)
    if (featureBrewControl) {
        // Shown brew time
        if (shouldDisplayBrewTimer()) {
            u8g2.setCursor(34, 44);
            u8g2.print(langstring_brew);
            u8g2.print(currBrewTime / 1000, 0);
            u8g2.print("/");
            u8g2.print(totalTargetBrewTime / 1000, 0);
        }

        // Shown flush time
        if (machineState == kManualFlush) {
            u8g2.setDrawColor(0);
            u8g2.drawBox(34, 44, 100, 15);
            u8g2.setDrawColor(1);
            u8g2.setCursor(34, 44);
            u8g2.print(langstring_manual_flush);
            u8g2.print(currBrewTime / 1000, 0);
        }
    }
    else {
        // Brew Timer with optocoupler
        if (shouldDisplayBrewTimer()) {
            u8g2.setCursor(34, 44);
            u8g2.print(langstring_brew);
            u8g2.print(currBrewTime / 1000, 0);
        }
    }
#endif

    // Show heater output in %
    displayProgressbar(pidOutput / 10, 15, 60, 100);

    u8g2.sendBuffer();
}
