/**
 * @file Displaytemplatestandard.h
 *
 * @brief Standard display template
 *
 */

#pragma once

/**
 * @brief Send data to display
 */
void printScreen()
{
    if  ((machineState == kAtSetpoint || machineState == kPidNormal || machineState == kBrewDetectionTrailing) ||
        ((machineState == kBrew || machineState == kShotTimerAfterBrew) && SHOTTIMER == 0) ||  // shottimer == 0, also show brew
        machineState == kCoolDown || ((machineState == kInit || machineState == kColdStart ) && HEATINGLOGO == 0) ||
        ((machineState == kPidOffline)  && OFFLINEGLOGO == 0))
    {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_profont11_tf); // set font
        
        displayStatusbar();
        
        u8g2.setCursor(35, 14);
        u8g2.print(langstring_current_temp);
        u8g2.setCursor(84, 14);
        u8g2.print(temperature, 1);
        u8g2.setCursor(114, 14);
        u8g2.print((char)176);
        u8g2.print("C");
        u8g2.setCursor(35, 24);
        u8g2.print(langstring_set_temp);
        u8g2.setCursor(84, 24);
        u8g2.print(setpoint, 1);
        u8g2.setCursor(114, 24);
        u8g2.print((char)176);
        u8g2.print("C");

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

        // PID values over heat bar
        u8g2.setCursor(38, 47);

        u8g2.print(bPID.GetKp(), 0);
        u8g2.print("|");

        if (bPID.GetKi() != 0) {
            u8g2.print(bPID.GetKp() / bPID.GetKi(), 0);;
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

        // Brew time 
        u8g2.setCursor(35, 34);

        // Shot timer shown if machine is brewing and after the brew
        if (machineState == kBrew || machineState == kShotTimerAfterBrew) {
            u8g2.print(langstring_brew);
            u8g2.setCursor(72, 34);
            u8g2.print(timeBrewed / 1000, 0);
            u8g2.print("/");

            if (ONLYPID == 1) {
                u8g2.print(brewtimesoftware, 0); // Deactivate if only pid without preinfusion
            }
            else {
                u8g2.print(totalBrewTime / 1000, 1); // Activate if pre-infusion and one decimal place or alternatively none
            }
        }

        displayHeatbar(30, 60, 98);

        u8g2.sendBuffer();
    }
}
