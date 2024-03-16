/**
 * @file displayTemplateStandard.h
 *
 * @brief Standard display template
 *
 */

#pragma once

/**
 * @brief Send data to display
 */
void printScreen() {
    if (((machineState == kAtSetpoint || machineState == kPidNormal || machineState == kBrewDetectionTrailing) ||
         ((machineState == kBrew || machineState == kShotTimerAfterBrew) && FEATURE_SHOTTIMER == 0) || // shottimer == 0, auch Bezug anzeigen
         machineState == kCoolDown || ((machineState == kInit || machineState == kColdStart) && FEATURE_HEATINGLOGO == 0) || ((machineState == kPidOffline) && FEATURE_OFFLINELOGO == 0)) &&
        (brewSwitchState != kBrewSwitchFlushOff)) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_profont11_tf); // set font

        displayStatusbar();

        u8g2.setCursor(35, 16);
        u8g2.print(langstring_current_temp);
        u8g2.setCursor(84, 16);
        u8g2.print(temperature, 1);
        u8g2.setCursor(114, 16);
        u8g2.print((char)176);
        u8g2.print("C");
        u8g2.setCursor(35, 26);
        u8g2.print(langstring_set_temp);
        u8g2.setCursor(84, 26);
        u8g2.print(setpoint, 1);
        u8g2.setCursor(114, 26);
        u8g2.print((char)176);
        u8g2.print("C");

        displayThermometerOutline(4, 62);

        // Draw current temp in thermometer
        if (fabs(temperature - setpoint) < 0.3) {
            if (isrCounter < 500) {
                drawTemperaturebar(8, 50, 30);
            }
        }
        else {
            drawTemperaturebar(8, 50, 30);
        }

        // Brew time
        u8g2.setCursor(35, 36);

        // Shot timer shown if machine is brewing and after the brew
        if (machineState == kBrew || machineState == kShotTimerAfterBrew) {
            u8g2.print(langstring_brew);
            u8g2.setCursor(84, 36);
            u8g2.print(timeBrewed / 1000, 0);
            u8g2.print("/");

            if (BREWCONTROL_TYPE == 0) {
                u8g2.print(brewtimesoftware, 0);
            }
            else {
                u8g2.print(totalBrewTime / 1000, 1);
            }
        }

        // PID values over heat bar
        u8g2.setCursor(38, 47);

        u8g2.print(bPID.GetKp(), 0);
        u8g2.print("|");

        if (bPID.GetKi() != 0) {
            u8g2.print(bPID.GetKp() / bPID.GetKi(), 0);
        }
        else {
            u8g2.print("0");
        }

        u8g2.print("|");
        u8g2.print(bPID.GetKd() / bPID.GetKp(), 0);
        u8g2.setCursor(96, 47);

        if (pidOutput < 99) {
            u8g2.print(pidOutput / 10, 1);
        }
        else {
            u8g2.print(pidOutput / 10, 0);
        }

        u8g2.print("%");

        // Show heater output in %
        displayProgressbar(pidOutput / 10, 30, 60, 98);

        u8g2.sendBuffer();
    }
}
