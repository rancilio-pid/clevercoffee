/**
 * @file menu.h
 * 
 * @brief Display menu for rotary encoder 
 */
#pragma once

void displayMenu();
void clearMenu();
void menuControls();
void changeNumericalValue(uint8_t param, double value, sto_item_id_t name, const char* readableName);
void changeBrewTemp(uint8_t param);
void changeSteamTemp(uint8_t param);
void changeBrewTime(uint8_t param);
void changePreinfusionTime(uint8_t param);
void changePreinfusionPauseTime(uint8_t param);
void switchBackflush(uint8_t param);
void menuBack(uint8_t param);
void menuClose(uint8_t param);
void setupMenu();

LCDMenuLib2_menu LCDML_0(255, 0, 0, NULL, NULL);
LCDMenuLib2 LCDML(LCDML_0, _LCDML_DISP_rows, _LCDML_DISP_cols, displayMenu, clearMenu, menuControls);

double menuRotaryLast = 0;
double initialValue = 0;
int last = 0;

void changeNumericalSetup(double value, const char* readableName, const char* unit) {
    if(LCDML.FUNC_setup()) {
        menuRotaryLast = encoder.getCount() / ENCODER_CLICKS_PER_NOTCH;
        initialValue = value;

        displayNumericalMenuSettingWithUnit(initialValue, readableName, unit);
    }
}

void changeNumericalLoop(const char* readableName, const char* unit) {
    int32_t pos = encoder.getCount() / ENCODER_CLICKS_PER_NOTCH;
    double diff = static_cast<double>(pos - menuRotaryLast) / 10.0;

    if (diff != 0) {
        initialValue = initialValue + diff;
        displayNumericalMenuSettingWithUnit(initialValue, readableName, unit);
        menuRotaryLast = pos;
    }
}

int saveToStorage(sto_item_id_t name) {
    storageSet(name, initialValue);
    return storageCommit();
}

void changeNumericalValue(double* param, double value, sto_item_id_t name, const char* readableName, const char* unit) {
    changeNumericalSetup(value, readableName, unit);

    if(LCDML.FUNC_loop()) {
        changeNumericalLoop(readableName, unit);
            
        if(LCDML.BT_checkEnter()) { 
            if (saveToStorage(name) == 0) {
                *param = initialValue;

                #if ROTARY_MENU_DEBUG == 1
                    debugPrintln("SAVED.");
                #endif

                LCDML.FUNC_goBackToMenu();
            } 
            else {
                #if ROTARY_MENU_DEBUG == 1
                    debugPrintln("error.");
                #endif
            }
        }
    }
}

void changeBrewTemp(uint8_t param) {
    changeNumericalValue(&brewSetpoint, brewSetpoint, STO_ITEM_BREW_SETPOINT, LANGSTRING_MENU_BREWSETPOINT, "C");
}

void changeSteamTemp(uint8_t param) {
    changeNumericalValue(&steamSetpoint, steamSetpoint, STO_ITEM_STEAM_SETPOINT, LANGSTRING_MENU_BREWSETPOINT, "C");
}

void changeTempOffset(uint8_t param) {
    changeNumericalValue(&brewTempOffset, brewTempOffset, STO_ITEM_BREW_TEMP_OFFSET, LANGSTRING_MENU_TEMP_OFFSET, "C");
}

void changeBrewTime(uint8_t param) {
    changeNumericalValue(&brewTime, brewTime, STO_ITEM_BREW_TIME, LANGSTRING_MENU_BREWTIME, "s");
}

void changePreinfusionTime(uint8_t param) {
    changeNumericalValue(&preinfusion, preinfusion, STO_ITEM_PRE_INFUSION_TIME, LANGSTRING_MENU_PREINFUSIONTIME, "s");
}

void changePreinfusionPauseTime(uint8_t param) {
    changeNumericalValue(&preinfusionPause, preinfusionPause, STO_ITEM_PRE_INFUSION_PAUSE, LANGSTRING_MENU_PREINFUSIONPAUSETIME, "s");
}

