/**
 * @file Displaytemplatetemponly.h
 *
 * @brief Temp-only display template
 */

// Define some Displayoptions
int blinkingtemp = 1;            // 0: blinking near setpoint, 1: blinking far away from setpoint
float blinkingtempoffset = 0.3;  // offset for blinking

/**
 * @brief Send data to display
 */
void printScreen() {
    if ((machinestate == kSetPointNegative || machinestate == kPidNormal || machinestate == kBrewDetectionTrailing) ||
        ((machinestate == kBrew || machinestate == kShotTimerAfterBrew) && SHOTTIMER == 0) ||  // shottimer == 0, auch Bezug anzeigen
        machinestate == kCoolDown || ((machinestate == kInit || machinestate == kColdStart) && HEATINGLOGO == 0) ||
        ((machinestate == kPidOffline) && OFFLINEGLOGO == 0)) {

        if (!sensorError) {
            u8g2.clearBuffer();

            // draw (blinking) temp
            if ((fabs(temperature - setPoint) < blinkingtempoffset && blinkingtemp == 0) ||
                (fabs(temperature - setPoint) >= blinkingtempoffset && blinkingtemp == 1)) {
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

        // Für Statusinfos
        if (offlineMode == 0) {
            getSignalStrength();

            if (WiFi.status() != WL_CONNECTED) {
                u8g2.drawFrame(116, 28, 12, 12);
                u8g2.drawXBMP(118, 30, 8, 8, antenna_NOK_u8g2);
            } else {
                if (BLYNK == 1) {
                    if (!Blynk.connected()) {
                        u8g2.drawFrame(116, 28, 12, 12);
                        u8g2.drawXBMP(118, 30, 8, 8, blynk_NOK_u8g2);
                    }
                }
            }
        } else {
            u8g2.drawFrame(116, 28, 12, 12);
            u8g2.setCursor(120, 30);
            u8g2.print("O");
        }

        u8g2.sendBuffer();
    }
}
