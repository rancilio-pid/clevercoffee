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

        if (config.get<int>("display.template") == 4) {
            u8g2->setCursor(x + 12, y - 1);
        }
        else {
            u8g2->setCursor(x + 36, y - 1);
        }

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
    u8g2->setFont(u8g2_font_fub30_tn);

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
            if (checkBrewActive()) {
                currBrewTimerState = kBrewTimerRunning;
            }
            break;

        case kBrewTimerRunning:
            if (!checkBrewActive()) {
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
    u8g2->setDrawColor(0);

    if (config.get<int>("display.template") == 1) {
        u8g2->drawBox(x, y, 100, 15);
    }
    else {
        u8g2->drawBox(x, y + 1, 100, 10);
    }

    u8g2->setDrawColor(1);

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
    u8g2->setDrawColor(0);
    u8g2->drawBox(x, y + 1, 100, 10);
    u8g2->setDrawColor(1);

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
        u8g2->setFont(u8g2_font_fub20_tn);
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
        u8g2->setFont(u8g2_font_fub25_tn);

        if (brewtime < 9950.000) {
            u8g2->setCursor(x + 16, y);
        }
        else {
            u8g2->setCursor(x, y);
        }

        u8g2->print(brewtime / 1000, 1);
        u8g2->setFont(u8g2_font_profont12_tf);

        if (brewtime < 9950.000) {
            u8g2->setCursor(x + 67, y + 16);
        }
        else {
            u8g2->setCursor(x + 69, y + 16);
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

inline void displayBluetoothStatus(const int x, const int y) {
    if (scale) {
        if (const bool connected = scale->isConnected(); connected) {
            u8g2->drawXBMP(x, y, 8, 9, Bluetooth_Icon);
        }
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
        displayMQTTStatus(40, 0);
    }
    else {
        u8g2->setCursor(40, 0);
        u8g2->setFont(u8g2_font_profont11_tf);
        u8g2->print(langstring_offlinemode);
    }

    if (config.get<bool>("hardware.sensors.scale.enabled") && config.get<int>("hardware.sensors.scale.type") == 2) {
        displayBluetoothStatus(24, 1);
    }

    const auto format = "%02luh %02lum";
    displayUptime(84, 0, format);
}

/**
 * @brief test character char length and return the number of extra spaces required to print special characters
 */
int check_utf8_char(const String& utf, long i) {
    constexpr uint8_t ASCII_MASK = 0x80;    // 10000000 - top bit marks non-ASCII
    constexpr uint8_t ASCII_TAG = 0x00;     // 0xxxxxxx - ASCII start
    constexpr uint8_t LEADCHAR_MASK = 0xF0; // 11110000 - top 4 bits
    constexpr uint8_t LEAD2_MASK = 0xE0;    // 11100000 - top 3 bits
    constexpr uint8_t LEAD2_TAG = 0xC0;     // 110xxxxx - start of 2-byte sequence covers 0xC0 and 0xD0
    constexpr uint8_t LEAD3_MASK = 0xF0;    // 11110000 - top 4 bits
    constexpr uint8_t LEAD3_TAG = 0xE0;     // 1110xxxx - start of 3-byte sequence
    constexpr uint8_t LEAD4_MASK = 0xF8;    // 11111000 - top 5 bits
    constexpr uint8_t LEAD4_TAG = 0xF0;     // 11110xxx - start of 4-byte sequence
    constexpr uint8_t CONT_MASK = 0xC0;     // 11000000 - top 2 bits
    constexpr uint8_t CONT_TAG = 0x80;      // 10xxxxxx - continuation byte

    unsigned char leadChar = utf[i] & LEADCHAR_MASK;
    size_t len = utf.length();

    if ((leadChar & ASCII_MASK) == ASCII_TAG) {
        return 0; // ASCII (1 byte)
    }
    else if ((leadChar & LEAD2_MASK) == LEAD2_TAG && (i + 1) < len) {
        if ((utf[i + 1] & CONT_MASK) == CONT_TAG) {
            return 1; // 2 bytes
        }
    }
    else if ((leadChar & LEAD3_MASK) == LEAD3_TAG && (i + 2) < len) {
        if ((utf[i + 1] & CONT_MASK) == CONT_TAG && (utf[i + 2] & CONT_MASK) == CONT_TAG) {
            return 2; // 3 bytes
        }
    }
    else if ((leadChar & LEAD4_MASK) == LEAD4_TAG && (i + 3) < len) {
        if ((utf[i + 1] & CONT_MASK) == CONT_TAG && (utf[i + 2] & CONT_MASK) == CONT_TAG && (utf[i + 3] & CONT_MASK) == CONT_TAG) {
            return 3; // 4 bytes
        }
    }

    return 0;
}

/**
 * @brief generate multi line prints of text
 */
inline void displayWrappedMessage(const String& message, int x, int startY, int spacing, boolean clearSend, boolean wrapWord) {
    if (clearSend) {
        u8g2->clearBuffer();
    }

    if (config.get<int>("display.template") == 4) {
        u8g2->setFont(u8g2_font_profont10_tf);
    }
    else {
        u8g2->setFont(u8g2_font_profont11_tf);
    }

    int lineHeight = u8g2->getMaxCharHeight() + spacing;
    int charWidth = u8g2->getMaxCharWidth();
    int displayWidth = u8g2->getDisplayWidth();
    int displayHeight = u8g2->getDisplayHeight();

    int y = startY;
    int wordCount = 0;

    String newWord;
    String line;
    size_t sz;
    String chr;

    for (size_t i = 0; i <= message.length(); ++i) {
        char c = message[i];
        sz = check_utf8_char(message, i);
        chr = message.substring(i, i + sz + 1);

        if (sz == 0 && (c == ' ' || c == '\n' || c == '\0')) {
            if (u8g2->getUTF8Width((line + newWord).c_str()) > displayWidth) {

                if (wordCount == 0) {
                    u8g2->drawUTF8(x, y, newWord.c_str());
                    y += lineHeight;
                    line = "";
                }
                else {
                    u8g2->drawUTF8(x, y, line.c_str());
                    y += lineHeight;
                    line = newWord + " ";
                    wordCount = 1;
                }
            }
            else {
                line += newWord + " ";
                wordCount += 1;
            }

            newWord = "";

            if (c == '\n') {
                u8g2->drawUTF8(x, y, line.c_str());
                y += lineHeight;
                line = "";
                wordCount = 0;
            }
        }
        else {
            if (wrapWord && (u8g2->getUTF8Width((line + newWord).c_str()) > (displayWidth - charWidth))) {
                u8g2->drawUTF8(x, y, (line + newWord).c_str());
                y += lineHeight;
                line = "";
                newWord = "";
                wordCount = 0;
            }

            newWord += chr;
        }

        i += sz;
    }

    if (line.length() > 0) {
        u8g2->drawUTF8(x, y, line.c_str()); // print remaining
    }

    if (clearSend) {
        u8g2->sendBuffer();
    }
}

/**
 * @brief print logo and message at boot
 */
inline void displayLogo(const String& displaymessagetext, boolean wrap = false) {
    u8g2->clearBuffer();

    if (config.get<int>("display.template") == 4) {
        displayWrappedMessage(displaymessagetext, 0, 47, 2, false, wrap);
        u8g2->drawXBMP(11, 4, CleverCoffee_Logo_width, CleverCoffee_Logo_height, CleverCoffee_Logo);
    }
    else {
        u8g2->clearBuffer();
        displayWrappedMessage(displaymessagetext, 0, 42, 0, false, wrap);
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
                u8g2->setFont(u8g2_font_profont22_tr);
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
        }
        else {
            u8g2->drawXBMP(-1, 11, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);

            if (config.get<bool>("hardware.sensors.scale.enabled")) {
                u8g2->setFont(u8g2_font_profont22_tr);
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
        }
        else {
            u8g2->drawXBMP(0, 12, Manual_Flush_Logo_width, Manual_Flush_Logo_height, Manual_Flush_Logo);
            displayBrewtimeFs(48, 25, currBrewTime);
        }

        displayBufferReady = true;
        return true;
    }
    return false;
}

/**
 * @brief display fullscreen hot water on timer
 */
inline bool displayFullscreenHotWaterTimer() {
    if (!featureFullscreenHotWaterTimer) {
        return false;
    }

    if (machineState == kHotWater) {
        u8g2->clearBuffer();

        if (config.get<int>("display.template") == 4) {
            u8g2->drawXBMP(12, 12, Hot_Water_Logo_width, Hot_Water_Logo_height, Hot_Water_Logo);
            displayBrewtimeFs(1, 80, currPumpOnTime);
        }
        else {
            u8g2->drawXBMP(0, 12, Hot_Water_Logo_width, Hot_Water_Logo_height, Hot_Water_Logo);
            displayBrewtimeFs(48, 25, currPumpOnTime);
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
        displayWrappedMessage("Begin Fallback,\nNo Wifi");
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
        u8g2->setFont(u8g2_font_fub25_tn);
        u8g2->setCursor(50, 30);
        u8g2->print(temperature, 1);
        u8g2->drawCircle(122, 32, 3);

        u8g2->sendBuffer();
        return true;
    }

    // Offline logo
    if (machineState == kPidDisabled) {
        u8g2->clearBuffer();
        u8g2->drawXBMP(38, 0, Off_Logo_width, Off_Logo_height, Off_Logo);
        u8g2->setCursor(0, 55);
        u8g2->setFont(u8g2_font_profont10_tf);
        u8g2->print("PID is disabled manually");
        u8g2->sendBuffer();
        return true;
    }

    if (machineState == kStandby) {
        u8g2->clearBuffer();
        u8g2->drawXBMP(38, 0, Off_Logo_width, Off_Logo_height, Off_Logo);
        u8g2->setCursor(36, 55);
        u8g2->setFont(u8g2_font_profont10_tf);
        u8g2->print("Standby mode");
        u8g2->sendBuffer();
        return true;
    }

    // Steam
    if (machineState == kSteam) {
        u8g2->clearBuffer();
        u8g2->drawXBMP(-1, 12, Steam_Logo_width, Steam_Logo_height, Steam_Logo);

        displayTemperature(48, 16);

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
        u8g2->setFont(u8g2_font_fub17_tr);
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

            case kBackflushEnding:
                u8g2->setFont(u8g2_font_profont12_tf);
                u8g2->setCursor(4, 37);
                u8g2->print(langstring_backflush_press);
                u8g2->setCursor(4, 50);
                u8g2->print(langstring_backflush_finish);
                break;

            default:
                u8g2->setFont(u8g2_font_fub17_tr);
                u8g2->setCursor(42, 42);
                u8g2->print(currBackflushCycles, 0);
                u8g2->print("/");
                u8g2->print(backflushCycles, 0);
                break;
        }

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

        u8g2->sendBuffer();

        return true;
    }

    if (machineState == kSensorError) {
        displayWrappedMessage(String(langstring_error_tsensor[0]) + String(temperature) + '\n' + String(langstring_error_tsensor[1]));
        return true;
    }

    if (machineState == kEepromError) {
        displayWrappedMessage("EEPROM Error,\nPlease set values");
        return true;
    }

    return false;
}