void changeTargetWeight(uint8_t param) {
    changeNumericalValue(&weightSetpoint, weightSetpoint, STO_ITEM_WEIGHTSETPOINT, LANGSTRING_MENU_WEIGHTSETPOINT, "g");
}

void changeStandbyTime(uint8_t param) {
    changeNumericalValue(&standbyModeTime, standbyModeTime, STO_ITEM_STANDBY_MODE_TIME, LANGSTRING_MENU_STANDBY_TIMER_TIME, "min");
}

void changePidKp(uint8_t param) {
    changeNumericalValue(&aggKp, aggKp, STO_ITEM_PID_KP_REGULAR, LANGSTRING_MENU_PID_KP, "");
}
void changePidTn(uint8_t param) {
    changeNumericalValue(&aggTn, aggTn, STO_ITEM_PID_TN_REGULAR, LANGSTRING_MENU_PID_TN, "");
}

void changePidTv(uint8_t param) {
    changeNumericalValue(&aggTv, aggbTv, STO_ITEM_PID_TV_REGULAR, LANGSTRING_MENU_PID_TV, "");
}

void changePidIMax(uint8_t param) {
    changeNumericalValue(&aggIMax, aggIMax, STO_ITEM_PID_I_MAX_REGULAR, LANGSTRING_MENU_PID_I_MAX, "");
}

void changeSteamKp(uint8_t param) {
    changeNumericalValue(&steamKp, steamKp, STO_ITEM_PID_KP_STEAM, LANGSTRING_MENU_STEAM_KP, "");
}

void changePonMKp(uint8_t param) {
    changeNumericalValue(&startKp, startKp, STO_ITEM_PID_KP_START, LANGSTRING_MENU_START_KP, "");
    
}
void changePonMTn(uint8_t param) {
    changeNumericalValue(&startTn, startTn, STO_ITEM_PID_TN_START, LANGSTRING_MENU_START_TN, "");
}

void changeBDDelay(uint8_t param) {
    changeNumericalValue(&brewPIDDelay, brewPIDDelay, STO_ITEM_BREW_PID_DELAY, LANGSTRING_MENU_PID_BD_DELAY, "s");
}

void changeBDKp(uint8_t param) {
    changeNumericalValue(&aggbKp, aggbKp, STO_ITEM_PID_KP_BD, LANGSTRING_MENU_PID_BD_KP, "");
}

void changeBDTn(uint8_t param) {
    changeNumericalValue(&aggbTn, aggbTn, STO_ITEM_PID_TN_BD, LANGSTRING_MENU_PID_BD_TN, "");
}

void changeBDTv(uint8_t param) {
    changeNumericalValue(&aggbTv, aggbTv, STO_ITEM_PID_TV_BD, LANGSTRING_MENU_PID_BD_TV, "");
}

void changeBDTime(uint8_t param) {
    changeNumericalValue(&brewtimesoftware, brewtimesoftware, STO_ITEM_BREW_SW_TIME, LANGSTRING_MENU_PID_BD_TIME, "s");
}

void changeBDSensitivity(uint8_t param) {
    changeNumericalValue(&brewSensitivity, brewSensitivity, STO_ITEM_BD_THRESHOLD, LANGSTRING_MENU_PID_BD_SENSITIVITY, "C");
}

void switchBoolean(uint8_t* flag, uint8_t param, const char* title, sto_item_id_t storageName) {
    if(LCDML.FUNC_setup()) {
        param = flipUintValue(param);
        int saved = storageSet(storageName, param, true);

        if (saved == 0) {
            *flag = param;
            #if ROTARY_MENU_DEBUG == 1
                debugPrintf("Saved %s: %d!\n", title, param);
            #endif
        } else {
            #if ROTARY_MENU_DEBUG == 1
                debugPrintf("Could not save %s: %d\n", title, saved);
            #endif
        }

        // update menu to re-evaluate conditions for showing/hiding menu entries
        LCDML.MENU_allCondetionRefresh();
        // if we do not go back to the previous level there are off-by-one cursor issues.... no idea for a fix at the moment
        LCDML.FUNC_goBackToMenu(1);
    }
}

