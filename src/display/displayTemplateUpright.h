/**
 * @file displayTemplateUpright.h
 *
 * @brief Vertical display template
 *
 */

#pragma once

/**
 * @brief Send data to display
 */
void printScreen() {
    if (((machineState == kPidNormal || machineState == kBrewDetectionTrailing) || ((machineState == kBrew || machineState == kShotTimerAfterBrew) && FEATURE_SHOTTIMER == 0) || // shottimer == 0, auch Bezug anzeigen
         machineState == kCoolDown || (machineState == kPidNormal && (setpoint - temperature) > 5. && FEATURE_HEATINGLOGO == 0) || ((machineState == kPidDisabled) && FEATURE_PIDOFF_LOGO == 0)) &&
        (brewSwitchState != kBrewSwitchFlushOff)) {
        if (!sensorError) {
            u8g2.clearBuffer();
            u8g2.setCursor(1, 14);
            u8g2.print(langstring_current_temp_rot_ur);
            u8g2.print(temperature, 1);
            u8g2.print(" ");
            u8g2.print((char)176);
            u8g2.print("C");
            u8g2.setCursor(1, 24);
            u8g2.print(langstring_set_temp_rot_ur);
            u8g2.print(setpoint, 1);
            u8g2.print(" ");
            u8g2.print((char)176);
            u8g2.print("C");

            // Draw heat bar
            u8g2.drawLine(0, 124, 63, 124);
            u8g2.drawLine(0, 124, 0, 127);
            u8g2.drawLine(64, 124, 63, 127);
            u8g2.drawLine(0, 127, 63, 127);
            u8g2.drawLine(1, 125, (pidOutput / 16.13) + 1, 125);
            u8g2.drawLine(1, 126, (pidOutput / 16.13) + 1, 126);

            // print heating status
            u8g2.setCursor(1, 50);
            u8g2.setFont(u8g2_font_profont22_tf);

            if (fabs(temperature - setpoint) < 0.3) {
                if (isrCounter < 500) {
                    u8g2.print("OK");
                }
            }
            else {
                u8g2.print("WAIT");
            }

            u8g2.setFont(u8g2_font_profont11_tf);

            if (isBrewDetected == 1) {
                u8g2.setCursor(1, 75);
                u8g2.print("BD ");
                u8g2.print((millis() - timeBrewDetection) / 1000, 1);
                u8g2.print("/");
                u8g2.print(brewtimesoftware, 0);
            }

            // PID values above heater output bar
            u8g2.setCursor(1, 84);
            u8g2.print("P: ");
            u8g2.print(bPID.GetKp(), 0);

            u8g2.setCursor(1, 93);
            u8g2.print("I: ");

            if (bPID.GetKi() != 0) {
                u8g2.print(bPID.GetKp() / bPID.GetKi(), 0);
            }
            else {
                u8g2.print("0");
            }

            u8g2.setCursor(1, 102);
            u8g2.print("D: ");
            u8g2.print(bPID.GetKd() / bPID.GetKp(), 0);

            u8g2.setCursor(1, 111);

            if (pidOutput < 99) {
                u8g2.print(pidOutput / 10, 1);
            }
            else {
                u8g2.print(pidOutput / 10, 0);
            }

            u8g2.print("%");

            // Brew
            u8g2.setCursor(1, 34);
            u8g2.print(langstring_brew_rot_ur);
            u8g2.print(timeBrewed / 1000, 0);
            u8g2.print("/");

            if (BREWCONTROL_TYPE == 0) {
                u8g2.print(brewtimesoftware, 0);     // deaktivieren wenn Preinfusion ( // voransetzen )
            }
            else {
                u8g2.print(totalBrewTime / 1000, 0); // aktivieren wenn Preinfusion
            }

            u8g2.print(" s");

            // For status info
            u8g2.drawFrame(0, 0, 64, 12);

            if (offlineMode == 0) {
                getSignalStrength();

                if (WiFi.status() == WL_CONNECTED) {
                    u8g2.drawXBMP(4, 2, 8, 8, Antenna_OK_Icon);

                    for (int b = 0; b <= getSignalStrength(); b++) {
                        u8g2.drawVLine(13 + (b * 2), 10 - (b * 2), b * 2);
                    }
                }
                else {
                    u8g2.drawXBMP(4, 2, 8, 8, Antenna_NOK_Icon);
                    u8g2.setCursor(56, 2);
                    u8g2.print("RC: ");
                    u8g2.print(wifiReconnects);
                }

                if (FEATURE_MQTT == 1) {
                    if (mqtt.connected() == 1) {
                        u8g2.setCursor(24, 2);
                        u8g2.setFont(u8g2_font_profont11_tf);
                        u8g2.print("MQTT");
                    }
                    else {
                        u8g2.setCursor(24, 2);
                        u8g2.print("");
                    }
                }
            }
            else {
                u8g2.setCursor(4, 1);
                u8g2.print("Offline");
            }

            u8g2.sendBuffer();
        }
    }
}
