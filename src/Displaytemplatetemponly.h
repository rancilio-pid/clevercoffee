/**
 * @file Displaytemplatetemponly.h
 *
 * @brief Temp-only display template
 *
 */

#pragma once

// Define some Displayoptions
int blinkingtemp = 1;            // 0: blinking near setpoint, 1: blinking far away from setpoint
float blinkingtempoffset = 0.3;  // offset for blinking

/**
 * @brief Send data to display
 */
void printScreen() {
    if ((machineState == kBelowSetpoint || machineState == kPidNormal || machineState == kBrewDetectionTrailing) ||
        ((machineState == kBrew || machineState == kShotTimerAfterBrew) && SHOTTIMER == 0) ||  // shottimer == 0, auch Bezug anzeigen
        machineState == kCoolDown || ((machineState == kInit || machineState == kColdStart) && HEATINGLOGO == 0) ||
        ((machineState == kPidOffline) && OFFLINEGLOGO == 0)) {

        if (!sensorError) {
            u8g2.clearBuffer();

            // draw (blinking) temp
            if (((fabs(temperature - setpoint) < blinkingtempoffset && blinkingtemp == 0) ||
                (fabs(temperature - setpoint) >= blinkingtempoffset && blinkingtemp == 1)) && !TEMP_LED) {
                if (isrCounter < 500) {
                    if (temperature < 99.999) {
                        u8g2.setCursor(13, 12);
                        u8g2.setFont(u8g2_font_fub35_tf);
                        u8g2.print(temperature, 1);
                    } else {
                        u8g2.setCursor(-1, 12);
                        u8g2.setFont(u8g2_font_fub35_tf);
                        u8g2.print(temperature, 1);
                    }
                }
            } else {
                if (temperature < 99.999) {
                    u8g2.setCursor(13, 12);
                    u8g2.setFont(u8g2_font_fub35_tf);
                    u8g2.print(temperature, 1);
                } else {
                    u8g2.setCursor(-1, 12);
                    u8g2.setFont(u8g2_font_fub35_tf);
                    u8g2.print(temperature, 1);
                }
            }
        }

        // For status info
        if (offlineMode == 0) {
            getSignalStrength();

            if (WiFi.status() != WL_CONNECTED) {
                u8g2.drawFrame(116, 28, 12, 12);
                u8g2.drawXBMP(118, 30, 8, 8, antenna_NOK_u8g2);
            }
        } else {
            u8g2.drawFrame(116, 28, 12, 12);
            u8g2.setCursor(120, 30);
            u8g2.print("O");
        }

        sendBufferWithIcons();

    }
}