// Special handling for backflush as this setting is not persisted
void switchBackflush(uint8_t* flag, uint8_t param, const char* title) {
    if(LCDML.FUNC_setup()) {
        param = flipUintValue(param);
        *flag = param;
        if (param == 1) {
            displayToggleMessage(title, param);
            delay(2000);
            menuOpen = false;
        }
        LCDML.FUNC_goBackToMenu(1);      
    }
}

void toggleBackflush(uint8_t param) {
    switchBackflush(&backflushOn, backflushOn, LANGSTRING_MENU_BACKFLUSH);
}

void toggleStandbyTimer(uint8_t param) {
    switchBoolean(&standbyModeOn, standbyModeOn, LANGSTRING_MENU_STANDBY_TIMER, STO_ITEM_STANDBY_MODE_ON);
}

void toggleUsePonM(uint8_t param) {
    switchBoolean(&usePonM, usePonM, LANGSTRING_MENU_START_USE_PONM, STO_ITEM_PID_START_PONM);
}

void toggleUseBrewPid(uint8_t param) {
    switchBoolean(&useBDPID, useBDPID, LANGSTRING_MENU_PID_BD_ON, STO_ITEM_USE_BD_PID);
}

void menuBack(uint8_t param) {
    if(LCDML.FUNC_setup()) {
        LCDML.FUNC_goBackToMenu(1);      
    }
}

void menuClose(uint8_t param) {
    if(LCDML.FUNC_setup()) {
        LCDML.FUNC_goBackToMenu(1);      
        menuOpen = false;
    }
}

boolean checkBackflushEnabled() { return backflushOn == 1; }
boolean checkBackflushDisabled() { return backflushOn == 0; }

boolean checkStandbyEnabled() { return standbyModeOn == 1; }
boolean checkStandbyDisabled() { return standbyModeOn == 0; }

boolean checkBrewModeScale() { return BREWMODE == 2; }

boolean checkPonMEnabled() { return usePonM == 1; }
boolean checkPonMDisabled() { return usePonM == 0; }

boolean checkBrewPIDEnabled() { return useBDPID == 1; }
boolean checkBrewPIDDisabled() { return useBDPID == 0; }

// -------------------
// Temperatures
// -------------------
LCDML_add(0, LCDML_0, 1, LANGSTRING_MENU_TEMPERATURE, NULL); 
LCDML_add(1, LCDML_0_1, 1, LANGSTRING_MENU_BREWSETPOINT, changeBrewTemp); 
LCDML_add(2, LCDML_0_1, 2, LANGSTRING_MENU_STEAMSETPOINT, changeSteamTemp); 
LCDML_add(3, LCDML_0_1, 3, LANGSTRING_MENU_TEMP_OFFSET, changeTempOffset); 
LCDML_add(4, LCDML_0_1, 4, LANGSTRING_MENU_BACK, menuBack); 

// -------------------
// Times and Weights
// -------------------
LCDML_add(5, LCDML_0, 2, LANGSTRING_MENU_TIMES_AND_WEIGHTS, NULL); 
LCDML_add(6, LCDML_0_2, 1, LANGSTRING_MENU_BREWTIME, changeBrewTime); 
LCDML_add(7, LCDML_0_2, 2, LANGSTRING_MENU_PREINFUSIONTIME, changePreinfusionTime); 
LCDML_add(8, LCDML_0_2, 3, LANGSTRING_MENU_PREINFUSIONPAUSETIME, changePreinfusionPauseTime); 
LCDML_addAdvanced(9, LCDML_0_2, 4, checkBrewModeScale, LANGSTRING_MENU_WEIGHTSETPOINT, changeTargetWeight, 0, _LCDML_TYPE_default); 
LCDML_add(10, LCDML_0_2, 5, LANGSTRING_MENU_BACK, menuBack); 

