/**
 * @file menu.h
 * 
 * @brief Display menu for rotary encoder 
 */
#pragma once

#include "LCDMenuLib2.h"
#include "Storage.h"
#include <ESP32Encoder.h>
#include "button.h"
#include "defaults.h"
#include "languages.h"
#include "debugSerial.h"

extern LCDMenuLib2_menu LCDML_0;
extern LCDMenuLib2 LCDML;
extern ESP32Encoder encoder;
extern button_event_t ev;
extern QueueHandle_t button_events;

extern double brewSetpoint;
extern double steamSetpoint;
extern double brewtime;
extern double preinfusion;
extern double preinfusionpause;
extern int backflushON;
extern bool menuOpen;
extern bool clicked;

extern void displayNumericalMenuSettingWithUnit(double temp, const char* name, const char* unit);
extern void displayToggleBackflushMessage(uint8_t mode);

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

void changeNumericalValue(uint8_t param, double value, sto_item_id_t name, const char* readableName, const char* unit) {
    if(LCDML.FUNC_setup()) {
        menuRotaryLast = encoder.getCount() / 4;
        initialValue = value;

        displayNumericalMenuSettingWithUnit(initialValue, readableName, unit);
    }

    if(LCDML.FUNC_loop()) {
        int32_t pos = encoder.getCount() / 4;
        double diff = static_cast<double>(pos - menuRotaryLast) / 10.0;

        if (diff != 0) {
            initialValue = initialValue + diff;
            displayNumericalMenuSettingWithUnit(initialValue, readableName, unit);
            menuRotaryLast = pos;
        }
            
        if(LCDML.BT_checkEnter()) { 
            storageSet(name, initialValue);

            int saved = storageCommit();

            if (saved == 0) {
                switch (name) {
                    case STO_ITEM_BREW_SETPOINT:
                        brewSetpoint = initialValue;
                        break;
                    case STO_ITEM_STEAM_SETPOINT:
                        steamSetpoint = initialValue;
                        break;
                    case STO_ITEM_BREW_TIME:
                        brewtime = initialValue;
                        break;
                    case STO_ITEM_PRE_INFUSION_TIME:
                        preinfusion = initialValue;
                        break;
                    case STO_ITEM_PRE_INFUSION_PAUSE:
                        preinfusionpause = initialValue;
                        break;
                    
                    default:
                        // do nothing
                        break;
                }

                debugPrintln("SAVED.");
                LCDML.FUNC_goBackToMenu();
            } 
            else {
                debugPrintln("error.");
            }
        }
    }
}

void changeBrewTemp(uint8_t param) {
    changeNumericalValue(param, brewSetpoint, STO_ITEM_BREW_SETPOINT, LANGSTRING_MENU_BREWSETPOINT, "C");
}

void changeSteamTemp(uint8_t param) {
    changeNumericalValue(param, steamSetpoint, STO_ITEM_STEAM_SETPOINT, LANGSTRING_MENU_BREWSETPOINT, "C");
}

void changeBrewTime(uint8_t param) {
    changeNumericalValue(param, brewtime, STO_ITEM_BREW_TIME, LANGSTRING_MENU_BREWTIME, "s");
}

void changePreinfusionTime(uint8_t param) {
    changeNumericalValue(param, preinfusion, STO_ITEM_PRE_INFUSION_TIME, LANGSTRING_MENU_PREINFUSIONTIME, "s");
}

void changePreinfusionPauseTime(uint8_t param) {
    changeNumericalValue(param, preinfusionpause, STO_ITEM_PRE_INFUSION_PAUSE, LANGSTRING_MENU_PREINFUSIONPAUSETIME, "s");
}

void switchBackflush(uint8_t param) {
    if(LCDML.FUNC_setup()) {
        backflushON = param;
        displayToggleBackflushMessage(param);
        delay(2000);
        LCDML.FUNC_goBackToMenu(1);      
    }
}

void backflushOn(uint8_t param) {
    switchBackflush(1);
}

void backflushOff(uint8_t param) {
    switchBackflush(0);
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

LCDML_add(0, LCDML_0, 1, LANGSTRING_MENU_TEMPERATURE, NULL); 
LCDML_add(1, LCDML_0_1, 1, LANGSTRING_MENU_BREWSETPOINT, changeBrewTemp); 
LCDML_add(2, LCDML_0_1, 2, LANGSTRING_MENU_STEAMSETPOINT, changeSteamTemp); 
LCDML_add(3, LCDML_0_1, 3, LANGSTRING_MENU_BACK, menuBack); 

LCDML_add(4, LCDML_0, 2, LANGSTRING_MENU_TIMES, NULL); 
LCDML_add(5, LCDML_0_2, 1, LANGSTRING_MENU_BREWTIME, changeBrewTime); // NULL = no menu function
LCDML_add(6, LCDML_0_2, 2, LANGSTRING_MENU_PREINFUSIONTIME, changePreinfusionTime); // NULL = no menu function
LCDML_add(7, LCDML_0_2, 3, LANGSTRING_MENU_PREINFUSIONPAUSETIME, changePreinfusionPauseTime); 
LCDML_add(8, LCDML_0_2, 4, LANGSTRING_MENU_BACK, menuBack); 

LCDML_add(9, LCDML_0, 3, LANGSTRING_MENU_MACHINESETTINGS, NULL); 
LCDML_add(10, LCDML_0_3, 1, LANGSTRING_MENU_BACKFLUSH, NULL); 
LCDML_add(11, LCDML_0_3_1, 1, LANGSTRING_MENU_ON, backflushOn); 
LCDML_add(12, LCDML_0_3_1, 2, LANGSTRING_MENU_OFF, backflushOff); 
LCDML_add(13, LCDML_0_3_1, 3, LANGSTRING_MENU_BACK, menuBack); 
LCDML_add(14, LCDML_0_3, 2, LANGSTRING_MENU_BACK, menuBack); 

LCDML_add(15, LCDML_0, 4, LANGSTRING_MENU_CLOSE, menuClose); 

// This value has to be the same as the last menu element
#define _LCDML_DISP_cnt 15
LCDML_createMenu(_LCDML_DISP_cnt);

// Translate encoder events to menu events
void menuControls(void) {
    int32_t pos = encoder.getCount() / 4;
    if (pos < last) {
        LCDML.BT_up();
        debugPrintf("Up\n");
    } 
    else if (pos > last) {
        LCDML.BT_down();
        debugPrintf("Down\n");
    } 
    else {
        if (xQueueReceive(button_events, &ev, 1000/portTICK_PERIOD_MS)) {
            if (ev.event == BUTTON_DOWN) {
                debugPrintf("Processing Click");
                LCDML.BT_enter();
            } else {
                // do nothing
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
