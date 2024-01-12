/**
 * @file display.h
 *
 * @brief Common functions for all display templates
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
 * @brief Draw a water empty icon at the given coordinates if water supply is low
 */
void displayWaterIcon(int x, int y) {
    if (!waterFull) {
        u8g2.drawXBMP(x, y, 8, 8, Water_Empty_Icon);
    }
}

/**
 * @brief Draw the system uptime at the given coordinates
 */
void displayUptime(int x, int y, const char * format) {
    // Show uptime of machine
    unsigned long seconds = millis() / 1000;
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;

    char uptimeString[9]; 
    snprintf(uptimeString, sizeof(uptimeString), format, hours, minutes, seconds);

    u8g2.setFont(u8g2_font_profont11_tf);
    u8g2.drawStr(x, y, uptimeString);
}

/**
 * @brief Draw a WiFi signal strength indicator at the given coordinates 
 */
void displayWiFiStatus(int x, int y) {
    getSignalStrength();

    if (WiFi.status() == WL_CONNECTED) {
        u8g2.drawXBMP(x, y, 8, 8, Antenna_OK_Icon);

        for (int b = 0; b <= signalBars; b++) {
            u8g2.drawVLine(x + 5 + (b * 2), y + 8 - (b * 2), b * 2);
        }
    } 
    else {
        u8g2.drawXBMP(x, y, 8, 8, Antenna_NOK_Icon);
        u8g2.setCursor(x + 5, y + 10);
        u8g2.setFont(u8g2_font_profont11_tf);
        u8g2.print("RC: ");
        u8g2.print(wifiReconnects);
    }
}

/**
 * @brief Draw an MQTT status indicator at the given coordinates if MQTT is enabled 
 */
void displayMQTTStatus(int x, int y) {
    if (MQTT == 1) {
        if (mqtt.connected() == 1) {
            u8g2.setCursor(x, y);
            u8g2.setFont(u8g2_font_profont11_tf);
            u8g2.print("MQTT");
        } 
        else {
            u8g2.setCursor(x, y);
            u8g2.print("");
        }
    }
}

/**
 * @brief Draw the outline of a thermometer for use in conjunction with the drawTemperaturebar method
 */
void displayThermometerOutline(int x, int y) {
    u8g2.drawLine(x + 3, y - 9, x + 3, y - 42);
    u8g2.drawLine(x + 9, y - 9, x + 9, y - 42);
    u8g2.drawPixel(x + 4, y - 43);
    u8g2.drawPixel(x + 8, y - 43);
    u8g2.drawLine(x + 5, y - 44, x + 7, y - 44);
    u8g2.drawDisc(x + 6, y - 5, 6);

    // draw setpoint line
    int height = map(setpoint, 0, 100, y - 9, y - 39);
    u8g2.drawLine(x + 11, height, x + 16, height);
}

/**
 * @brief Draw temperature bar, e.g. inside the thermometer outline. 
 *        Add 4 pixels to the x-coordinate and subtract 12 pixels from the y-coordinate of the thermometer.
 */
void drawTemperaturebar(int x, int y, int heightRange) {
    int width = x + 5;

    for (int i = x; i < width; i++) {
        int height = map(temperature, 0, 100, 0, heightRange);
        u8g2.drawVLine(i, 52 - height, height);
    }

    if (temperature > 100) {
        u8g2.drawLine(x, heightRange - 11, x + 3, heightRange - 11);
        u8g2.drawLine(x, heightRange - 10, x + 4, heightRange - 10);
        u8g2.drawLine(x, heightRange - 9, x + 4, heightRange - 9);
    }
}

/**
 * @brief Draw a bar showing the heater output in % at the given coordinates and with the given width 
 */
