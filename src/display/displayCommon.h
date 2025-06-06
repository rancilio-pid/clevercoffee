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
    if (!waterTankFull) {
        u8g2.drawXBMP(x, y, 8, 8, Water_Tank_Empty_Icon);
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

        for (int b = 0; b <= getSignalStrength(); b++) {
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

    if (temperature < 99.499) {
        u8g2.setCursor(x + 20, y);
        u8g2.print(temperature, 0);
    }
    else {
        u8g2.setCursor(x, y);
        u8g2.print(temperature, 0);
    }

    u8g2.drawCircle(x + 72, y + 4, 3);
}

/**
 * @brief determines if brew timer should be visible; postBrewTimerDuration defines how long the timer after the brew is shown
 * @return true if timer should be visible, false otherwise
 */
bool shouldDisplayBrewTimer() {

    enum BrewTimerState {
        kBrewTimerIdle = 10,
        kBrewTimerRunning = 20,
        kBrewTimerPostBrew = 30
    };

    static BrewTimerState currBrewTimerState = kBrewTimerIdle;

    static uint32_t brewEndTime = 0;

    switch (currBrewTimerState) {
        case kBrewTimerIdle:
            if (brew()) {
                currBrewTimerState = kBrewTimerRunning;
            }
            break;

        case kBrewTimerRunning:
            if (!brew()) {
                currBrewTimerState = kBrewTimerPostBrew;
                brewEndTime = millis();
            }
            break;

        case kBrewTimerPostBrew:
            if ((millis() - brewEndTime) > (uint32_t)(postBrewTimerDuration * 1000)) {
                currBrewTimerState = kBrewTimerIdle;
            }
            break;
    }

    return (currBrewTimerState != kBrewTimerIdle);
}

/**
 * @brief Draw current brew time with optional brew target time at given position
 *
 * Shows the current brew time in seconds. If a target time (totalTargetBrewTime) is provided (> 0), it is displayed alongside the current time.
 *
 * @param x              Horizontal position to start drawing
 * @param y              Vertical position to start drawing
 * @param label          Text label to display before the time
 * @param currBrewTime     Current brewed time in milliseconds
 * @param totalTargetBrewTime  Target brew time in milliseconds (optional, default -1)
 */
void displayBrewTime(int x, int y, const char* label, double currBrewTime, double totalTargetBrewTime = -1) {
    u8g2.setCursor(x, y);
    u8g2.print(label);
    u8g2.setCursor(x + 50, y);
    u8g2.print(currBrewTime / 1000, 0);

    if (totalTargetBrewTime > 0) {
        u8g2.print("/");
        u8g2.print(totalTargetBrewTime / 1000, 0);
    }

    u8g2.print(" s");
}

/**
 * @brief Draw the current weight with error handling and target indicators at given position
 *
 * If the scale reports an error, "fault" is shown on the display instead of weight.
 * Otherwise, the function displays the current weight.
 * If a target weight (setpoint) is set (> 0), it will be displayed alongside the current weight.
 * This function is intended to provide the status of the scales during brewing, flushing, or other machine states.
 *
 * @param x        Horizontal position to start drawing
 * @param y        Vertical position to start drawing
 * @param weight   Current measured weight to display
 * @param setpoint Target weight to display alongside current weight (optional, default -1)
 * @param fault    Indicates if the scale has an error (optional, default false)
 */
void displayBrewWeight(int x, int y, float weight, float setpoint = -1, bool fault = false) {
    if (fault) {
        u8g2.setCursor(x, y);
        u8g2.print(langstring_weight);
        u8g2.setCursor(x + 50, y);
        u8g2.print(langstring_scale_Failure);
        return;
    }

    u8g2.setCursor(x, y);
    u8g2.print(langstring_weight);
    u8g2.setCursor(x + 50, y);
    u8g2.print(weight, 0);

    if (setpoint > 0) {
        u8g2.print("/");
        u8g2.print(setpoint, 0);
    }

    u8g2.print(" g");
}

/**
 * @brief Draw the brew time at given position (fullscreen brewtimer)
 */
void displayBrewtimeFs(int x, int y, double brewtime) {
    u8g2.setFont(u8g2_font_fub25_tf);

    if (brewtime < 10000.000) {
        u8g2.setCursor(x + 16, y);
    }
    else {
        u8g2.setCursor(x, y);
    }

    u8g2.print(brewtime / 1000, 1);
    u8g2.setFont(u8g2_font_profont15_tf);

    if (brewtime < 10000.000) {
        u8g2.setCursor(x + 67, y + 14);
    }
    else {
        u8g2.setCursor(x + 69, y + 14);
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
 * @brief print error message for scales
 */
void displayScaleFailed(void) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 32, "failed!");
    u8g2.drawStr(0, 42, "Scale not working..."); // scale timeout will most likely trigger after OTA update, but will still work after boot
    u8g2.sendBuffer();
}

/**
 * @brief scroll a message based on a provided size
 */
void displayScrollingSubstring(int x, int y, int displayWidth, const char* text, bool bounce) {
    static int offset = 0;
    static int direction = 1;
    static unsigned long lastUpdate = 0;
    static unsigned long interval = 100;
    static const char* lastText = nullptr;

    if (text == nullptr) return;

    // Reset if text has changed
    if (lastText != text) {
        offset = 0;
        direction = 1;
        lastText = text;
        interval = 500;
        lastUpdate = millis();
    }
    else if (offset > 0) {
        interval = 100;
    }

    int textLen = strlen(text);
    int fullTextWidth = u8g2.getStrWidth(text);

    // limit display width
    if ((displayWidth == 0) || (displayWidth + x > SCREEN_WIDTH)) {
        displayWidth = SCREEN_WIDTH - x;
    }

    // No scrolling needed
    if (fullTextWidth <= displayWidth) {
        u8g2.drawStr(x, y, text);
        return;
    }

    // Scroll logic
    if (millis() - lastUpdate > interval) {
        lastUpdate = millis();
        if (bounce) {
            offset += direction;
            if (offset < 0 || u8g2.getStrWidth(&text[offset]) < displayWidth) {
                direction = -direction;
                offset += direction;
            }
        }
        else {
            offset++;
            if (offset >= u8g2.getStrWidth(&text[offset])) {
                offset = 0;
            }
        }
    }

    // Determine how many characters fit
    int visibleWidth = 0;
    int end = offset;
    while (end < textLen && visibleWidth < displayWidth) {
        char buf[2] = {text[end], '\0'};
        visibleWidth += u8g2.getStrWidth(buf);
        if (visibleWidth > displayWidth) break;
        end++;
    }

    // Copy visible substring
    char visible[64] = {0};
    strncpy(visible, &text[offset], end - offset);
    visible[end - offset] = '\0';

    u8g2.drawStr(x, y, visible);
}

/**
 * @brief display fullscreen brew timer
 */
bool displayFullscreenBrewTimer() {
    if (featureFullscreenBrewTimer == 0) {
        return false;
    }

    if (shouldDisplayBrewTimer()) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(-1, 11, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);
#if (FEATURE_SCALE == 1)
        u8g2.setFont(u8g2_font_profont22_tf);
        u8g2.setCursor(64, 15);
        u8g2.print(currBrewTime / 1000, 1);
        u8g2.print("s");
        u8g2.setCursor(64, 38);
        u8g2.print(currBrewWeight, 1);
        u8g2.print("g");
        u8g2.setFont(u8g2_font_profont11_tf);
#else
        displayBrewtimeFs(48, 25, currBrewTime);
#endif
        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
        return true;
    }

    return false;
}

