/**
 * @file displayCommon.h
 *
 * @brief Common functions for all display templates
 */

#pragma once

#include "DisplayManager.h"
extern DisplayManager display;  // declare the extern DisplayManager object to use the same instance everywhere

#if (DISPLAY_HARDWARE != 0)

/**
 * @brief Draw a water empty icon at the given coordinates if water supply is low
 */
void displayWaterIcon(int x, int y) {
    if (!waterFull) {
        display.drawImage(x, y, 8, 8, Water_Empty_Icon);
    }
}

/**
 * @brief Draw the system uptime at the given coordinates
 */
void displayUptime(int x, int y, const char* format) {
    // Show uptime of machine
    unsigned long seconds = millis() / 1000;
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;

    char uptimeString[9];
    snprintf(uptimeString, sizeof(uptimeString), format, hours, minutes, seconds);

    display.setFont(FontType::Normal);
    display.setCursor(x, y);
    display.print(uptimeString);
}

/**
 * @brief Draw a WiFi signal strength indicator at the given coordinates
 */
void displayWiFiStatus(int x, int y) {
    getSignalStrength();

    if (WiFi.status() == WL_CONNECTED) {
        display.drawImage(x, y, 8, 8, Antenna_OK_Icon);

        for (int b = 0; b <= getSignalStrength(); b++) {
            display.drawVLine(x + 5 + (b * 2), y + 8 - (b * 2), b * 2);
        }
    }
    else {
        display.drawImage(x, y, 8, 8, Antenna_NOK_Icon);
        display.setCursor(x + 5, y + 10);
        display.setFont(FontType::Normal);
        display.print("RC: ");
        display.print(wifiReconnects);
    }
}

/**
 * @brief Draw an MQTT status indicator at the given coordinates if MQTT is enabled
 */
void displayMQTTStatus(int x, int y) {
    if (FEATURE_MQTT == 1) {
        if (mqtt.connected() == 1) {
            display.setCursor(x, y);
            display.setFont(FontType::Normal);
            display.print("MQTT");
        }
        else {
            display.setCursor(x, y);
            display.print("");
        }
    }
}

/**
 * @brief Draw the outline of a thermometer for use in conjunction with the drawTemperaturebar method
 */
void displayThermometerOutline(int x, int y) {
    display.drawLine(x + 3, y - 9, x + 3, y - 42);
    display.drawLine(x + 9, y - 9, x + 9, y - 42);
    display.drawPixel(x + 4, y - 43);
    display.drawPixel(x + 8, y - 43);
    display.drawLine(x + 5, y - 44, x + 7, y - 44);
    display.drawDisc(x + 6, y - 5, 6);

    // draw setpoint line
    int height = map(setpoint, 0, 100, y - 9, y - 39);
    display.drawLine(x + 11, height, x + 16, height);
}

/**
 * @brief Draw temperature bar, e.g. inside the thermometer outline.
 *        Add 4 pixels to the x-coordinate and subtract 12 pixels from the y-coordinate of the thermometer.
 */
void drawTemperaturebar(int x, int y, int heightRange) {
    int width = x + 5;

    for (int i = x; i < width; i++) {
        int height = map(temperature, 0, 100, 0, heightRange);
        display.drawVLine(i, 52 - height, height);
    }

    if (temperature > 100) {
        display.drawLine(x, heightRange - 11, x + 3, heightRange - 11);
        display.drawLine(x, heightRange - 10, x + 4, heightRange - 10);
        display.drawLine(x, heightRange - 9, x + 4, heightRange - 9);
    }
}

/**
 * @brief Draw the temperature in big font at given position
 */
void displayTemperature(int x, int y) {
    display.setFont(FontType::fup25);

    if (temperature < 99.999) {
        display.setCursor(x + 20, y);
        display.print(temperature, 0);
    }
    else {
        display.setCursor(x, y);
        display.print(temperature, 0);
    }

    display.drawCircle(x + 72, y + 4, 3);
}

/**
 * @brief Draw the brew time at given position
 */
void displayBrewtime(int x, int y, double brewtime) {
    display.setFont(FontType::fup25);

    if (brewtime < 10000.000) {
        display.setCursor(x + 16, y);
    }
    else {
        display.setCursor(x, y);
    }

    display.print(brewtime / 1000, 1);
    display.setFont(FontType::fup25);

    if (brewtime < 10000.000) {
        display.setCursor(x + 67, y + 14);
    }
    else {
        display.setCursor(x + 69, y + 14);
    }

    display.print("s");
    display.setFont(FontType::Normal);
}