void displayHeatbar(int x, int y, int width) {
    u8g2.drawFrame(x, y, width, 4);
    int output = map(pidOutput / 10, 0, 100, 0, width);

    if (output - 2 > 0) {
        u8g2.drawLine(x + 2, y + 1, x + output - 2, y + 1);
        u8g2.drawLine(x + 2, y + 2, x + output - 2, y + 2);
    }
}

/** 
 * @brief Draw a status bar at the top of the screen with icons for WiFi, MQTT, 
 *        the system uptime and a separator line underneath
 */
void displayStatusbar() {
    // For status info
    u8g2.drawLine(0, 12, 128, 12);

    if (offlineMode == 0) {
        displayWiFiStatus(4, 1);
        displayMQTTStatus(38, 0);
    } 
    else {
        u8g2.setCursor(4, 0);
        u8g2.setFont(u8g2_font_profont11_tf);
        u8g2.print(langstring_offlinemode);
    }

    const char* format = "%02luh %02lum";
    displayUptime(84, 0, format);
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
    u8g2.sendBuffer();
}

/**
 * @brief print logo and message at boot
 */
void displayLogo(String displaymessagetext, String displaymessagetext2) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 47, displaymessagetext.c_str());
    u8g2.drawStr(0, 55, displaymessagetext2.c_str());

    u8g2.drawXBMP(38, 4, CleverCoffee_Logo_width, CleverCoffee_Logo_height, CleverCoffee_Logo);

    u8g2.sendBuffer();
}

/**
 * @brief display shot timer
 */