/**
 * @brief display fullscreen manual flush timer
 */
bool displayFullscreenManualFlushTimer() {
    if (featureFullscreenManualFlushTimer == 0) {
        return false;
    }

    if (machineState == kManualFlush) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(0, 12, Manual_Flush_Logo_width, Manual_Flush_Logo_height, Manual_Flush_Logo);
        displayBrewtimeFs(48, 25, currBrewTime);
        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
        return true;
    }

    return false;
}

/**
 * @brief display heating logo
 */
bool displayMachineState() {
    // Show the heating logo when we are in regular PID mode and more than 5degC below the set point
    if (featureHeatingLogo > 0 && (machineState == kPidNormal || machineState == kSteam) && (setpoint - temperature) > 5.) {
        // For status info
        u8g2.clearBuffer();

        displayStatusbar();

        u8g2.drawXBMP(0, 20, Heating_Logo_width, Heating_Logo_height, Heating_Logo);
        u8g2.setFont(u8g2_font_fub25_tf);
        u8g2.setCursor(50, 30);
        u8g2.print(temperature, 1);
        u8g2.drawCircle(122, 32, 3);

        u8g2.sendBuffer();
        return true;
    }
    // Offline logo
    else if (featurePidOffLogo == 1 && machineState == kPidDisabled) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(38, 0, Off_Logo_width, Off_Logo_height, Off_Logo);
        u8g2.setCursor(0, 55);
        u8g2.setFont(u8g2_font_profont10_tf);
        u8g2.print("PID is disabled manually");
        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
        return true;
    }
    else if (featurePidOffLogo == 1 && machineState == kStandby) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(38, 0, Off_Logo_width, Off_Logo_height, Off_Logo);
        u8g2.setCursor(36, 55);
        u8g2.setFont(u8g2_font_profont10_tf);
        u8g2.print("Standby mode");
        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
        return true;
    }
    // Steam
    else if (machineState == kSteam) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(-1, 12, Steam_Logo_width, Steam_Logo_height, Steam_Logo);

        displayTemperature(48, 16);

        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
        return true;
    }
    // Water empty
    else if (machineState == kWaterTankEmpty) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(45, 0, Water_Tank_Empty_Logo_width, Water_Tank_Empty_Logo_height, Water_Tank_Empty_Logo);
        u8g2.setFont(u8g2_font_profont11_tf);
        u8g2.sendBuffer();
        return true;
    }
    // Backflush
    else if (machineState == kBackflush) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_fub17_tf);
        u8g2.setCursor(2, 10);
        u8g2.print("Backflush");

        switch (currBackflushState) {
            case kBackflushIdle:
                u8g2.setFont(u8g2_font_profont12_tf);
                u8g2.setCursor(4, 37);
                u8g2.print(langstring_backflush_press);
                u8g2.setCursor(4, 50);
                u8g2.print(langstring_backflush_start);
                break;

            case kBackflushFinished:
                u8g2.setFont(u8g2_font_profont12_tf);
                u8g2.setCursor(4, 37);
                u8g2.print(langstring_backflush_press);
                u8g2.setCursor(4, 50);
                u8g2.print(langstring_backflush_finish);
                break;

            default:
                u8g2.setFont(u8g2_font_fub17_tf);
                u8g2.setCursor(42, 42);
                u8g2.print(currBackflushCycles, 0);
                u8g2.print("/");
                u8g2.print(backflushCycles, 0);
                break;
        }

        displayWaterIcon(119, 1);
        u8g2.sendBuffer();
        return true;
    }
    // PID Off
    else if (machineState == kEmergencyStop) {
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
        return true;
    }
    else if (machineState == kSensorError) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_profont11_tf);
        displayMessage(langstring_error_tsensor[0], String(temperature), langstring_error_tsensor[1], "", "", "");
        return true;
    }
    else if (machineState == kEepromError) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_profont11_tf);
        displayMessage("EEPROM Error, please set Values", "", "", "", "", "");
        return true;
    }

    return false;
}
#endif
