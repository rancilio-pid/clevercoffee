/**
 * @file Displaytemplatescale.h
 *
 * @brief Display template with brew scale
 *
 */

#pragma once

/**
 * @brief Send data to display
 */
void printScreen() {
    if ((machineState == kAtSetpoint || machineState == kPidNormal || machineState == kBrewDetectionTrailing) ||
        ((machineState == kBrew || machineState == kShotTimerAfterBrew) && SHOTTIMER == 0) ||  // shottimer == 0, auch Bezug anzeigen
        machineState == kCoolDown || ((machineState == kInit || machineState == kColdStart) &&
        HEATINGLOGO == 0) || ((machineState == kPidOffline) && OFFLINEGLOGO == 0))
    {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_profont11_tf);

        displayStatusbar();

        u8g2.setCursor(32, 14);
        u8g2.print("T:  ");
        u8g2.print(temperature, 1);

        u8g2.print("/");
        u8g2.print(setpoint, 1);

        displayThermometerOutline(4, 62);

        // Draw current temp in thermometer
        if (fabs(temperature  - setpoint) < 0.3) {
            if (isrCounter < 500) {
                drawTemperaturebar(8, 50, 30);
            }
        }
        else {
            drawTemperaturebar(8, 50, 30);
        }

        // Brew
        u8g2.setCursor(32, 34);
        u8g2.print("t: ");
        u8g2.print(timeBrewed / 1000, 0);
        u8g2.print("/");

        if (ONLYPID == 1) {
            u8g2.print(brewtimesoftware, 0);
        }
        else {
            u8g2.print(totalBrewTime / 1000, 1);
        }

        u8g2.setCursor(32, 24);
        u8g2.print("W: ");

        if (scaleFailure) {
            u8g2.print("fault");
        }
        else {
            if (brewswitch == LOW) {
                u8g2.print(weight, 0);
            }
            else {
                u8g2.print(weightBrew, 0);
            }

            u8g2.print("/");
            u8g2.print(weightSetpoint, 0);
            u8g2.print(" (");
            u8g2.print(weightBrew, 1);
            u8g2.print(")");
        }

        #if (PRESSURESENSOR == 1)
            u8g2.setCursor(32, 44);
            u8g2.print("P: ");
            u8g2.print(inputPressure, 1);
        #endif

        displayHeatbar(30, 60, 98);

        u8g2.sendBuffer();
    }
}
