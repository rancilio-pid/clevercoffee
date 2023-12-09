/**
 * @file display.h
 *
 * @brief TODO
 */

#pragma once

#if (OLED_DISPLAY != 0)


/**
 * @brief initialize display
 */
void u8g2_prepare(void) {
    u8g2.setFont(u8g2_font_profont11_tf);
    u8g2.setFontRefHeightExtendedText();
    u8g2.setDrawColor(1);
    u8g2.setFontPosTop();
    u8g2.setFontDirection(0);
    u8g2.setDisplayRotation(DISPLAYROTATE);
}

/**
 * Icons...
 * Show water empty icon in upper right corner if water empty
 */
void displayWaterIcon() {
        if (!waterFull) {
            //small water empty logo
            u8g2.drawXBMP(119, 2, 8, 8, water_EMPTY_u8g2);
		}
}

/**
 * Collector to always show indicators along all the other information
 */
void displayIcons() {
    displayWaterIcon();
}

/**
 * @brief print message
 */
void displayMessage(String text1, String text2, String text3, String text4, String text5, String text6) {
    u8g2.clearBuffer();
    u8g2.setCursor(0, 0);
    u8g2.print(text1);
    u8g2.setCursor(0, 10);
    u8g2.print(text2);
    u8g2.setCursor(0, 20);
    u8g2.print(text3);
    u8g2.setCursor(0, 30);
    u8g2.print(text4);
    u8g2.setCursor(0, 40);
    u8g2.print(text5);
    u8g2.setCursor(0, 50);
    u8g2.print(text6);
    displayIcons();
    u8g2.sendBuffer();
}


/**
 * @brief print logo and message at boot
 */
void displayLogo(String displaymessagetext, String displaymessagetext2) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 47, displaymessagetext.c_str());
    u8g2.drawStr(0, 55, displaymessagetext2.c_str());

    // Rancilio startup logo
    switch (machine) {
        case RancilioSilvia:  // Rancilio
            u8g2.drawXBMP(41, 2, startLogoRancilio_width, startLogoRancilio_height,
                            startLogoRancilio_bits);
            break;

        case RancilioSilviaE:  // Rancilio
            u8g2.drawXBMP(41, 2, startLogoRancilio_width, startLogoRancilio_height,
                            startLogoRancilio_bits);
            break;

        case Gaggia:  // Gaggia
            u8g2.drawXBMP(0, 2, startLogoGaggia_width, startLogoGaggia_height,
                            startLogoGaggia_bits);
            break;

        case QuickMill:  // Quickmill
            u8g2.drawXBMP(22, 0, startLogoQuickMill_width, startLogoQuickMill_height,
                            startLogoQuickMill_bits);
            break;
    }
    displayIcons();
    u8g2.sendBuffer();
}

/**
 * @brief calibration mode
 *
 * @param display_distance
 */
void displayDistance(int display_distance) {
    u8g2.clearBuffer();
    u8g2.setCursor(13, 12);
    u8g2.setFont(u8g2_font_fub20_tf);
    u8g2.printf("%d", display_distance);
    u8g2.print("mm");
    displayIcons();
    u8g2.sendBuffer();
}

/**
 * @brief display shot timer
 */
