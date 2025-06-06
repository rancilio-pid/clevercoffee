/**
 * @file displayRotateUpright.h
 *
 * @brief Display template based on the standard template but rotated 90 degrees
 *
 */

#pragma once

#if (OLED_DISPLAY == 1 || OLED_DISPLAY == 2)
/**
 * @brief initialize display
 */
void u8g2_prepare(void) {
    u8g2.setFont(u8g2_font_profont11_tf);
    u8g2.setFontRefHeightExtendedText();
    u8g2.setDrawColor(1);
    u8g2.setFontPosTop();
    u8g2.setFontDirection(0);
    u8g2.setDisplayRotation(DISPLAYROTATE); // either put U8G2_R1 or U8G2_R3 in userconfig
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
#endif

/**
 * @brief Draw a water empty icon at the given coordinates if water supply is low
 */
void displayWaterIcon(int x, int y) {
    if (!waterTankFull) {
        u8g2.drawXBMP(x, y, 8, 8, Water_Tank_Empty_Icon);
    }
}

/**
 * @brief Draw the brew time at given position (fullscreen brewtimer)
 */
void displayBrewtimeFs(int x, int y, double brewtime) {
    u8g2.setFont(u8g2_font_fub20_tf);
    if (brewtime < 10000.000) {
        u8g2.setCursor(x + 15, y);
    }
    else {
        u8g2.setCursor(x, y);
    }
    u8g2.print(brewtime / 1000, 1);
    u8g2.setFont(u8g2_font_profont15_tf);
    u8g2.setCursor(x, y + 25);
    u8g2.print("seconds");
    u8g2.setFont(u8g2_font_profont11_tf);
}

/**
 * @brief print error message for scales
 */
void displayScaleFailed(void) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 32, "Failed!");
    u8g2.drawStr(0, 42, "Scale");
    u8g2.drawStr(0, 52, "not");
    u8g2.drawStr(0, 62, "working...");
    u8g2.sendBuffer();
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
        u8g2.print(langstring_weight_rot_ur);
        u8g2.print(langstring_scale_Failure);
        return;
    }

    u8g2.setCursor(x, y);
    u8g2.print(langstring_weight_rot_ur);
    u8g2.print(weight, 0);

    if (setpoint > 0) {
        u8g2.print("/");
        u8g2.print(setpoint, 0);
    }

    u8g2.print(" g");
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
 * @brief print logo and message at boot
 */
void displayLogo(String displaymessagetext, String displaymessagetext2) {
    int printrow = 47;
    u8g2.clearBuffer();
    // Create modifiable copies
    char text1[displaymessagetext.length() + 1];
    char text2[displaymessagetext2.length() + 1];

    strcpy(text1, displaymessagetext.c_str());
    strcpy(text2, displaymessagetext2.c_str());

    char* token = strtok(text1, " ");
    while (token != NULL) {
        u8g2.drawStr(0, printrow, token);
        token = strtok(NULL, " "); // Get the next token
        printrow += 10;
    }
    token = strtok(text2, " ");
    while (token != NULL) {
        u8g2.drawStr(0, printrow, token);
        token = strtok(NULL, " "); // Get the next token
        printrow += 10;
    }
    u8g2.drawXBMP(11, 4, CleverCoffee_Logo_width, CleverCoffee_Logo_height, CleverCoffee_Logo);

    u8g2.sendBuffer();
}

#if 0 // not used a.t.m.
/**
 * @brief display emergency stop
 */
void displayEmergencyStop(void) {
    u8g2.clearBuffer();
    u8g2.setCursor(1, 34);
    u8g2.print(langstring_current_temp_rot_ur);
    u8g2.print(temperature, 1);
    u8g2.print(" ");
    u8g2.print((char)176);
    u8g2.print("C");
    u8g2.setCursor(1, 44);
    u8g2.print(langstring_set_temp_rot_ur);
    u8g2.print(setPoint, 1);
    u8g2.print(" ");
    u8g2.print((char)176);
    u8g2.print("C");

    if (isrCounter < 500) {
        u8g2.setCursor(1, 4);
        u8g2.print(langstring_emergencyStop[0]);
        u8g2.setCursor(1, 14);
        u8g2.print(langstring_emergencyStop[1]);
    }

    u8g2.sendBuffer();
}
#endif

/**
 * @brief display shot timer
 */
#if (FEATURE_BREWSWITCH == 1)
bool displayShottimer() {

    if (featureBrewControl) {
        // Shown brew time
        if (shouldDisplayBrewTimer()) {
            u8g2.clearBuffer();

            u8g2.drawXBMP(0, 0, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);
            u8g2.setFont(u8g2_font_profont22_tf);
            u8g2.setCursor(5, 70);
            u8g2.print(currBrewTime / 1000, 1);
            u8g2.setFont(u8g2_font_profont11_tf);
            u8g2.sendBuffer();
            return true;
        }
    }
    return false;
}
#endif

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
    if ((displayWidth == 0) || (displayWidth + x > SCREEN_HEIGHT)) {
        displayWidth = SCREEN_HEIGHT - x;
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
        u8g2.setCursor(5, 70);
        u8g2.print(currBrewTime / 1000, 1);
        u8g2.print("s");
        u8g2.setCursor(5, 100);
        u8g2.print(currBrewWeight, 1);
        u8g2.print("g");
        u8g2.setFont(u8g2_font_profont11_tf);
#else
        displayBrewtimeFs(1, 80, currBrewTime);
#endif
        displayWaterIcon(55, 1);
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
        displayBrewtimeFs(1, 80, currBrewTime);
        displayWaterIcon(55, 1);
        u8g2.sendBuffer();
        return true;
    }

    return false;
}