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
    if ((machineState == kBrew) && FEATURE_SHOTTIMER == 1 && SHOTTIMER_TYPE == 1) {
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
    if (((machineState == kShotTimerAfterBrew) && FEATURE_SHOTTIMER == 1 && SHOTTIMER_TYPE == 1)) {
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
        if ((machineState == kBrew) && FEATURE_SHOTTIMER == 1 && SHOTTIMER_TYPE == 2) {
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

        if (((machineState == kShotTimerAfterBrew) && FEATURE_SHOTTIMER == 1 && SHOTTIMER_TYPE == 2)) {
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

#if (ROTARY_MENU == 1) 

void displayNumericalMenuSettingWithUnit(double temp, const char* name, const char* unit) {
    u8g2.clearBuffer();
    u8g2.setCursor(0, 0);
    u8g2.print(name);
    u8g2.print(": ");
    u8g2.setCursor(0, 10);
    u8g2.print(temp, 1);
    u8g2.print(unit);
    u8g2.setCursor(0, 25);
    u8g2.println(langstring_pressToSave[0]);
    u8g2.setCursor(0, 35);
    u8g2.println(langstring_pressToSave[1]);
    u8g2.sendBuffer();
}

void displayToggleBackflushMessage(uint8_t mode) {
    u8g2.clearBuffer();
    u8g2.setCursor(0, 0);
    u8g2.print(LANGSTRING_MENU_BACKFLUSH);
    switch (mode) {
        case 1:
            u8g2.print(LANGSTRING_MENU_ON);
            break;
        default:
            u8g2.print(LANGSTRING_MENU_OFF);
            break;
    }
    u8g2.setCursor(0, 25);
    u8g2.println(langstring_autoclose[0]);
    u8g2.setCursor(0, 35);
    u8g2.println(langstring_autoclose[1]);
    u8g2.sendBuffer();
}

void displayMenu() {
    #if ROTARY_MENU_DEBUG // output menu to serial 
    // init vars
    //uint8_t n_max = (LCDML.MENU_getChilds() >= _LCDML_DISP_rows) ? _LCDML_DISP_rows : (LCDML.MENU_getChilds());

    // update content
    // ***************
    if (LCDML.DISP_checkMenuUpdate() || LCDML.DISP_checkMenuCursorUpdate() ) {
        // clear menu
        // ***************
        LCDML.DISP_clear();

        debugPrintln(F("==========================================="));
        debugPrintln(F("================  Menu ===================="));
        debugPrintln(F("==========================================="));

        // declaration of some variables
        // ***************
        // content variable
        char content_text[_LCDML_DISP_cols];  // save the content text of every menu element
        // menu element object
        LCDMenuLib2_menu *tmp;
        // some limit values
        uint8_t i = LCDML.MENU_getScroll();
        uint8_t maxi = (_LCDML_DISP_rows) + i;
        uint8_t n = 0;

        // check if this element has children
        if ((tmp = LCDML.MENU_getDisplayedObj()) != NULL)
        {

        // loop to display lines
        do
        {
            // check if a menu element has a condition and if the condition be true
            if (tmp->checkCondition())
            {
            // display cursor
            if (n == LCDML.MENU_getCursorPos())
            {
                debugPrint(F("(x) "));
            }
            else
            {
                debugPrint(F("( ) "));
            }

            // check the type off a menu element
            if(tmp->checkType_menu() == true)
            {
                // display normal content
                LCDML_getContent(content_text, tmp->getID());
                debugPrint(content_text);
            }
            else
            {
                if(tmp->checkType_dynParam()) {
                tmp->callback(n);
                }
            }

            debugPrintln("");

            // increment some values
            i++;
            n++;
            }
        // try to go to the next sibling and check the number of displayed rows
        } while (((tmp = tmp->getSibling(1)) != NULL) && (i < maxi));
        }
    }

    #endif


    char content_text[_LCDML_DISP_cols];  // save the content text of every menu element
    LCDMenuLib2_menu *tmp;
    uint8_t i = LCDML.MENU_getScroll();
    uint8_t maxi = _LCDML_DISP_rows + i;
    uint8_t n = 0;

    // init vars
    uint8_t n_max             = (LCDML.MENU_getChilds() >= _LCDML_DISP_rows) ? _LCDML_DISP_rows : (LCDML.MENU_getChilds());

    uint8_t scrollbar_min     = 0;
    uint8_t scrollbar_max     = LCDML.MENU_getChilds();
    uint8_t scrollbar_cur_pos = LCDML.MENU_getCursorPosAbs();
    uint8_t scroll_pos        = ((1.*n_max * _LCDML_DISP_rows) / (scrollbar_max - 1) * scrollbar_cur_pos);

    // generate content
    u8g2.firstPage();
    do {
        n = 0;
        i = LCDML.MENU_getScroll();
        // check if this element has children
        if ((tmp = LCDML.MENU_getDisplayedObj()) != NULL) {
        // loop to display lines
        do {
            // check if a menu element has a condition and if the condition be true
            if (tmp->checkCondition()) {
            // check the type off a menu element
            if(tmp->checkType_menu() == true) {
                // display normal content
                LCDML_getContent(content_text, tmp->getID());
                u8g2.drawStr( _LCDML_DISP_box_x0+_LCDML_DISP_font_w + _LCDML_DISP_cur_space_behind, _LCDML_DISP_box_y0 + _LCDML_DISP_font_h * (n + 1), content_text);
            }
            else {
                if(tmp->checkType_dynParam()) {
                tmp->callback(n);
                }
            }
            // increment some values
            i++;
            n++;
            }
        // try to go to the next sibling and check the number of displayed rows
        } while (((tmp = tmp->getSibling(1)) != NULL) && (i < maxi));
        }

        // set cursor
        u8g2.drawStr( _LCDML_DISP_box_x0+_LCDML_DISP_cur_space_before, _LCDML_DISP_box_y0 + _LCDML_DISP_font_h * (LCDML.MENU_getCursorPos() + 1),  _LCDML_DISP_cursor_char);

        if(_LCDML_DISP_draw_frame == 1) {
        u8g2.drawFrame(_LCDML_DISP_box_x0, _LCDML_DISP_box_y0, (_LCDML_DISP_box_x1-_LCDML_DISP_box_x0), (_LCDML_DISP_box_y1-_LCDML_DISP_box_y0));
        }

        // display scrollbar when more content as rows available and with > 2
        if (scrollbar_max > n_max && _LCDML_DISP_scrollbar_w > 2)
        {
        // set frame for scrollbar
        u8g2.drawFrame(_LCDML_DISP_box_x1 - _LCDML_DISP_scrollbar_w, _LCDML_DISP_box_y0, _LCDML_DISP_scrollbar_w, _LCDML_DISP_box_y1-_LCDML_DISP_box_y0);

        // calculate scrollbar length
        uint8_t scrollbar_block_length = scrollbar_max - n_max;
        scrollbar_block_length = (_LCDML_DISP_box_y1-_LCDML_DISP_box_y0) / (scrollbar_block_length + _LCDML_DISP_rows);

        //set scrollbar
        if (scrollbar_cur_pos == 0) {                                   // top position     (min)
            u8g2.drawBox(_LCDML_DISP_box_x1 - (_LCDML_DISP_scrollbar_w-1), _LCDML_DISP_box_y0 + 1                                                     , (_LCDML_DISP_scrollbar_w-2)  , scrollbar_block_length);
        }
        else if (scrollbar_cur_pos == (scrollbar_max-1)) {            // bottom position  (max)
            u8g2.drawBox(_LCDML_DISP_box_x1 - (_LCDML_DISP_scrollbar_w-1), _LCDML_DISP_box_y1 - scrollbar_block_length                                , (_LCDML_DISP_scrollbar_w-2)  , scrollbar_block_length);
        }
        else {                                                                // between top and bottom
            u8g2.drawBox(_LCDML_DISP_box_x1 - (_LCDML_DISP_scrollbar_w-1), _LCDML_DISP_box_y0 + (scrollbar_block_length * scrollbar_cur_pos + 1),(_LCDML_DISP_scrollbar_w-2)  , scrollbar_block_length);
        }
        }
    } while ( u8g2.nextPage() );
}
#endif

/**
 * @brief display heating logo
 */
void Displaymachinestate() {
    if (FEATURE_HEATINGLOGO > 0 && (machineState == kInit || machineState == kColdStart)) {
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