void displayShottimer(void) {
    if ((machineState == kBrew) && SHOTTIMER == 1) { // Shotimer has to be 1 and brew is running, then show time
        u8g2.clearBuffer();

        // temp icon
        u8g2.drawXBMP(0, 0, brewlogo_width, brewlogo_height, brewlogo_bits_u8g2);
        u8g2.setFont(u8g2_font_profont22_tf);
        u8g2.setCursor(64, 25);
        u8g2.print(timeBrewed / 1000, 1);
        u8g2.setFont(u8g2_font_profont11_tf);
        displayIcons();
        u8g2.sendBuffer();
    }

    /* if the totalBrewTime is reached automatically,
     * nothing should be done, otherwise wrong time is displayed
     * because the switch is pressed later than totalBrewTime
     */
    if (((machineState == kShotTimerAfterBrew) && SHOTTIMER == 1)) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(0, 0, brewlogo_width, brewlogo_height, brewlogo_bits_u8g2);
        u8g2.setFont(u8g2_font_profont22_tf);
        u8g2.setCursor(64, 25);
        u8g2.print(lastbrewTime / 1000, 1);
        u8g2.setFont(u8g2_font_profont11_tf);
        displayIcons();
        u8g2.sendBuffer();;
    }

    #if (ONLYPIDSCALE == 1 || BREWMODE == 2)
        if ((machineState == kBrew) && SHOTTIMER == 2) {
            u8g2.clearBuffer();

            // temp icon
            u8g2.drawXBMP(0, 0, brewlogo_width, brewlogo_height, brewlogo_bits_u8g2);
            u8g2.setFont(u8g2_font_profont22_tf);
            u8g2.setCursor(64, 15);
            u8g2.print(timeBrewed / 1000, 1);
            u8g2.print("s");
            u8g2.setCursor(64, 38);
            u8g2.print(weightBrew, 0);
            u8g2.print("g");
            u8g2.setFont(u8g2_font_profont11_tf);
            displayIcons();
            u8g2.sendBuffer();
        }

        if (((machineState == kShotTimerAfterBrew) && SHOTTIMER == 2)) {
            u8g2.clearBuffer();
            u8g2.drawXBMP(0, 0, brewlogo_width, brewlogo_height, brewlogo_bits_u8g2);
            u8g2.setFont(u8g2_font_profont22_tf);
            u8g2.setCursor(64, 15);
            u8g2.print(lastbrewTime/1000, 1);
            u8g2.print("g");
            u8g2.setCursor(64, 38);
            u8g2.print(weightBrew, 0);
            u8g2.print(" g");
            u8g2.setFont(u8g2_font_profont11_tf);
            displayIcons();
            u8g2.sendBuffer();;
        }
    #endif
}


/**
 * @brief display heating logo
 */
