/**
 * @file displayCommon.h
 *
 * @brief Common functions for all display templates
 */

#pragma once

#include "bitmaps.h"
#include "languages.h"

inline const u8g2_cb_t* getU8G2Rotation(const int rotationValue) {
    switch (rotationValue) {
        case 0:
            return U8G2_R0;
        case 1:
            return U8G2_R1;
        case 2:
            return U8G2_R2;
        case 3:
            return U8G2_R3;
        default:
            return U8G2_R0;
    }
}

/**
 * @brief initialize display
 */
inline void u8g2_prepare() {
    int rotation = 0;
    u8g2->clearBuffer();
    u8g2->setFont(u8g2_font_profont11_tf);
    u8g2->setFontRefHeightExtendedText();
    u8g2->setDrawColor(1);
    u8g2->setFontPosTop();
    u8g2->setFontDirection(0);

    if (config.get<bool>("display.inverted")) {
        rotation += 2;
    }

    if (config.get<int>("display.template") == 4) {
        rotation++;
    }

    u8g2->setDisplayRotation(getU8G2Rotation(rotation));
}

/**
 * @brief print error message for scales
 */
inline void displayScaleFailed() {
    if (config.get<int>("display.template") == 4) {
        u8g2->clearBuffer();
        u8g2->drawStr(0, 32, "Failed!");
        u8g2->drawStr(0, 42, "Scale");
        u8g2->drawStr(0, 52, "not");
        u8g2->drawStr(0, 62, "working...");
        u8g2->sendBuffer();
    }
    else {
        u8g2->clearBuffer();
        u8g2->drawStr(0, 32, "failed!");
        u8g2->drawStr(0, 42, "Scale not working..."); // scale timeout will most likely trigger after OTA update, but will still work after boot
        u8g2->sendBuffer();
    }
}

/**
 * @brief Draw a water empty icon at the given coordinates if water supply is low
 */
inline void displayWaterIcon(const int x, const int y) {
    if (!waterTankFull) {
        u8g2->drawXBMP(x, y, 8, 8, Water_Tank_Empty_Icon);
    }
}

/**
 * @brief Draw the system uptime at the given coordinates
 */
inline void displayUptime(const int x, const int y, const char* format) {
    // Show uptime of machine
    unsigned long seconds = millis() / 1000;
    const unsigned long hours = seconds / 3600;
    const unsigned long minutes = seconds % 3600 / 60;
    seconds = seconds % 60;

    char uptimeString[9];
    snprintf(uptimeString, sizeof(uptimeString), format, hours, minutes, seconds);

    u8g2->setFont(u8g2_font_profont11_tf);
    u8g2->drawStr(x, y, uptimeString);
}

/**
 * @brief Draw a WiFi signal strength indicator at the given coordinates
 */
inline void displayWiFiStatus(const int x, const int y) {
    if (WiFi.status() == WL_CONNECTED) {
        u8g2->drawXBMP(x, y, 8, 8, Antenna_OK_Icon);

        for (int b = 0; b <= getSignalStrength(); b++) {
            u8g2->drawVLine(x + 5 + b * 2, y + 8 - b * 2, b * 2);
        }
    }
    else {
        u8g2->drawXBMP(x, y, 8, 8, Antenna_NOK_Icon);
        u8g2->setCursor(x + 12, y - 1);
        u8g2->setFont(u8g2_font_profont11_tf);
        u8g2->print("RC: ");
        u8g2->print(wifiReconnects);
    }
}

/**
 * @brief Draw an MQTT status indicator at the given coordinates if MQTT is enabled
 */
inline void displayMQTTStatus(const int x, const int y) {
    if (mqtt_enabled) {
        if (mqtt.connected() == 1) {
            u8g2->setCursor(x, y);
            u8g2->setFont(u8g2_font_profont11_tf);
            u8g2->print("MQTT");
            if (getSignalStrength() <= 1) {
                u8g2->print("!");
            }
        }
        else {
            u8g2->setCursor(x, y);
            u8g2->print("");
        }
    }
}

/**
 * @brief Draw the outline of a thermometer for use in conjunction with the drawTemperaturebar method
 */
