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
    if  ((machinestate == kSetPointNegative || machinestate == kPidNormal || machinestate == kBrewDetectionTrailing) ||
        ((machinestate == kBrew || machinestate == kShotTimerAfterBrew) && SHOTTIMER == 0) ||  // shottimer == 0, also show brew
        machinestate == kCoolDown || ((machinestate == kInit || machinestate == kColdStart ) && HEATINGLOGO == 0) ||
        ((machinestate == kPidOffline)  && OFFLINEGLOGO == 0))
    {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_profont11_tf); // set font
        u8g2.drawXBMP(0, 0, logo_width, logo_height, logo_bits_u8g2);   //draw temp icon
        u8g2.setCursor(32, 14);
        u8g2.print(langstring_current_temp);
        u8g2.print(temperature, 1);
        u8g2.print(" ");
        u8g2.print((char)176);
        u8g2.print("C");
        u8g2.setCursor(32, 24);
        u8g2.print(langstring_set_temp);
        u8g2.print(setPoint, 1);
        u8g2.print(" ");
        u8g2.print((char)176);
        u8g2.print("C");

        // Draw heat bar
        u8g2.drawLine(15, 58, 117, 58);
        u8g2.drawLine(15, 58, 15, 61);
        u8g2.drawLine(117, 58, 117, 61);

        u8g2.drawLine(16, 59, (pidOutput / 10) + 16, 59);
        u8g2.drawLine(16, 60, (pidOutput / 10) + 16, 60);
        u8g2.drawLine(15, 61, 117, 61);

        // draw current temp in icon
        if (fabs(temperature  - setPoint) < 0.3) {
            if (isrCounter < 500) {
                u8g2.drawLine(9, 48, 9, 58 - (temperature  / 2));
                u8g2.drawLine(10, 48, 10, 58 - (temperature  / 2));
                u8g2.drawLine(11, 48, 11, 58 - (temperature  / 2));
                u8g2.drawLine(12, 48, 12, 58 - (temperature  / 2));
                u8g2.drawLine(13, 48, 13, 58 - (temperature  / 2));
            }
        } else if (temperature > 106) {
            u8g2.drawLine(9, 48, 9, 5);
            u8g2.drawLine(10, 48, 10, 4);
            u8g2.drawLine(11, 48, 11, 3);
            u8g2.drawLine(12, 48, 12, 4);
            u8g2.drawLine(13, 48, 13, 5);
        } else {
            u8g2.drawLine(9, 48, 9, 58 - (temperature  / 2));
            u8g2.drawLine(10, 48, 10, 58 - (temperature  / 2));
            u8g2.drawLine(11, 48, 11, 58 - (temperature  / 2));
            u8g2.drawLine(12, 48, 12, 58 - (temperature  / 2));
            u8g2.drawLine(13, 48, 13, 58 - (temperature  / 2));
        }

        //draw setPoint line
        u8g2.drawLine(18, 58 - (setPoint / 2), 23, 58 - (setPoint / 2));

        u8g2.setCursor(40, 48);

        u8g2.print(bPID.GetKp(), 0); // P
        u8g2.print("|");
        if (bPID.GetKi() != 0) {
            u8g2.print(bPID.GetKp() / bPID.GetKi(), 0);;
        } // I
        else {
            u8g2.print("0");
        }
        u8g2.print("|");
        u8g2.print(bPID.GetKd() / bPID.GetKp(), 0); // D
        u8g2.setCursor(98, 48);

        if (pidOutput < 99) {
            u8g2.print(pidOutput / 10, 1);
        } else {
            u8g2.print(pidOutput / 10, 0);
        }
        u8g2.print("%");

        // Brew time or uptime
        u8g2.setCursor(32, 34);

        if (isBrewDetected) {
            //show shot time
            u8g2.print(langstring_brew);
            u8g2.print(timeBrewed / 1000, 0);
            u8g2.print("/");

            if (ONLYPID == 1) {
                u8g2.print(brewtimersoftware, 0);       // deactivate if only pid without preinfusion
            }
            else {
                u8g2.print(totalBrewTime / 1000, 1);    // activate if pre-infusion and one decimal place or alternatively none
            }
        } else {
            //show uptime of machine
            u8g2.print(langstring_uptime);
            float seconds = millis() / 1000;
            int hours = seconds / 3600;
            int minutes = (seconds - hours * 60 * 60) / 60;

            //print hours and minutes
            u8g2.print(hours);
            u8g2.print("h ");
            u8g2.print(minutes);
            u8g2.print("m");
        }

        //draw box
        u8g2.drawFrame(0, 0, 128, 64);

        // Für Statusinfos
        u8g2.drawFrame(32, 0, 128, 12);

        if (offlineMode == 0) {
            getSignalStrength();

            if (WiFi.status() == WL_CONNECTED) {
                u8g2.drawXBMP(40, 2, 8, 8, antenna_OK_u8g2);
                for (int b = 0; b <= signalBars; b++) {
                    u8g2.drawVLine(45 + (b * 2), 10 - (b * 2), b * 2);
                }
            } else {
                u8g2.drawXBMP(40, 2, 8, 8, antenna_NOK_u8g2);
                u8g2.setCursor(88, 2);
                u8g2.print("RC: ");
                u8g2.print(wifiReconnects);
            }

            if (MQTT == 1) {
                if (mqtt.connected() == 1) {
                    u8g2.setCursor(77, 1);
                    u8g2.setFont(u8g2_font_profont11_tf);
                    u8g2.print("MQTT");
                } else {
                    u8g2.setCursor(77, 2);
                    u8g2.print("");
                }
            }
        } else {
            u8g2.setCursor(40, 2);
            u8g2.print(langstring_offlinemod);
        }

        u8g2.sendBuffer();
    }
}