// -------------------
// Machine settings
// -------------------
LCDML_add(11, LCDML_0, 3, LANGSTRING_MENU_MACHINESETTINGS, NULL); 
// BACKFLUSH
LCDML_add(12, LCDML_0_3, 1, LANGSTRING_MENU_BACKFLUSH, NULL); 
LCDML_addAdvanced(13, LCDML_0_3_1, 1, checkBackflushDisabled, LANGSTRING_MENU_ON, toggleBackflush, 0, _LCDML_TYPE_default); 
LCDML_addAdvanced(14, LCDML_0_3_1, 2, checkBackflushEnabled, LANGSTRING_MENU_OFF, toggleBackflush, 0, _LCDML_TYPE_default); 
LCDML_add(15, LCDML_0_3_1, 3, LANGSTRING_MENU_BACK, menuBack); 
// STANDBY TIMER
LCDML_add(16, LCDML_0_3, 2, LANGSTRING_MENU_STANDBY_TIMER, NULL); 
LCDML_addAdvanced(17, LCDML_0_3_2, 1, checkStandbyDisabled, LANGSTRING_MENU_ON, toggleStandbyTimer, 0, _LCDML_TYPE_default); 
LCDML_addAdvanced(18, LCDML_0_3_2, 2, checkStandbyEnabled, LANGSTRING_MENU_OFF, toggleStandbyTimer, 0, _LCDML_TYPE_default); 
LCDML_addAdvanced(19, LCDML_0_3_2, 3, checkStandbyEnabled, LANGSTRING_MENU_STANDBY_TIMER_TIME, changeStandbyTime, 0, _LCDML_TYPE_default); 
LCDML_add(20, LCDML_0_3_2, 4, LANGSTRING_MENU_BACK, menuBack); 
LCDML_add(21, LCDML_0_3, 3, LANGSTRING_MENU_BACK, menuBack); 

