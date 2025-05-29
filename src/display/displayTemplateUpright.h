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

    if ((machineState == kPidNormal || (machineState == kBrew && FEATURE_SHOTTIMER == 0) || ((machineState == kPidDisabled) && FEATURE_PIDOFF_LOGO == 0)) && (brewSwitchState != kManualFlush) ||
        (machineState == kWaterTankEmpty) || (machineState == kPidDisabled) || (machineState == kStandby) || (machineState == kSteam)) {
        if (!tempSensor->hasError()) {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_profont11_tf);
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

            // Show the heating logo when we are in regular PID mode
            if (FEATURE_HEATINGLOGO > 0 && machineState == kPidNormal && (setpoint - temperature) > 0.3) {
                // For status info

                u8g2.drawXBMP(12, 50, Heating_Logo_width, Heating_Logo_height, Heating_Logo);
                u8g2.setFont(u8g2_font_fub17_tf);
                u8g2.setCursor(8, 90);
                u8g2.print(temperature, 1);
            }
            // Offline logo
            else if (FEATURE_PIDOFF_LOGO == 1 && machineState == kPidDisabled) {

                u8g2.drawXBMP(6, 50, Off_Logo_width, Off_Logo_height, Off_Logo);
                u8g2.setCursor(1, 110);
                u8g2.setFont(u8g2_font_profont10_tf);
                u8g2.print("PID disabled");
            }
            else if (FEATURE_PIDOFF_LOGO == 1 && machineState == kStandby) {

                u8g2.drawXBMP(6, 50, Off_Logo_width, Off_Logo_height, Off_Logo);
                u8g2.setCursor(1, 110);
                u8g2.setFont(u8g2_font_profont10_tf);
                u8g2.print("Standby mode");
            }
            // Steam
            else if (machineState == kSteam) {

                u8g2.drawXBMP(12, 50, Steam_Logo_width, Steam_Logo_height, Steam_Logo);
            }
            // Water empty
            else if (machineState == kWaterEmpty) {
                u8g2.drawXBMP(8, 50, Water_Empty_Logo_width, Water_Empty_Logo_height, Water_Empty_Logo);
            }
            else {

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
            u8g2.print(currBrewTime / 1000, 0);

            if (FEATURE_BREWCONTROL == 1) {
                u8g2.print("/");
                u8g2.print(totalTargetBrewTime / 1000, 0);
            }

            u8g2.print(" s");
        }

        // Brew time
#if (FEATURE_BREWSWITCH == 1)
        if (featureBrewControl) {
            // Show brew time
            if (shouldDisplayBrewTimer()) {
                u8g2.setCursor(1, 34);
                u8g2.print(langstring_brew_rot_ur);
                u8g2.print(currBrewTime / 1000, 0);
                u8g2.print("/");
                u8g2.print(totalTargetBrewTime / 1000, 0);
                u8g2.print(" s");
            }

            // Shown flush time
            if (machineState == kManualFlush) {
                u8g2.setDrawColor(0);
                u8g2.drawBox(1, 34, 100, 15);
                u8g2.setDrawColor(1);
                u8g2.setCursor(1, 34);
                u8g2.print(langstring_manual_flush_rot_ur);
                u8g2.print(currBrewTime / 1000, 0);
                u8g2.print(" s");
            }
        }
        else {
            // Show brew time with optocoupler
            if (shouldDisplayBrewTimer()) {
                u8g2.setCursor(1, 34);
                u8g2.print(langstring_brew_rot_ur);
                u8g2.print(currBrewTime / 1000, 0);
                u8g2.print(" s");
            }
        }
#endif

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
