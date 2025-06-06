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
    u8g2.clearBuffer();
    if (machineState == kWaterTankEmpty) { // Water empty
        u8g2.drawXBMP(8, 50, Water_Tank_Empty_Logo_width, Water_Tank_Empty_Logo_height, Water_Tank_Empty_Logo);
    }
    else if (machineState == kSensorError) {
        u8g2.setFont(u8g2_font_profont11_tf);
        displayMessage(langstring_error_tsensor_rot_ur[0], langstring_error_tsensor_rot_ur[1], String(temperature), langstring_error_tsensor_rot_ur[2], langstring_error_tsensor_rot_ur[3], langstring_error_tsensor_rot_ur[4]);
    }
    else if (FEATURE_PIDOFF_LOGO == 1 && machineState == kStandby) {
        u8g2.drawXBMP(6, 50, Off_Logo_width, Off_Logo_height, Off_Logo);
        u8g2.setCursor(1, 110);
        u8g2.setFont(u8g2_font_profont10_tf);
        u8g2.print("Standby mode");
    }

    else {
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

        if (FEATURE_PIDOFF_LOGO == 1 && machineState == kPidDisabled) {

            u8g2.drawXBMP(6, 50, Off_Logo_width, Off_Logo_height, Off_Logo);
            u8g2.setCursor(1, 110);
            u8g2.setFont(u8g2_font_profont10_tf);
            u8g2.print("PID disabled");
        }

        // Steam
        else if (machineState == kSteam) {
            u8g2.drawXBMP(12, 50, Steam_Logo_width, Steam_Logo_height, Steam_Logo);
        }

        // Show the heating logo when we are in regular PID mode
        if (FEATURE_HEATINGLOGO > 0 && machineState == kPidNormal && (setpoint - temperature) > 0.3) {
            // For status info

            u8g2.drawXBMP(12, 50, Heating_Logo_width, Heating_Logo_height, Heating_Logo);
            u8g2.setFont(u8g2_font_fub17_tf);
            u8g2.setCursor(8, 90);
            u8g2.print(temperature, 1);
        }
        else {
// print heating status
#if (FEATURE_SCALE == 1) && (FEATURE_PRESSURESENSOR == 1)
            u8g2.setCursor(1, 64);
#elif (FEATURE_SCALE == 1) || (FEATURE_PRESSURESENSOR == 1)
            u8g2.setCursor(1, 60);
#else
            u8g2.setCursor(1, 50);
#endif

            u8g2.setFont(u8g2_font_profont22_tf);

            if (fabs(temperature - setpoint) < 0.3) {
                if (isrCounter < 500) {
                    if (featureBrewControl && machineState == kManualFlush) {
                        u8g2.print("FLUSH"); // no flush function if optocoupler? manual flush is still triggered if momentary
                    }
                    else if (shouldDisplayBrewTimer()) {
                        u8g2.print("BREW");
                    }
                    else {
                        u8g2.print("OK");
                    }
                }
            }
            else {
                u8g2.print("WAIT");
            }
            u8g2.setFont(u8g2_font_profont11_tf);

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
            displayBrewTime(1, 34, langstring_brew_rot_ur, currBrewTime, totalTargetBrewTime);
#if (FEATURE_SCALE == 1)
            displayBrewWeight(1, 44, currReadingWeight, -1, scaleFailure);
#endif
#if (FEATURE_PRESSURESENSOR == 1)
            u8g2.setFont(u8g2_font_profont11_tf);
            if (FEATURE_SCALE == 1) {
                u8g2.setCursor(1, 54);
            }
            else {
                u8g2.setCursor(1, 44);
            }
            u8g2.print(langstring_pressure_rot_ur);
            u8g2.print(inputPressure, 1);
            u8g2.print(" bar");
#endif
            // Brew time
#if (FEATURE_BREWSWITCH == 1)
            if (featureBrewControl) {
                // Show brew time
                if (shouldDisplayBrewTimer()) {
                    displayBrewTime(1, 34, langstring_brew_rot_ur, currBrewTime, totalTargetBrewTime);
#if (FEATURE_SCALE == 1)
                    u8g2.setDrawColor(0);
                    u8g2.drawBox(1, 45, 100, 10);
                    u8g2.setDrawColor(1);
                    displayBrewWeight(1, 44, currBrewWeight, targetBrewWeight, scaleFailure);
#endif
                }
                // Shown flush time
                if (machineState == kManualFlush) {
                    u8g2.setDrawColor(0);
                    u8g2.drawBox(1, 35, 100, 10);
                    u8g2.setDrawColor(1);
                    displayBrewTime(1, 34, langstring_manual_flush_rot_ur, currBrewTime);
                }
            }
            else {
                // Show brew time with optocoupler
                if (shouldDisplayBrewTimer()) {
                    u8g2.setDrawColor(0);
                    u8g2.drawBox(1, 35, 100, 10);
                    u8g2.setDrawColor(1);
                    displayBrewTime(1, 34, langstring_brew_rot_ur, currBrewTime);
#if (FEATURE_SCALE == 1)
                    u8g2.setDrawColor(0);
                    u8g2.drawBox(1, 45, 100, 10);
                    u8g2.setDrawColor(1);
                    displayBrewWeight(1, 44, currBrewWeight, -1, scaleFailure);
#endif
                }
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
        else {
            u8g2.setCursor(4, 1);
            u8g2.print("Offline");
        }
    }

    u8g2.sendBuffer();
}