// -------------------
// PID Parameters
// -------------------
LCDML_add(22, LCDML_0, 4, LANGSTRING_MENU_PIDPARAMS, NULL); 
LCDML_add(23, LCDML_0_4, 1, LANGSTRING_MENU_PID_KP, changePidKp); 
LCDML_add(24, LCDML_0_4, 2, LANGSTRING_MENU_PID_TN, changePidTn); 
LCDML_add(25, LCDML_0_4, 3, LANGSTRING_MENU_PID_TV, changePidTv); 
LCDML_add(26, LCDML_0_4, 4, LANGSTRING_MENU_PID_I_MAX, changePidIMax); 
LCDML_add(27, LCDML_0_4, 5, LANGSTRING_MENU_STEAM_KP, changeSteamKp); 
// PONM
LCDML_add(28, LCDML_0_4, 6, LANGSTRING_MENU_START_USE_PONM, NULL); 
LCDML_addAdvanced(29, LCDML_0_4_6, 1, checkPonMDisabled, LANGSTRING_MENU_ON, toggleUsePonM, 0, _LCDML_TYPE_default); 
LCDML_addAdvanced(30, LCDML_0_4_6, 2, checkPonMEnabled, LANGSTRING_MENU_OFF, toggleUsePonM, 0, _LCDML_TYPE_default); 
LCDML_addAdvanced(31, LCDML_0_4_6, 3, checkPonMEnabled, LANGSTRING_MENU_START_KP, changePonMKp, 0, _LCDML_TYPE_default); 
LCDML_addAdvanced(32, LCDML_0_4_6, 4, checkPonMEnabled, LANGSTRING_MENU_START_TN, changePonMTn, 0, _LCDML_TYPE_default); 
LCDML_add(33, LCDML_0_4_6, 5, LANGSTRING_MENU_BACK, menuBack); 
// Brew PID
LCDML_add(34, LCDML_0_4, 7, LANGSTRING_MENU_PID_BD_ON, NULL); 
LCDML_addAdvanced(35, LCDML_0_4_7, 1, checkBrewPIDDisabled, LANGSTRING_MENU_ON, toggleUseBrewPid, 0, _LCDML_TYPE_default); 
LCDML_addAdvanced(36, LCDML_0_4_7, 2, checkBrewPIDEnabled, LANGSTRING_MENU_OFF, toggleUseBrewPid, 0, _LCDML_TYPE_default); 
LCDML_addAdvanced(37, LCDML_0_4_7, 3, checkBrewPIDEnabled, LANGSTRING_MENU_PID_BD_DELAY, changeBDDelay, 0, _LCDML_TYPE_default); 
LCDML_addAdvanced(38, LCDML_0_4_7, 4, checkBrewPIDEnabled, LANGSTRING_MENU_PID_BD_KP, changeBDKp, 0, _LCDML_TYPE_default); 
LCDML_addAdvanced(39, LCDML_0_4_7, 5, checkBrewPIDEnabled, LANGSTRING_MENU_PID_BD_TN, changeBDTn, 0, _LCDML_TYPE_default); 
LCDML_addAdvanced(40, LCDML_0_4_7, 6, checkBrewPIDEnabled, LANGSTRING_MENU_PID_BD_TV, changeBDTv, 0, _LCDML_TYPE_default); 
LCDML_addAdvanced(41, LCDML_0_4_7, 7, checkBrewPIDEnabled, LANGSTRING_MENU_PID_BD_TIME, changeBDTime, 0, _LCDML_TYPE_default); 
LCDML_addAdvanced(42, LCDML_0_4_7, 8, checkBrewPIDEnabled, LANGSTRING_MENU_PID_BD_SENSITIVITY, changeBDSensitivity, 0, _LCDML_TYPE_default); 
LCDML_add(43, LCDML_0_4_7, 9, LANGSTRING_MENU_BACK, menuBack); 
LCDML_add(44, LCDML_0_4, 8, LANGSTRING_MENU_BACK, menuBack); 

LCDML_add(45, LCDML_0, 5, LANGSTRING_MENU_CLOSE, menuClose); 

// This value has to be the same as the last menu element
#define _LCDML_DISP_cnt 45
LCDML_createMenu(_LCDML_DISP_cnt);

// Menu aborts via longpress have to be "debounced" heavily to feel natural
long lastMenuAbort = 0;
long menuAbortInterval = 3000;

// Translate encoder events to menu events
void menuControls(void) {
    int32_t pos = encoder.getCount() / ENCODER_CLICKS_PER_NOTCH;
    if (pos < last) {
        LCDML.BT_up();
        #if ROTARY_MENU_DEBUG == 1
            debugPrintf("Up\n");
        #endif 
    } 
    else if (pos > last) {
        LCDML.BT_down();
        #if ROTARY_MENU_DEBUG == 1
            debugPrintf("Down\n");
        #endif
    } 
    else {
        if (xQueueReceive(button_events, &ev, 1/portTICK_PERIOD_MS)) {
            if (ev.event == BUTTON_UP) {
                #if ROTARY_MENU_DEBUG == 1
                    debugPrintf("Processing Click");
                #endif
                LCDML.BT_enter();
            } else if (ev.event == BUTTON_HELD) {
                long abortTime = millis();
                if (abortTime - lastMenuAbort > menuAbortInterval) {
                    #if ROTARY_MENU_DEBUG == 1
                        debugPrintf("Processing button held, aborting.");
                    #endif
                    LCDML.FUNC_goBackToMenu(1);
                    lastMenuAbort = abortTime;
                }
            }
        }
    }

    last = pos;
}

void clearMenu() {
}