inline void displayThermometerOutline(const int x, const int y) {
    u8g2->drawLine(x + 3, y - 9, x + 3, y - 42);
    u8g2->drawLine(x + 9, y - 9, x + 9, y - 42);
    u8g2->drawPixel(x + 4, y - 43);
    u8g2->drawPixel(x + 8, y - 43);
    u8g2->drawLine(x + 5, y - 44, x + 7, y - 44);
    u8g2->drawDisc(x + 6, y - 5, 6);

    // draw setpoint line
    const int height = map(static_cast<int>(setpoint), 0, 100, y - 9, y - 39);
    u8g2->drawLine(x + 11, height, x + 16, height);
}

/**
 * @brief Draw temperature bar, e.g. inside the thermometer outline.
 *        Add 4 pixels to the x-coordinate and subtract 12 pixels from the y-coordinate of the thermometer.
 */
inline void drawTemperaturebar(const int x, const int heightRange) {
    const int width = x + 5;

    for (int i = x; i < width; i++) {
        const int height = map(static_cast<int>(temperature), 0, 100, 0, heightRange);
        u8g2->drawVLine(i, 52 - height, height);
    }

    if (temperature > 100) {
        u8g2->drawLine(x, heightRange - 11, x + 3, heightRange - 11);
        u8g2->drawLine(x, heightRange - 10, x + 4, heightRange - 10);
        u8g2->drawLine(x, heightRange - 9, x + 4, heightRange - 9);
    }
}

/**
 * @brief Draw the temperature in big font at given position
 */
inline void displayTemperature(const int x, const int y) {
    u8g2->setFont(u8g2_font_fub30_tf);

    if (temperature < 99.499) {
        u8g2->setCursor(x + 20, y);
        u8g2->print(temperature, 0);
    }
    else {
        u8g2->setCursor(x, y);
        u8g2->print(temperature, 0);
    }

    u8g2->drawCircle(x + 72, y + 4, 3);
}

/**
 * @brief determines if brew timer should be visible; postBrewTimerDuration defines how long the timer after the brew is shown
 * @return true if timer should be visible, false otherwise
 */
