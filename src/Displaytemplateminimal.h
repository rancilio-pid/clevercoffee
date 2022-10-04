/**
 * @file Displaytemplateminimal.h
 *
 * @brief Minimal display template
 */


/**
 * @brief Send data to display
 */
void printScreen() {
    if ((machinestate == kSetPointNegative || machinestate == kPidNormal || machinestate == kBrewDetectionTrailing) ||
        ((machinestate == kBrew || machinestate == kShotTimerAfterBrew) && SHOTTIMER == 0) || // shottimer == 0, auch Bezug anzeigen
        machinestate == kCoolDown || ((machinestate == kColdStart) && HEATINGLOGO == 0) || ((machinestate == kPidOffline) && OFFLINEGLOGO == 0))
    {
        if (!sensorError) {
            u8g2.clearBuffer();

            // Draw heat bar outline
            u8g2.drawFrame(15, 58, 102, 4);
            u8g2.drawLine(16, 59, (pidOutput / 10) + 16, 59);
            u8g2.drawLine(16, 60, (pidOutput / 10) + 16, 60);

            // Draw heat bar outline
            u8g2.drawFrame(15, 58, 102, 4);
            u8g2.drawLine(16, 59, (pidOutput / 10) + 16, 59);
            u8g2.drawLine(16, 60, (pidOutput / 10) + 16, 60);

            int numDecimalsInput = 1;

            if (temperature > 99.999) {
                numDecimalsInput = 0;
            }

            int numDecimalsSetPoint = 1;

            if (setPoint > 99.999) {
                numDecimalsSetPoint = 0;
            }

            //draw (blinking) temp
            if (fabs(temperature - setPoint) < 0.3) {
                if (isrCounter < 500) {
                    // limit to 4 characters
                    u8g2.setCursor(2, 2);
                    u8g2.setFont(u8g2_font_profont22_tf);
                    u8g2.print(temperature, numDecimalsInput);
                    u8g2.setFont(u8g2_font_open_iconic_arrow_2x_t);
                    u8g2.print(char(78));
                    u8g2.setCursor(78, 2);
                    u8g2.setFont(u8g2_font_profont22_tf);
                    u8g2.print(setPoint, numDecimalsSetPoint);
                }
            } else {
                u8g2.setCursor(2, 2);
                u8g2.setFont(u8g2_font_profont22_tf);
                u8g2.print(temperature, numDecimalsInput);
                u8g2.setFont(u8g2_font_open_iconic_arrow_2x_t);
                u8g2.setCursor(56, 6);

                if (pidMode == 1) {
                    u8g2.print(char(74));
                } else {
                    u8g2.print(char(70));
                }

                u8g2.setCursor(79, 2);
                u8g2.setFont(u8g2_font_profont22_tf);
                u8g2.print(setPoint, numDecimalsSetPoint);
            }

            if (brewcounter > 10) {
                u8g2.setFont(u8g2_font_profont17_tf);

                // Brew
                u8g2.setCursor(2, 30);
            } else {
                u8g2.setFont(u8g2_font_profont10_tf);

                // Brew
                u8g2.setCursor(36, 30);
            }

            u8g2.print(langstring_brew);
            u8g2.print(timeBrewed / 1000, 0);
            u8g2.print("/");

            if (ONLYPID == 1) {
                u8g2.print(brewtimersoftware, 0); // deaktivieren wenn Preinfusion ( // voransetzen )
            } else {
                u8g2.print(totalBrewTime / 1000, 0); // aktivieren wenn Preinfusion
            }

            if (isBrewDetected == 1 && brewcounter == 10) {
                u8g2.setFont(u8g2_font_profont11_tf);

                // Brew
                u8g2.setCursor(30, 40);
                u8g2.print("BD:  ");
                u8g2.print((millis() - timeBrewDetection) / 1000, 1);
                u8g2.print("/");
                u8g2.print(brewtimersoftware, 0);
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
}
