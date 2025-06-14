/**
 * @file displayTemplateTempOnly.h
 *
 * @brief Temp-only display template
 *
 */

#pragma once

// Define some Displayoptions
int blinkingtemp = 1;           // 0: blinking near setpoint, 1: blinking far away from setpoint
float blinkingtempoffset = 0.3; // offset for blinking

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

    // Print the machine state
    if (displayMachineState()) {
        // Display was updated, end here
        return;
    }

    // If no specific machine state was printed, print default:
    u8g2.clearBuffer();

    // draw (blinking) temp
    if (((fabs(temperature - setpoint) < blinkingtempoffset && blinkingtemp == 0) || (fabs(temperature - setpoint) >= blinkingtempoffset && blinkingtemp == 1)) && !config.getStatusLedEnabled()) {
        if (isrCounter < 500) {
            if (temperature < 99.999) {
                u8g2.setCursor(8, 22);
                u8g2.setFont(u8g2_font_fub35_tf);
                u8g2.print(temperature, 1);
                u8g2.drawCircle(116, 27, 4);
            }
            else {
                u8g2.setCursor(24, 22);
                u8g2.setFont(u8g2_font_fub35_tf);
                u8g2.print(temperature, 0);
                u8g2.drawCircle(116, 27, 4);
            }
        }
    }
    else {
        if (temperature < 99.999) {
            u8g2.setCursor(8, 22);
            u8g2.setFont(u8g2_font_fub35_tf);
            u8g2.print(temperature, 1);
            u8g2.drawCircle(116, 27, 4);
        }
        else {
            u8g2.setCursor(24, 22);
            u8g2.setFont(u8g2_font_fub35_tf);
            u8g2.print(temperature, 0);
            u8g2.drawCircle(116, 27, 4);
        }
    }

    displayStatusbar();

    u8g2.sendBuffer();
}