inline bool shouldDisplayBrewTimer() {

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
            if (millis() - brewEndTime > static_cast<uint32_t>(postBrewTimerDuration * 1000)) {
                currBrewTimerState = kBrewTimerIdle;
            }
            break;
    }

    return currBrewTimerState != kBrewTimerIdle;
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
inline void displayBrewTime(const int x, const int y, const char* label, const double currBrewTime, const double totalTargetBrewTime = -1) {
    if (config.get<int>("display.template") == 4) {
        u8g2->setCursor(x, y);
        u8g2->print(label);
        u8g2->print(currBrewTime / 1000, 0);

        if (totalTargetBrewTime > 0) {
            u8g2->print("/");
            u8g2->print(totalTargetBrewTime / 1000, 0);
        }
        u8g2->print(" s");
    }
    else {
        u8g2->setCursor(x, y);
        u8g2->print(label);
        u8g2->setCursor(x + 50, y);
        u8g2->print(currBrewTime / 1000, 0);

        if (totalTargetBrewTime > 0) {
            u8g2->print("/");
            u8g2->print(totalTargetBrewTime / 1000, 0);
        }

        u8g2->print(" s");
    }
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
inline void displayBrewWeight(const int x, const int y, const float weight, const float setpoint = -1, const bool fault = false) {
    if (config.get<int>("display.template") == 4) {
        if (fault) {
            u8g2->setCursor(x, y);
            u8g2->print(langstring_weight_ur);
            u8g2->print(langstring_scale_Failure);
            return;
        }

        u8g2->setCursor(x, y);
        u8g2->print(langstring_weight_ur);
        u8g2->print(weight, 0);

        if (setpoint > 0) {
            u8g2->print("/");
            u8g2->print(setpoint, 0);
        }

        u8g2->print(" g");
    }
    else {
        if (fault) {
            u8g2->setCursor(x, y);
            u8g2->print(langstring_weight);
            u8g2->setCursor(x + 50, y);
            u8g2->print(langstring_scale_Failure);
            return;
        }

        u8g2->setCursor(x, y);
        u8g2->print(langstring_weight);
        u8g2->setCursor(x + 50, y);
        u8g2->print(weight, 0);

        if (setpoint > 0) {
            u8g2->print("/");
            u8g2->print(setpoint, 0);
        }

        u8g2->print(" g");
    }
}

/**
 * @brief Draw the brew time at given position (fullscreen brewtimer)
 */
inline void displayBrewtimeFs(const int x, const int y, const double brewtime) {
    if (config.get<int>("display.template") == 4) {
        u8g2->setFont(u8g2_font_fub20_tf);
        if (brewtime < 9950.000) {
            u8g2->setCursor(x + 15, y);
        }
        else {
            u8g2->setCursor(x, y);
        }
        u8g2->print(brewtime / 1000, 1);
        u8g2->setFont(u8g2_font_profont11_tf);
        u8g2->setCursor(x + 56, y + 12);
        u8g2->print("s");
    }
    else {
        u8g2->setFont(u8g2_font_fub25_tf);

        if (brewtime < 9950.000) {
            u8g2->setCursor(x + 16, y);
        }
        else {
            u8g2->setCursor(x, y);
        }

        u8g2->print(brewtime / 1000, 1);
        u8g2->setFont(u8g2_font_profont15_tf);

        if (brewtime < 9950.000) {
            u8g2->setCursor(x + 67, y + 14);
        }
        else {
            u8g2->setCursor(x + 69, y + 14);
        }

        u8g2->print("s");
    }

    u8g2->setFont(u8g2_font_profont11_tf);
}

/**
 * @brief Draw a bar visualizing the output in % at the given coordinates and with the given width
 */
inline void displayProgressbar(const int value, const int x, const int y, const int width) {
    u8g2->drawFrame(x, y, width, 4);

    if (const int output = map(value, 0, 100, 0, width); output - 2 > 0) {
        u8g2->drawLine(x + 1, y + 1, x + output - 1, y + 1);
        u8g2->drawLine(x + 1, y + 2, x + output - 1, y + 2);
    }
}

/**
 * @brief Draw a status bar at the top of the screen with icons for WiFi, MQTT,
 *        the system uptime and a separator line underneath
 */
inline void displayStatusbar() {
    // For status info
    u8g2->drawLine(0, 12, 128, 12);

    if (!offlineMode) {
        displayWiFiStatus(4, 1);
        displayMQTTStatus(38, 0);
    }
    else {
        u8g2->setCursor(4, 0);
        u8g2->setFont(u8g2_font_profont11_tf);
        u8g2->print(langstring_offlinemode);
    }

    const auto format = "%02luh %02lum";
    displayUptime(84, 0, format);
}

/**
 * @brief print message
 */
inline void displayMessage(const String& text1, const String& text2, const String& text3, const String& text4, const String& text5, const String& text6) {
    u8g2->clearBuffer();
    u8g2->setCursor(0, 0);
    u8g2->print(text1);
    u8g2->setCursor(0, 10);
    u8g2->print(text2);
    u8g2->setCursor(0, 20);
    u8g2->print(text3);
    u8g2->setCursor(0, 30);
    u8g2->print(text4);
    u8g2->setCursor(0, 40);
    u8g2->print(text5);
    u8g2->setCursor(0, 50);
    u8g2->print(text6);
    u8g2->sendBuffer();
}

/**
 * @brief print logo and message at boot
 */
inline void displayLogo(const String& displaymessagetext, const String& displaymessagetext2) {
    if (config.get<int>("display.template") == 4) {
        int printrow = 47;
        u8g2->clearBuffer();

        // Create modifiable copies
        char text1[displaymessagetext.length() + 1];
        char text2[displaymessagetext2.length() + 1];

        strcpy(text1, displaymessagetext.c_str());
        strcpy(text2, displaymessagetext2.c_str());

        char* token = strtok(text1, " ");

        while (token != nullptr) {
            u8g2->drawStr(0, printrow, token);
            token = strtok(nullptr, " "); // Get the next token
            printrow += 10;
        }

        token = strtok(text2, " ");

        while (token != nullptr) {
            u8g2->drawStr(0, printrow, token);
            token = strtok(nullptr, " "); // Get the next token
            printrow += 10;
        }

        u8g2->drawXBMP(11, 4, CleverCoffee_Logo_width, CleverCoffee_Logo_height, CleverCoffee_Logo);
    }
    else {
        u8g2->clearBuffer();
        u8g2->drawStr(0, 45, displaymessagetext.c_str());
        u8g2->drawStr(0, 55, displaymessagetext2.c_str());
        u8g2->drawXBMP(38, 0, CleverCoffee_Logo_width, CleverCoffee_Logo_height, CleverCoffee_Logo);
    }

    u8g2->sendBuffer();
}

/**
 * @brief display fullscreen brew timer
 */
inline bool displayFullscreenBrewTimer() {
    if (!featureFullscreenBrewTimer) {
        return false;
    }

    if (shouldDisplayBrewTimer()) {
        u8g2->clearBuffer();

        if (config.get<int>("display.template") == 4) {
            u8g2->drawXBMP(12, 12, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);

            if (config.get<bool>("hardware.sensors.scale.enabled")) {
                u8g2->setFont(u8g2_font_profont22_tf);
                u8g2->setCursor(5, 70);
                u8g2->print(currBrewTime / 1000, 1);
                u8g2->print("s");
                u8g2->setCursor(5, 100);
                u8g2->print(currBrewWeight, 1);
                u8g2->print("g");
                u8g2->setFont(u8g2_font_profont11_tf);
            }
            else {
                displayBrewtimeFs(1, 80, currBrewTime);
            }

            displayWaterIcon(55, 2);
        }
        else {
            u8g2->drawXBMP(-1, 11, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);

            if (config.get<bool>("hardware.sensors.scale.enabled")) {
                u8g2->setFont(u8g2_font_profont22_tf);
                u8g2->setCursor(64, 15);
                u8g2->print(currBrewTime / 1000, 1);
                u8g2->print("s");
                u8g2->setCursor(64, 38);
                u8g2->print(currBrewWeight, 1);
                u8g2->print("g");
                u8g2->setFont(u8g2_font_profont11_tf);
            }
            else {
                displayBrewtimeFs(48, 25, currBrewTime);
            }

            displayWaterIcon(119, 1);
        }

        displayBufferReady = true;
        return true;
    }

    return false;
}

/**
 * @brief display fullscreen manual flush timer
 */
inline bool displayFullscreenManualFlushTimer() {
    if (!featureFullscreenManualFlushTimer) {
        return false;
    }

    if (machineState == kManualFlush) {
        u8g2->clearBuffer();

        if (config.get<int>("display.template") == 4) {
            u8g2->drawXBMP(12, 12, Manual_Flush_Logo_width, Manual_Flush_Logo_height, Manual_Flush_Logo);
            displayBrewtimeFs(1, 80, currBrewTime);
            displayWaterIcon(55, 2);
        }
        else {
            u8g2->drawXBMP(0, 12, Manual_Flush_Logo_width, Manual_Flush_Logo_height, Manual_Flush_Logo);
            displayBrewtimeFs(48, 25, currBrewTime);
            displayWaterIcon(119, 1);
        }

        displayBufferReady = true;
        return true;
    }
    return false;
}

/**
 * @brief display offline message
 */
inline bool displayOfflineMode() {

    if (displayOffline > 0 && displayOffline < 20) {
        displayMessage("", "", "", "", "Begin Fallback,", "No Wifi");
        displayOffline++;
        return true;
    }

    return false;
}

/**
 * @brief display heating logo
 */
inline bool displayMachineState() {

    if (displayOfflineMode()) {
        return true;
    }

    // Show the heating logo when we are in regular PID mode and more than 5degC below the set point
    if (featureHeatingLogo > 0 && (machineState == kPidNormal || machineState == kSteam) && setpoint - temperature > 5.) {
        // For status info
        u8g2->clearBuffer();

        displayStatusbar();

        u8g2->drawXBMP(0, 20, Heating_Logo_width, Heating_Logo_height, Heating_Logo);
        u8g2->setFont(u8g2_font_fub25_tf);
        u8g2->setCursor(50, 30);
        u8g2->print(temperature, 1);
        u8g2->drawCircle(122, 32, 3);

        u8g2->sendBuffer();
        return true;
    }

    // Offline logo
    if (featurePidOffLogo == 1 && machineState == kPidDisabled) {
        u8g2->clearBuffer();
        u8g2->drawXBMP(38, 0, Off_Logo_width, Off_Logo_height, Off_Logo);
        u8g2->setCursor(0, 55);
        u8g2->setFont(u8g2_font_profont10_tf);
        u8g2->print("PID is disabled manually");
        displayWaterIcon(119, 1);
        u8g2->sendBuffer();
        return true;
    }

    if (featurePidOffLogo == 1 && machineState == kStandby) {
        u8g2->clearBuffer();
        u8g2->drawXBMP(38, 0, Off_Logo_width, Off_Logo_height, Off_Logo);
        u8g2->setCursor(36, 55);
        u8g2->setFont(u8g2_font_profont10_tf);
        u8g2->print("Standby mode");
        displayWaterIcon(119, 1);
        u8g2->sendBuffer();
        return true;
    }

    // Steam
    if (machineState == kSteam) {
        u8g2->clearBuffer();
        u8g2->drawXBMP(-1, 12, Steam_Logo_width, Steam_Logo_height, Steam_Logo);

        displayTemperature(48, 16);

        displayWaterIcon(119, 1);
        u8g2->sendBuffer();
        return true;
    }

    // Water empty
    if (machineState == kWaterTankEmpty) {
        u8g2->clearBuffer();
        u8g2->drawXBMP(45, 0, Water_Tank_Empty_Logo_width, Water_Tank_Empty_Logo_height, Water_Tank_Empty_Logo);
        u8g2->setFont(u8g2_font_profont11_tf);
        u8g2->sendBuffer();
        return true;
    }

    // Backflush
    if (machineState == kBackflush) {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_fub17_tf);
        u8g2->setCursor(2, 10);
        u8g2->print("Backflush");

        switch (currBackflushState) {
            case kBackflushIdle:
                u8g2->setFont(u8g2_font_profont12_tf);
                u8g2->setCursor(4, 37);
                u8g2->print(langstring_backflush_press);
                u8g2->setCursor(4, 50);
                u8g2->print(langstring_backflush_start);
                break;

            case kBackflushFinished:
                u8g2->setFont(u8g2_font_profont12_tf);
                u8g2->setCursor(4, 37);
                u8g2->print(langstring_backflush_press);
                u8g2->setCursor(4, 50);
                u8g2->print(langstring_backflush_finish);
                break;

            default:
                u8g2->setFont(u8g2_font_fub17_tf);
                u8g2->setCursor(42, 42);
                u8g2->print(currBackflushCycles, 0);
                u8g2->print("/");
                u8g2->print(backflushCycles, 0);
                break;
        }

        displayWaterIcon(119, 1);
        u8g2->sendBuffer();
        return true;
    }

    // PID Off
    if (machineState == kEmergencyStop) {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_profont11_tf);
        u8g2->setCursor(32, 24);
        u8g2->print(langstring_current_temp);
        u8g2->print(temperature, 1);
        u8g2->print(" ");
        u8g2->print(static_cast<char>(176));
        u8g2->print("C");
        u8g2->setCursor(32, 34);
        u8g2->print(langstring_set_temp);
        u8g2->print(setpoint, 1);
        u8g2->print(" ");
        u8g2->print(static_cast<char>(176));
        u8g2->print("C");

        displayThermometerOutline(4, 58);

        // draw current temp in thermometer
        if (isrCounter < 500) {
            drawTemperaturebar(8, 30);
            u8g2->setCursor(32, 4);
            u8g2->print("PID STOPPED");
        }

        displayWaterIcon(119, 1);

        u8g2->sendBuffer();

        return true;
    }

    if (machineState == kSensorError) {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_profont11_tf);
        displayMessage(langstring_error_tsensor[0], String(temperature), langstring_error_tsensor[1], "", "", "");
        return true;
    }

    if (machineState == kEepromError) {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_profont11_tf);
        displayMessage("EEPROM Error, please set Values", "", "", "", "", "");
        return true;
    }

    return false;
}
