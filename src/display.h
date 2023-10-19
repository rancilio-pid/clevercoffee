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
        u8g2.sendBuffer();
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
            } else {
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
                } else {
                    u8g2.setCursor(60, 2);
                    u8g2.print("");
                }
            }
        } else {
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
        u8g2.sendBuffer();
    }

    // Offline logo
    if (OFFLINEGLOGO == 1 && machineState == kPidOffline) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(38, 0, OFFLogo_width, OFFLogo_height, OFFLogo);
        u8g2.setCursor(0, 55);
        u8g2.setFont(u8g2_font_profont10_tf);
        u8g2.print("PID is disabled manually");
        u8g2.sendBuffer();
    }

    if (OFFLINEGLOGO == 1 && machineState == kStandby) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(38, 0, OFFLogo_width, OFFLogo_height, OFFLogo);
        u8g2.setCursor(36, 55);
        u8g2.setFont(u8g2_font_profont10_tf);
        u8g2.print("Standby mode");
        u8g2.sendBuffer();
    }

    // Steam
    if (machineState == kSteam) {
        u8g2.clearBuffer();
        u8g2.drawXBMP(0, 0, steamlogo_width, steamlogo_height, steamlogo);
        u8g2.setCursor(64, 25);
        u8g2.setFont(u8g2_font_profont22_tf);
        u8g2.print(temperature, 0);
        u8g2.setCursor(64, 25);
        u8g2.sendBuffer();
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

    // PID Off Logo
    if (machineState == kEmergencyStop) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_profont11_tf);
        u8g2.drawXBMP(0, 0, logo_width, logo_height, logo_bits_u8g2);  // draw temp icon
        u8g2.setCursor(32, 24);
        u8g2.print("Ist :  ");
        u8g2.print(temperature, 1);
        u8g2.print(" ");
        u8g2.print((char)176);
        u8g2.print("C");
        u8g2.setCursor(32, 34);
        u8g2.print("Soll:  ");
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
