// #if ROTARY_MENU == 1
#ifndef MENU_H
#define MENU_H

#include "LCDMenuLib2.h"  // Include this if LCDMenuLib2.h is not included in the main file
#include "Storage.h"
#include "ClickEncoder.h"

extern LCDMenuLib2_menu LCDML_0;
extern LCDMenuLib2 LCDML;
extern ClickEncoder encoder;

extern double brewSetpoint;
extern double steamSetpoint;
extern double brewtime;
extern double preinfusion;
extern double preinfusionpause;
extern int backflushON;
extern bool menuOpen;

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

#if (ROTARY_MENU == 1)
#include "Storage.h"  
#include "LCDMenuLib2.h"  
#include "ClickEncoder.h"
#include "defaults.h"
// #include "display.h"

LCDMenuLib2_menu LCDML_0(255, 0, 0, NULL, NULL);
LCDMenuLib2 LCDML(LCDML_0, _LCDML_DISP_rows, _LCDML_DISP_cols, displayMenu, clearMenu, menuControls);

double menuRotaryLast = 0;
double initialValue = 0;
int last = 0;

void changeNumericalValue(uint8_t param, double value, sto_item_id_t name, const char* readableName, const char* unit) {
    if(LCDML.FUNC_setup()) {
        // remmove compiler warnings when the param variable is not used:
        LCDML_UNUSED(param);

        menuRotaryLast = encoder.getAccumulate();
        initialValue = value;
        char message[100];
        snprintf(message, sizeof(message), "Initialized old to %f.0.", value);
        debugPrintln(message);

        displayNumericalMenuSettingWithUnit(initialValue, readableName, unit);
    }

    if(LCDML.FUNC_loop()) {
        int32_t pos = encoder.getAccumulate();
        double diff = static_cast<double>(pos - menuRotaryLast) / 10.0;

        char message[100];
        snprintf(message, sizeof(message), "Pos: %d, last: %d, diff: %f.00", pos, last, diff);
        debugPrintln(message);

        if (diff < 0) {
            initialValue = initialValue + diff;
            char message[100];
            snprintf(message, sizeof(message), "DOWN. Old %s: %f.0, new: %f.0, diff: %f.0", readableName, initialValue - diff, initialValue, diff);
            debugPrintln(message);
        } 
        else if (diff > 0) {
            initialValue = initialValue + diff;
            char message[100];
            snprintf(message, sizeof(message), "UP. Old %s: %f.0, new: %f.0, diff: %f.0", readableName, initialValue - diff, initialValue, diff);
            debugPrintln(message);
        } 

        displayNumericalMenuSettingWithUnit(initialValue, readableName, unit);
        menuRotaryLast = pos;
            
        if(LCDML.BT_checkEnter()) { 
            char message[100];
            snprintf(message, sizeof(message), "Saving new %s: %f.0", readableName, initialValue);
            debugPrintln(message);
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
            } else {
                debugPrintln("error.");
            }
        }
    }

    if(LCDML.FUNC_close()) {
        // you can here reset some global vars or do nothing
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
        LCDML_UNUSED(param);
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

// this value has to be the same as the last menu element
#define _LCDML_DISP_cnt 15
LCDML_createMenu(_LCDML_DISP_cnt);

// Translate encoder events to menu events
void menuControls(void) {
    int32_t pos = encoder.getAccumulate();
    Button::eButtonStates buttonState = encoder.getButton();

    if (pos < last) {
        LCDML.BT_up();
    } 
    else if (pos > last) {
        LCDML.BT_down();
    } 
    else {
        if (buttonState == Button::Clicked) {
            LCDML.BT_enter();
        } else {
            // do nothing
        }
    }

    last = pos;
}


void clearMenu() {
}


void setupMenu() {
    LCDML_setup(_LCDML_DISP_cnt);
}
#endif


#endif  // MENU_H
// #endif