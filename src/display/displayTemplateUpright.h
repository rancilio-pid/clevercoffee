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
    // Show shot timer:
    if (displayShottimer()) {
        // Display was updated, end here
        return;
    }

    // If no specific machine state was printed, print default:
    if (((machineState == kPidNormal || machineState == kBrewDetectionTrailing) || ((machineState == kBrew || machineState == kShotTimerAfterBrew) && FEATURE_SHOTTIMER == 0) || // shottimer == 0, auch Bezug anzeigen
         (machineState == kPidNormal && (setpoint - temperature) > 5. && FEATURE_HEATINGLOGO == 0) || ((machineState == kPidDisabled) && FEATURE_PIDOFF_LOGO == 0)) &&
        (brewSwitchState != kBrewSwitchFlushOff)) {
        if (!tempSensor->hasError()) {
            display.clearBuffer();
            display.setCursor(1, 14);
            display.print(langstring_current_temp_rot_ur);
            display.print(temperature, 1);
            display.print(" ");
            display.print((char)176);
            display.print("C");
            display.setCursor(1, 24);
            display.print(langstring_set_temp_rot_ur);
            display.print(setpoint, 1);
            display.print(" ");
            display.print((char)176);
            display.print("C");

            // Draw heat bar
            display.drawLine(0, 124, 63, 124);
            display.drawLine(0, 124, 0, 127);
            display.drawLine(64, 124, 63, 127);
            display.drawLine(0, 127, 63, 127);
            display.drawLine(1, 125, (pidOutput / 16.13) + 1, 125);
            display.drawLine(1, 126, (pidOutput / 16.13) + 1, 126);

            // print heating status
            display.setCursor(1, 50);
            display.setFont(FontType::Big);

            if (fabs(temperature - setpoint) < 0.3) {
                if (isrCounter < 500) {
                    display.print("OK");
                }
            }
            else {
                display.print("WAIT");
            }

            display.setFont(FontType::Normal);

            if (isBrewDetected == 1) {
                display.setCursor(1, 75);
                display.print("BD ");
                display.print((millis() - timeBrewDetection) / 1000, 1);
                display.print("/");
                display.print(brewtimesoftware, 0);
            }

            // PID values above heater output bar
            display.setCursor(1, 84);
            display.print("P: ");
            display.print(bPID.GetKp(), 0);

            display.setCursor(1, 93);
            display.print("I: ");

            if (bPID.GetKi() != 0) {
                display.print(bPID.GetKp() / bPID.GetKi(), 0);
            }
            else {
                display.print("0");
            }

            display.setCursor(1, 102);
            display.print("D: ");
            display.print(bPID.GetKd() / bPID.GetKp(), 0);

            display.setCursor(1, 111);

            if (pidOutput < 99) {
                display.print(pidOutput / 10, 1);
            }
            else {
                display.print(pidOutput / 10, 0);
            }

            display.print("%");

            // Brew
            display.setCursor(1, 34);
            display.print(langstring_brew_rot_ur);
            display.print(timeBrewed / 1000, 0);
            display.print("/");

            if (FEATURE_BREWCONTROL == 0) {
                display.print(brewtimesoftware, 0);     // deaktivieren wenn Preinfusion ( // voransetzen )
            }
            else {
                display.print(totalBrewTime / 1000, 0); // aktivieren wenn Preinfusion
            }

            display.print(" s");

            // For status info
            display.drawFrame(0, 0, 64, 12);

            if (offlineMode == 0) {
                getSignalStrength();

                if (WiFi.status() == WL_CONNECTED) {
                    display.drawImage(4, 2, 8, 8, Antenna_OK_Icon);

                    for (int b = 0; b <= getSignalStrength(); b++) {
                        display.drawVLine(13 + (b * 2), 10 - (b * 2), b * 2);
                    }
                }
                else {
                    display.drawImage(4, 2, 8, 8, Antenna_NOK_Icon);
                    display.setCursor(56, 2);
                    display.print("RC: ");
                    display.print(wifiReconnects);
                }

                if (FEATURE_MQTT == 1) {
                    if (mqtt.connected() == 1) {
                        display.setCursor(24, 2);
                        display.setFont(FontType::Normal);
                        display.print("MQTT");
                    }
                    else {
                        display.setCursor(24, 2);
                        display.print("");
                    }
                }
            }
            else {
                display.setCursor(4, 1);
                display.print("Offline");
            }

            display.sendBuffer();
        }
    }
}