void displayShottimer(void) {
    if ((machineState == kBrew) && SHOTTIMER == 1) { // Shotimer has to be 1 and brew is running, then show time
        u8g2.clearBuffer();

        // temp icon
        u8g2.drawXBMP(-1, 11, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);
        u8g2.setFont(u8g2_font_fub25_tf);

        if (timeBrewed < 10000.000) {
            u8g2.setCursor(64, 25);
        }
        else {
            u8g2.setCursor(48, 25);
        }

        u8g2.print(timeBrewed / 1000, 1);
        u8g2.setFont(u8g2_font_profont11_tf);
        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
    }

    /* if the totalBrewTime is reached automatically,
     * nothing should be done, otherwise wrong time is displayed
     * because the switch is pressed later than totalBrewTime
     */
    if (((machineState == kShotTimerAfterBrew) && SHOTTIMER == 1)) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(-1, 11, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);
        u8g2.setFont(u8g2_font_fub25_tf);

        if (lastBrewTime < 10000.000) {
            u8g2.setCursor(64, 25);
        }
        else {
            u8g2.setCursor(48, 25);
        }

        u8g2.print(lastBrewTime / 1000, 1);
        u8g2.setFont(u8g2_font_profont11_tf);
        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
    }

    #if (ONLYPIDSCALE == 1 || BREWMODE == 2)
        if ((machineState == kBrew) && SHOTTIMER == 2) {
            u8g2.clearBuffer();

            // temp icon
            u8g2.drawXBMP(-1, 11, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);
            u8g2.setFont(u8g2_font_profont22_tf);
            u8g2.setCursor(64, 15);
            u8g2.print(timeBrewed / 1000, 1);
            u8g2.print("s");
            u8g2.setCursor(64, 38);
            u8g2.print(weightBrew, 0);
            u8g2.print("g");
            u8g2.setFont(u8g2_font_profont11_tf);
            displayWaterIcon(119, 1);
            u8g2.sendBuffer();
        }

        if (((machineState == kShotTimerAfterBrew) && SHOTTIMER == 2)) {
            u8g2.clearBuffer();
            u8g2.drawXBMP(-1, 11, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);
            u8g2.setFont(u8g2_font_profont22_tf);
            u8g2.setCursor(64, 15);
            u8g2.print(lastBrewTime / 1000, 1);
            u8g2.print("g");
            u8g2.setCursor(64, 38);
            u8g2.print(weightBrew, 0);
            u8g2.print(" g");
            u8g2.setFont(u8g2_font_profont11_tf);
            displayWaterIcon(119, 1);
            u8g2.sendBuffer();
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

        displayStatusbar();

        // Rancilio logo
        if (HEATINGLOGO == 1) {
            u8g2.drawXBMP(-1, 15, Rancilio_Silvia_Logo_width, Rancilio_Silvia_Logo_height, Rancilio_Silvia_Logo);
            u8g2.drawXBMP(52, 15, Heating_Logo_width, Heating_Logo_height, Heating_Logo);
            u8g2.setFont(u8g2_font_profont22_tf);
        }

        // Gaggia Logo
        if (HEATINGLOGO == 2) {
            u8g2.drawXBMP(0, 14, Gaggia_Classic_Logo_width, Gaggia_Classic_Logo_height, Gaggia_Classic_Logo);
            u8g2.drawXBMP(53, 15, Heating_Logo_width, Heating_Logo_height, Heating_Logo);
            u8g2.setFont(u8g2_font_profont22_tf);
        }

        // Temperature
        u8g2.setCursor(92, 30);
        u8g2.setFont(u8g2_font_profont17_tf);
        u8g2.print(temperature, 1);
        u8g2.sendBuffer();
    }

    // Offline logo
    if (OFFLINEGLOGO == 1 && machineState == kPidOffline) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(38, 0, Off_Logo_width, Off_Logo_height, Off_Logo);
        u8g2.setCursor(0, 55);
        u8g2.setFont(u8g2_font_profont10_tf);
        u8g2.print("PID is disabled manually");
        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
    }

    if (OFFLINEGLOGO == 1 && machineState == kStandby) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(38, 0, Off_Logo_width, Off_Logo_height, Off_Logo);
        u8g2.setCursor(36, 55);
        u8g2.setFont(u8g2_font_profont10_tf);
        u8g2.print("Standby mode");
        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
    }

    // Steam
    if (machineState == kSteam) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(-1, 12, Steam_Logo_width, Steam_Logo_height, Steam_Logo);
        
        u8g2.setFont(u8g2_font_fub30_tf);

        if (temperature < 99.999) {
            u8g2.setCursor(68, 16);
            u8g2.print(temperature, 0);
        }
        else {
            u8g2.setCursor(48, 16);
            u8g2.print(temperature, 0);
        }
        
        u8g2.drawCircle(120, 20, 3);

        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
    }

    // Water empty
    if (machineState == kWaterEmpty) {
        u8g2.clearBuffer();
        u8g2.drawXBMP( 45, 0, Water_Empty_Logo_width, Water_Empty_Logo_height, Water_Empty_Logo); 
        u8g2.setFont(u8g2_font_profont11_tf);
        u8g2.sendBuffer();
    }

    // Backflush
    if (machineState == kBackflush) {
        u8g2.setFont(u8g2_font_profont11_tf);
        
        if (backflushState == 43) {
            #if OLED_DISPLAY != 0
                displayMessage(langstring_bckffinished[0], langstring_bckffinished[1], "", "", "", "");
            #endif
        } 
        else if (backflushState == 10) {
            #if OLED_DISPLAY != 0
                displayMessage(langstring_bckfactivated[0], langstring_bckfactivated[1], "", "", "", "");
            #endif
        } 
        else if (backflushState > 10) {
            #if OLED_DISPLAY != 0
                displayMessage(langstring_bckfrunning[0], String(flushCycles), langstring_bckfrunning[1], String(maxflushCycles), "", "");
            #endif
        }
    }

    // PID Off
    if (machineState == kEmergencyStop) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_profont11_tf);
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

        displayThermometerOutline(4, 58);

        // draw current temp in thermometer
        if (isrCounter < 500) {
            drawTemperaturebar(8, 46, 30);
            u8g2.setCursor(32, 4);
            u8g2.print("PID STOPPED");
        }

        displayWaterIcon(119, 1);

        u8g2.sendBuffer();
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