/**
 * @brief Draw a bar visualizing the output in % at the given coordinates and with the given width
 */
void displayProgressbar(int value, int x, int y, int width) {
    display.drawFrame(x, y, width, 4);
    int output = map(value, 0, 100, 0, width);

    if (output - 2 > 0) {
        display.drawLine(x + 1, y + 1, x + output - 1, y + 1);
        display.drawLine(x + 1, y + 2, x + output - 1, y + 2);
    }
}

/**
 * @brief Draw a status bar at the top of the screen with icons for WiFi, MQTT,
 *        the system uptime and a separator line underneath
 */
void displayStatusbar() {
    // For status info
    display.drawLine(0, 12, 128, 12);

    if (offlineMode == 0) {
        displayWiFiStatus(4, 1);
        displayMQTTStatus(38, 0);
    }
    else {
        display.setCursor(4, 0);
        display.setFont(FontType::Normal);
        display.print(langstring_offlinemode);
    }

    const char* format = "%02luh %02lum";
    displayUptime(84, 0, format);
}

/**
 * @brief print message
 */
void displayMessage(String text1, String text2, String text3, String text4, String text5, String text6) {
    display.clearBuffer();
    display.setCursor(0, 0);
    display.println(text1);
    display.setCursor(0, 10);
    display.println(text2);
    display.setCursor(0, 20);
    display.println(text3);
    display.setCursor(0, 30);
    display.println(text4);
    display.setCursor(0, 40);
    display.println(text5);
    display.setCursor(0, 50);
    display.println(text6);
    display.sendBuffer();
}

/**
 * @brief print logo and message at boot
 */
void displayLogo(String displaymessagetext, String displaymessagetext2) {
    display.clearBuffer();
    display.setCursor(0, 45);
    display.print(displaymessagetext.c_str());
    display.setCursor(0, 55);
    display.print(displaymessagetext2.c_str());

    display.drawImage(38, 0, CleverCoffee_Logo_width, CleverCoffee_Logo_height, CleverCoffee_Logo);

    display.sendBuffer();
}

/**
 * @brief display shot timer
 */
bool displayShottimer() {
    if (FEATURE_SHOTTIMER == 0) {
        return false;
    }

    if (machineState == kBrew || brewSwitchState == kBrewSwitchFlushOff) {
        display.clearBuffer();

        if (brewSwitchState != kBrewSwitchFlushOff) {
            display.drawImage(-1, 11, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);
        }
        else {
            display.drawImage(0, 12, Manual_Flush_Logo_width, Manual_Flush_Logo_height, Manual_Flush_Logo);
        }

#if (FEATURE_SCALE == 1)
        display.setFont(u8g2_font_profont22_tf);
        display.setCursor(64, 15);
        display.print(timeBrewed / 1000, 1);
        display.print("s");
        display.setCursor(64, 38);
        display.print(weightBrew, 1);
        display.print("g");
        display.setFont(u8g2_font_profont11_tf);
#else
        displayBrewtime(48, 25, timeBrewed);
#endif

        displayWaterIcon(119, 1);
        display.sendBuffer();
        return true;
    }

    /* if the totalBrewTime is reached automatically,
     * nothing should be done, otherwise wrong time is displayed
     * because the switch is pressed later than totalBrewTime
     */
    else if (machineState == kShotTimerAfterBrew && brewSwitchState != kBrewSwitchFlushOff) {
        display.clearBuffer();
        display.drawXBMP(-1, 11, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);

#if (FEATURE_SCALE == 1)
        display.setFont(u8g2_font_profont22_tf);
        display.setCursor(64, 15);
        display.print(lastBrewTime / 1000, 1);
        display.print("s");
        display.setCursor(64, 38);
        display.print(weightBrew, 1);
        display.print("g");
        display.setFont(u8g2_font_profont11_tf);
#else
        displayBrewtime(48, 25, lastBrewTime);
#endif

        displayWaterIcon(119, 1);
        display.sendBuffer();
        return true;
    }
    return false;
}

/**
 * @brief display heating logo
 */
