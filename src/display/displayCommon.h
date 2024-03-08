/**
 * @file displayCommon.h
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
    if (FEATURE_MQTT == 1) {
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
 * @brief Draw the temperature in big font at given position
 */
void displayTemperature(int x, int y) {
    u8g2.setFont(u8g2_font_fub30_tf);

    if (temperature < 99.999) {
        u8g2.setCursor(x+20, y);
        u8g2.print(temperature, 0);
    }
    else {
        u8g2.setCursor(x,y);
        u8g2.print(temperature, 0);
    }
    
    u8g2.drawCircle(x+72, y+4, 3);
}

/**
 * @brief Draw the brew time at given position
 */
void displayBrewtime(int x, int y, double brewtime) {
    u8g2.setFont(u8g2_font_fub25_tf);

    if (brewtime < 10000.000) {
        u8g2.setCursor(x+16, y);
    }
    else {
        u8g2.setCursor(x, y);
    }

    u8g2.print(brewtime / 1000, 1);
    u8g2.setFont(u8g2_font_profont15_tf);

    if (brewtime < 10000.000) {
        u8g2.setCursor(x+67, y+14);
    }
    else {
        u8g2.setCursor(x+69, y+14);
    }

    u8g2.print("s");
    u8g2.setFont(u8g2_font_profont11_tf);
}

/**
 * @brief Draw a bar visualizing the output in % at the given coordinates and with the given width
 */
void displayProgressbar(int value, int x, int y, int width) {
    u8g2.drawFrame(x, y, width, 4);
    int output = map(value, 0, 100, 0, width);

    if (output - 2 > 0) {
        u8g2.drawLine(x + 1, y + 1, x + output - 1, y + 1);
        u8g2.drawLine(x + 1, y + 2, x + output - 1, y + 2);
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
    u8g2.drawStr(0, 45, displaymessagetext.c_str());
    u8g2.drawStr(0, 55, displaymessagetext2.c_str());

    u8g2.drawXBMP(38, 0, CleverCoffee_Logo_width, CleverCoffee_Logo_height, CleverCoffee_Logo);

    u8g2.sendBuffer();
}

/**
 * @brief display shot timer
 */
void displayShottimer(void) {
    if (FEATURE_SHOTTIMER == 0) {
        return;
    }

    if ((machineState == kBrew || brewSwitchState == kBrewSwitchFlushOff) && SHOTTIMER_TYPE == 1) {
        u8g2.clearBuffer();

        if (brewSwitchState != kBrewSwitchFlushOff) {
            u8g2.drawXBMP(-1, 11, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);
        } 
        else {
            u8g2.drawXBMP( 0, 12, Manual_Flush_Logo_width, Manual_Flush_Logo_height, Manual_Flush_Logo);
        }

        displayBrewtime(48, 25, timeBrewed);

        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
    }

    /* if the totalBrewTime is reached automatically,
     * nothing should be done, otherwise wrong time is displayed
     * because the switch is pressed later than totalBrewTime
     */
    if ((machineState == kShotTimerAfterBrew && brewSwitchState != kBrewSwitchFlushOff) && SHOTTIMER_TYPE == 1) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(-1, 11, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);

        displayBrewtime(48, 25, lastBrewTime);

        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
    }

    #if FEATURE_SCALE == 1
        if ((machineState == kBrew) && SHOTTIMER_TYPE == 2) {
            u8g2.clearBuffer();

            // temp icon
            u8g2.drawXBMP(-1, 11, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);
            u8g2.setFont(u8g2_font_profont22_tf);
            u8g2.setCursor(64, 15);
            u8g2.print(timeBrewed / 1000, 1);
            u8g2.print("s");
            u8g2.setCursor(64, 38);
            u8g2.print(weightBrew, 1);
            u8g2.print("g");
            u8g2.setFont(u8g2_font_profont11_tf);
            displayWaterIcon(119, 1);
            u8g2.sendBuffer();
        }

        if (((machineState == kShotTimerAfterBrew) && SHOTTIMER_TYPE == 2)) {
            u8g2.clearBuffer();
            u8g2.drawXBMP(-1, 11, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);
            u8g2.setFont(u8g2_font_profont22_tf);
            u8g2.setCursor(64, 15);
            u8g2.print(lastBrewTime / 1000, 1);
            u8g2.print("s");
            u8g2.setCursor(64, 38);
            u8g2.print(weightBrew, 1);
            u8g2.print("g");
            u8g2.setFont(u8g2_font_profont11_tf);
            displayWaterIcon(119, 1);
            u8g2.sendBuffer();
        }
    #endif  
}


/**
 * @brief display heating logo
 */
void displayMachineState() {
    if (FEATURE_HEATINGLOGO > 0 && (machineState == kInit || machineState == kColdStart) && brewSwitchState != kBrewSwitchFlushOff) {
        // For status info
        u8g2.clearBuffer();

        displayStatusbar();

        u8g2.drawXBMP(0, 20, Heating_Logo_width, Heating_Logo_height, Heating_Logo);
        u8g2.setFont(u8g2_font_fub25_tf);
        u8g2.setCursor(50, 30);
        u8g2.print(temperature, 1);
        u8g2.drawCircle(122, 32, 3);

        u8g2.sendBuffer();
    }

    // Offline logo
    if (FEATURE_OFFLINELOGO == 1 && machineState == kPidOffline) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(38, 0, Off_Logo_width, Off_Logo_height, Off_Logo);
        u8g2.setCursor(0, 55);
        u8g2.setFont(u8g2_font_profont10_tf);
        u8g2.print("PID is disabled manually");
        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
    }

    if (FEATURE_OFFLINELOGO == 1 && machineState == kStandby) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(38, 0, Off_Logo_width, Off_Logo_height, Off_Logo);
        u8g2.setCursor(36, 55);
        u8g2.setFont(u8g2_font_profont10_tf);
        u8g2.print("Standby mode");
        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
    }

    // Steam
    if (machineState == kSteam && brewSwitchState != kBrewSwitchFlushOff) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(-1, 12, Steam_Logo_width, Steam_Logo_height, Steam_Logo);
        
        displayTemperature(48, 16);

        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
    }

    // Water empty
    if (machineState == kWaterEmpty && brewSwitchState != kBrewSwitchFlushOff) {
        u8g2.clearBuffer();
        u8g2.drawXBMP( 45, 0, Water_Empty_Logo_width, Water_Empty_Logo_height, Water_Empty_Logo); 
        u8g2.setFont(u8g2_font_profont11_tf);
        u8g2.sendBuffer();
    }

    // Backflush
    if (machineState == kBackflush) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_fub17_tf);
        u8g2.setCursor(2, 10);
        u8g2.print("Backflush");

        switch (backflushState) {
            case kBackflushWaitBrewswitchOn:
                u8g2.setFont(u8g2_font_profont12_tf);
                u8g2.setCursor(4, 37);
                u8g2.print(langstring_backflush_press);
                u8g2.setCursor(4, 50);
                u8g2.print(langstring_backflush_start);
                break;

            case kBackflushWaitBrewswitchOff:
                u8g2.setFont(u8g2_font_profont12_tf);
                u8g2.setCursor(4, 37);
                u8g2.print(langstring_backflush_press);
                u8g2.setCursor(4, 50);
                u8g2.print(langstring_backflush_finish);
                break;

            default:
                u8g2.setFont(u8g2_font_fub17_tf);
                u8g2.setCursor(42, 42);
                u8g2.print(flushCycles + 1, 0);
                u8g2.print("/");
                u8g2.print(maxflushCycles, 0);
                break;
        }

        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
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