void setupMenu() {
    LCDML_setup(_LCDML_DISP_cnt);
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

    uint8_t n_max = (LCDML.MENU_getChilds() >= _LCDML_DISP_rows) ? _LCDML_DISP_rows : (LCDML.MENU_getChilds());

    uint8_t scrollbar_min = 0;
    uint8_t scrollbar_max = LCDML.MENU_getChilds();
    uint8_t scrollbar_cur_pos = LCDML.MENU_getCursorPosAbs();
    uint8_t scroll_pos = ((1.*n_max * _LCDML_DISP_rows) / (scrollbar_max - 1) * scrollbar_cur_pos);

    u8g2.setFont(u8g2_font_profont11_tf);
    u8g2.firstPage();

    do {
        n = 0;
        i = LCDML.MENU_getScroll();

        if ((tmp = LCDML.MENU_getDisplayedObj()) != NULL) {
            do {
                if (tmp->checkCondition()) {
                    if(tmp->checkType_menu() == true) {
                        LCDML_getContent(content_text, tmp->getID());
                        u8g2.drawStr( _LCDML_DISP_box_x0+_LCDML_DISP_font_w + _LCDML_DISP_cur_space_behind, _LCDML_DISP_box_y0 + _LCDML_DISP_font_h * (n + 1), content_text);
                    }
                    else {
                        if(tmp->checkType_dynParam()) {
                            tmp->callback(n);
                        }
                    }

                    i++;
                    n++;
                }
            } 
            while (((tmp = tmp->getSibling(1)) != NULL) && (i < maxi));
        }

        u8g2.drawStr( _LCDML_DISP_box_x0+_LCDML_DISP_cur_space_before, _LCDML_DISP_box_y0 + _LCDML_DISP_font_h * (LCDML.MENU_getCursorPos() + 1),  _LCDML_DISP_cursor_char);

        if(_LCDML_DISP_draw_frame == 1) {
            u8g2.drawFrame(_LCDML_DISP_box_x0, _LCDML_DISP_box_y0, (_LCDML_DISP_box_x1-_LCDML_DISP_box_x0), (_LCDML_DISP_box_y1-_LCDML_DISP_box_y0));
        }

        if (scrollbar_max > n_max && _LCDML_DISP_scrollbar_w > 2) {
            // Set frame for scrollbar
            u8g2.drawFrame(_LCDML_DISP_box_x1 - _LCDML_DISP_scrollbar_w, _LCDML_DISP_box_y0, _LCDML_DISP_scrollbar_w, _LCDML_DISP_box_y1-_LCDML_DISP_box_y0);

            // Calculate scrollbar length
            uint8_t scrollbar_block_length = scrollbar_max - n_max;
            scrollbar_block_length = (_LCDML_DISP_box_y1-_LCDML_DISP_box_y0) / (scrollbar_block_length + _LCDML_DISP_rows);

            if (scrollbar_cur_pos == 0) {                                   // top position     (min)
                u8g2.drawBox(_LCDML_DISP_box_x1 - (_LCDML_DISP_scrollbar_w-1), _LCDML_DISP_box_y0 + 1 , (_LCDML_DISP_scrollbar_w-2), scrollbar_block_length);
            }
            else if (scrollbar_cur_pos == (scrollbar_max-1)) {            // bottom position  (max)
                u8g2.drawBox(_LCDML_DISP_box_x1 - (_LCDML_DISP_scrollbar_w-1), _LCDML_DISP_box_y1 - scrollbar_block_length, (_LCDML_DISP_scrollbar_w-2), scrollbar_block_length);
            }
            else {                                                                // between top and bottom
                u8g2.drawBox(_LCDML_DISP_box_x1 - (_LCDML_DISP_scrollbar_w-1), _LCDML_DISP_box_y0 + (scrollbar_block_length * scrollbar_cur_pos + 1),(_LCDML_DISP_scrollbar_w-2), scrollbar_block_length);
            }
        }
    } 
    while (u8g2.nextPage());
}