bool displayMachineState() {
    // Show the heating logo when we are in regular PID mode and more than 5degC below the set point
    if (FEATURE_HEATINGLOGO > 0 && machineState == kPidNormal && (setpoint - temperature) > 5. && brewSwitchState != kBrewSwitchFlushOff) {
        // For status info
        display.clearBuffer();

        displayStatusbar();

        display.drawImage(0, 20, Heating_Logo_width, Heating_Logo_height, Heating_Logo);
        display.setFont(FontType::fup25);
        display.setCursor(50, 30);
        display.print(temperature, 1);
        display.drawCircle(122, 32, 3);

        display.sendBuffer();
        return true;
    }
    // Offline logo
    else if (FEATURE_PIDOFF_LOGO == 1 && machineState == kPidDisabled) {
        display.clearBuffer();
        display.drawImage(38, 0, Off_Logo_width, Off_Logo_height, Off_Logo);
        display.setCursor(0, 55);
        display.setFont(FontType::Normal);
        display.print("PID is disabled manually");
        displayWaterIcon(119, 1);
        display.sendBuffer();
        return true;
    }
    else if (FEATURE_PIDOFF_LOGO == 1 && machineState == kStandby) {
        display.clearBuffer();
        display.drawImage(38, 0, Off_Logo_width, Off_Logo_height, Off_Logo);
        display.setCursor(36, 55);
        display.setFont(FontType::Normal);
        display.print("Standby mode");
        displayWaterIcon(119, 1);
        display.sendBuffer();
        return true;
    }
    // Steam
    else if (machineState == kSteam && brewSwitchState != kBrewSwitchFlushOff) {
        display.clearBuffer();
        display.drawImage(-1, 12, Steam_Logo_width, Steam_Logo_height, Steam_Logo);

        displayTemperature(48, 16);

        displayWaterIcon(119, 1);
        display.sendBuffer();
        return true;
    }
    // Water empty
    else if (machineState == kWaterEmpty && brewSwitchState != kBrewSwitchFlushOff) {
        display.clearBuffer();
        display.drawImage(45, 0, Water_Empty_Logo_width, Water_Empty_Logo_height, Water_Empty_Logo);
        display.setFont(FontType::Normal);
        display.sendBuffer();
        return true;
    }
    // Backflush
    else if (machineState == kBackflush) {
        display.clearBuffer();
        display.setFont(FontType::fup17);
        display.setCursor(2, 10);
        display.print("Backflush");

        switch (backflushState) {
            case kBackflushWaitBrewswitchOn:
                display.setFont(FontType::Normal);
                display.setCursor(4, 37);
                display.print(langstring_backflush_press);
                display.setCursor(4, 50);
                display.print(langstring_backflush_start);
                break;

            case kBackflushWaitBrewswitchOff:
                display.setFont(FontType::Normal);
                display.setCursor(4, 37);
                display.print(langstring_backflush_press);
                display.setCursor(4, 50);
                display.print(langstring_backflush_finish);
                break;

            default:
                display.setFont(u8g2_font_fub17_tf);
                display.setCursor(42, 42);
                display.print(currBackflushCycles + 1, 0);
                display.print("/");
                display.print(backflushCycles, 0);
                break;
        }

        displayWaterIcon(119, 1);
        display.sendBuffer();
        return true;
    }
    // PID Off
    else if (machineState == kEmergencyStop) {
        display.clearBuffer();
        display.setFont(FontType::Normal);
        display.setCursor(32, 24);
        display.print(langstring_current_temp);
        display.print(temperature, 1);
        display.print(" ");
        display.print((char)176);
        display.print("C");
        display.setCursor(32, 34);
        display.print(langstring_set_temp);
        display.print(setpoint, 1);
        display.print(" ");
        display.print((char)176);
        display.print("C");

        displayThermometerOutline(4, 58);

        // draw current temp in thermometer
        if (isrCounter < 500) {
            drawTemperaturebar(8, 46, 30);
            display.setCursor(32, 4);
            display.print("PID STOPPED");
        }

        displayWaterIcon(119, 1);

        display.sendBuffer();
        return true;
    }
    else if (machineState == kSensorError) {
        display.clearBuffer();
        display.setFont(FontType::Normal);
        displayMessage(langstring_error_tsensor[0], String(temperature), langstring_error_tsensor[1], "", "", "");
        return true;
    }
    else if (machineState == kEepromError) {
        display.clearBuffer();
        display.setFont(FontType::Normal);
        displayMessage("EEPROM Error, please set Values", "", "", "", "", "");
        return true;
    }

    return false;
}
#endif