void Displaymachinestate() {
    if (HEATINGLOGO > 0 && (machineState == kInit || machineState == kColdStart)) {
        // For status info
        u8g2.clearBuffer();
        u8g2.drawFrame(8, 0, 110, 12);

        if (offlineMode == 0) {
            getSignalStrength();

            if (WiFi.status() == WL_CONNECTED) {
                u8g2.drawXBMP(40, 2, 8, 8, antenna_OK_u8g2);

                for (int b = 0; b <= signalBars; b++) {
                    u8g2.drawVLine(45 + (b * 2), 10 - (b * 2), b * 2);
                }
            } 
            else {
                u8g2.drawXBMP(40, 2, 8, 8, antenna_NOK_u8g2);
                u8g2.setCursor(88, 1);
                u8g2.print("RC: ");
                u8g2.print(wifiReconnects);
            }

            if (MQTT == 1) {
                if (mqtt.connected() == 1) {
                    u8g2.setCursor(60, 1);
                    u8g2.setFont(u8g2_font_profont11_tf);
                    u8g2.print("MQTT");
                } 
                else {
                    u8g2.setCursor(60, 2);
                    u8g2.print("");
                }
            }
        } 
        else {
            u8g2.setCursor(40, 1);
            u8g2.print(langstring_offlinemode);
        }

        // Rancilio logo
        if (HEATINGLOGO == 1) {
            u8g2.drawXBMP(0, 14, Rancilio_Silvia_Logo_width, Rancilio_Silvia_Logo_height, Rancilio_Silvia_Logo);
            u8g2.drawXBMP(53, 14, Heiz_Logo_width, Heiz_Logo_height, Heiz_Logo);
            u8g2.setFont(u8g2_font_profont22_tf);
        }

        // Gaggia Logo
        if (HEATINGLOGO == 2) {
            u8g2.drawXBMP(0, 14, Gaggia_Classic_Logo_width, Gaggia_Classic_Logo_height, Gaggia_Classic_Logo);
            u8g2.drawXBMP(53, 14, Heiz_Logo_width, Heiz_Logo_height, Heiz_Logo);
            u8g2.setFont(u8g2_font_profont22_tf);
        }

        // Temperature
        u8g2.setCursor(92, 30);
        u8g2.setFont(u8g2_font_profont17_tf);
        u8g2.print(temperature, 1);
        displayIcons();
        u8g2.sendBuffer();;
    }

    // Offline logo
    if (OFFLINEGLOGO == 1 && machineState == kPidOffline) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(38, 0, OFFLogo_width, OFFLogo_height, OFFLogo);
        u8g2.setCursor(0, 55);
        u8g2.setFont(u8g2_font_profont10_tf);
        u8g2.print("PID is disabled manually");
        displayIcons();
        u8g2.sendBuffer();
    }

    if (OFFLINEGLOGO == 1 && machineState == kStandby) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(38, 0, OFFLogo_width, OFFLogo_height, OFFLogo);
        u8g2.setCursor(36, 55);
        u8g2.setFont(u8g2_font_profont10_tf);
        u8g2.print("Standby mode");
        displayIcons();
        u8g2.sendBuffer();;
    }

    // Steam
    if (machineState == kSteam) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(0, 0, steamlogo_width, steamlogo_height, steamlogo);
        u8g2.setCursor(64, 25);
        u8g2.setFont(u8g2_font_profont22_tf);
        u8g2.print(temperature, 0);
        u8g2.setCursor(64, 25);
        displayIcons();
        u8g2.sendBuffer();;
    }

    // Water empty
    if (machineState == kWaterEmpty) {
        u8g2.clearBuffer();
        u8g2.setFontPosBottom();
        u8g2.setFont(u8g2_font_profont22_tr);
        u8g2.drawStr(53, 26, "Fill");
        u8g2.drawStr(59, 42, "water");
        u8g2.drawXBMP( 3, 0, water_empty_big_width, water_empty_big_height, water_EMPTY_big_u8g2); 
        u8g2.setFont(u8g2_font_profont11_tf);
        u8g2.setCursor(80, 64);
        u8g2.print(temperature);
        u8g2.print((char)176);
        u8g2.print("C");
        u8g2.setFontPosTop();
        displayIcons();
        u8g2.sendBuffer();;
    }

    // Water empty
    if (machineState == kWaterEmpty) {
        u8g2.clearBuffer();
        u8g2.setFontPosBottom();
        static const unsigned char image_fill_bits[] U8X8_PROGMEM = {0x00,0x00,0xfc,0x07,0x00,0x00,0x00,0x80,0xff,0x3f,0x00,0x00,0x00,0xe0,0x07,0xfc,0x00,0x00,0x00,0xf0,0x00,0xe0,0x01,0x00,0x00,0x3c,0xf0,0x81,0x03,0x00,0x00,0x1e,0xfe,0x0f,0x07,0x00,0x00,0x87,0x1f,0x3f,0x0e,0x00,0x00,0xc3,0x01,0x78,0x1c,0x00,0x80,0xf3,0x00,0xe0,0x38,0x00,0xc0,0x31,0x00,0xc0,0x31,0x00,0xc0,0x18,0x00,0x80,0x73,0x00,0xe0,0x1c,0x00,0x00,0x67,0x00,0x60,0x0c,0x00,0x00,0xe6,0x00,0x60,0x0e,0x00,0x00,0xc6,0x00,0x60,0x06,0x00,0x00,0xcc,0x00,0x70,0x06,0x00,0x00,0xcc,0x00,0x30,0x06,0x00,0x00,0xcc,0x00,0xfc,0x0f,0x00,0x00,0xcc,0x00,0xfc,0x1f,0x00,0x00,0xcc,0x00,0x0c,0x18,0x00,0x00,0xcc,0x00,0x0c,0x18,0x00,0x00,0xcc,0x00,0x0c,0x18,0x00,0x00,0xcc,0x00,0x0c,0x18,0x00,0x00,0xcc,0x01,0x0e,0x18,0x00,0x00,0xcc,0x01,0x06,0x38,0x00,0x00,0xcc,0x01,0x0e,0x38,0x00,0x00,0xcc,0x01,0xfe,0x3f,0x00,0x00,0xcc,0x01,0xfc,0x1f,0x00,0x00,0xcc,0x01,0x00,0x00,0x00,0x00,0xcc,0x01,0x00,0x00,0x00,0x00,0xcc,0x01,0x00,0x00,0x80,0x03,0xcc,0x01,0x00,0x00,0xc0,0x07,0xcc,0x01,0x00,0x00,0xe0,0x0f,0xcc,0x01,0x80,0x00,0x60,0x1c,0xcc,0x01,0xc0,0x01,0x70,0x38,0xfe,0x01,0xe0,0x03,0xe0,0x70,0xfe,0x03,0x70,0x03,0xc0,0xe1,0x86,0x03,0x30,0x06,0x80,0xe3,0x06,0x03,0x38,0x0e,0x00,0x77,0x06,0x03,0x1c,0x0c,0x00,0x7e,0x06,0x03,0x0c,0x1c,0x00,0xfc,0x86,0x03,0x4e,0x18,0x00,0xc0,0x87,0x03,0xc6,0x30,0x00,0x80,0xff,0x03,0x47,0x30,0x00,0x00,0xff,0x03,0x03,0x70,0x00,0x00,0x86,0x03,0x03,0x60,0x00,0x00,0x07,0x03,0x73,0x60,0x00,0x00,0x03,0x07,0x63,0x70,0x00,0x00,0x03,0x07,0x47,0x30,0x00,0x00,0x07,0x03,0x0e,0x38,0x00,0x00,0x8e,0x03,0x1c,0x1c,0x00,0x00,0xfe,0x03,0xf8,0x0f,0x00,0x00,0xfe,0x03,0xf0,0x07,0x00,0x00,0x86,0x03,0x00,0x00,0x00,0x00,0x86,0x03,0x00,0x00,0x00,0x00,0x06,0x03,0x00,0x00,0x00,0x00,0x06,0x03,0x00,0x00,0x00,0x00,0x06,0x03,0x00,0x00,0x00,0x00,0x8e,0x03,0x00,0x00,0x00,0xf0,0xff,0x7f,0x00,0x00,0x00,0xf0,0xff,0x7f,0x00,0x00,0x00,0x30,0x00,0x60,0x00,0x00,0x00,0x30,0x00,0x60,0x00,0x00,0x00,0xf0,0xff,0x7f,0x00,0x00,0x00,0xf0,0xff,0x7f};
        u8g2.setFont(u8g2_font_profont22_tr);
        u8g2.drawStr(50, 29, "Fill");
        u8g2.drawStr(56, 48, "water");
        u8g2.drawXBMP( 3, 0, 47, 64, image_fill_bits); //water_EMPTY_big_u8g2
        u8g2.setFont(u8g2_font_5x8_tr);
        u8g2.setCursor(91,63);
        u8g2.print(temperature);
        u8g2.print("°C");
        u8g2.setFontPosTop();
        sendBufferWithIcons();
    }

    // Backflush
    if (machineState == kBackflush) {
        u8g2.setFont(u8g2_font_profont11_tf);
        if (backflushState == 43) {
            #if OLED_DISPLAY != 0
                displayMessage(langstring_bckffinished[0], langstring_bckffinished[1], "", "", "", "");
            #endif
        } else if (backflushState == 10) {
            #if OLED_DISPLAY != 0
                displayMessage(langstring_bckfactivated[0], langstring_bckfactivated[1], "", "", "", "");
            #endif
        } else if (backflushState > 10) {
            #if OLED_DISPLAY != 0
                displayMessage(langstring_bckfrunning[0], String(flushCycles), langstring_bckfrunning[1], String(maxflushCycles), "", "");
            #endif
        }
    }

    // PID Off
    if (machineState == kEmergencyStop) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_profont11_tf);
        u8g2.drawXBMP(0, 0, logo_width, logo_height, logo_bits_u8g2);  // draw temp icon
        u8g2.setCursor(32, 24);
        u8g2.print(langstring_current_temp);
        u8g2.print(temperature, 1);
        u8g2.print(" ");
        u8g2.print((char)176);
        u8g2.print("C");
        u8g2.setCursor(32, 34);
        u8g2.print(langstring_set_temp);
        u8g2.print(setpoint, 1);
        u8g2.print(" ");
        u8g2.print((char)176);
        u8g2.print("C");

        // draw current temp in icon
        if (isrCounter < 500) {
            u8g2.drawLine(9, 48, 9, 5);
            u8g2.drawLine(10, 48, 10, 4);
            u8g2.drawLine(11, 48, 11, 3);
            u8g2.drawLine(12, 48, 12, 4);
            u8g2.drawLine(13, 48, 13, 5);
            u8g2.setCursor(32, 4);
            u8g2.print("PID STOPPED");
        }

        displayIcons();
        u8g2.sendBuffer();;
    }

    if (machineState == kSensorError) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_profont11_tf);
        displayMessage(langstring_error_tsensor[0], String(temperature), langstring_error_tsensor[1], "", "", "");
    }

    if (machineState == kEepromError) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_profont11_tf);
        displayMessage("EEPROM Error, please set Values", "", "", "", "", "");
    }
}
#endif
