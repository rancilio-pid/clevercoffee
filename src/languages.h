/**
 * @file languages.h
 *
 * @brief Localized strings
 */

#pragma once

#include "userConfig.h"

#if (LANGUAGE == 0) // DE
#if (DISPLAYTEMPLATE <= 4)
    static const char *langstring_set_temp = "Soll:  ";
    static const char *langstring_current_temp = "Ist:   ";
    static const char *langstring_brew = "Brew: ";
    static const char *langstring_uptime = "Uptime:  ";
#endif
#if (DISPLAYTEMPLATE >= 20)  //vertical templates
    static const char *langstring_set_temp_rot_ur =      "S: ";
    static const char *langstring_current_temp_rot_ur =  "I: ";
    static const char *langstring_brew_rot_ur =          "B: ";
#endif

static const char *langstring_offlinemode = "Offline";
#if TOF == 1
static const char *langstring_waterempty = "Wasser leer";
#endif

static const char *langstring_wifirecon = "Wifi reconnect:";
static const char *langstring_connectwifi1 = "1: Verbinde WLAN:";
static const char *langstring_nowifi[] = {"Kein ", "WLAN"};

static const char *langstring_error_tsensor[] = {"Fehler, Temp: ", "Temp.-Sensor ueberpruefen!"};
// static const char *langstring_emergencyStop[] = {"HEATING", "STOPPED"};

static const char *langstring_backflush_press = "Bruehsch. druecken";
static const char *langstring_backflush_start = "um zu Starten...";
static const char *langstring_backflush_finish = "zum Beenden...";

// ATTENTION: It is important that none of the displayed strings are longer than 
// there is columns in the display, otherwise the ESP will crash!
#define LANGSTRING_MENU_TEMPERATURE "Temperaturen"
#define LANGSTRING_MENU_BREWSETPOINT "Bruehtemperatur"
#define LANGSTRING_MENU_STEAMSETPOINT "Dampftemperatur"
#define LANGSTRING_MENU_BACK "Zurueck"
#define LANGSTRING_MENU_CLOSE "Menue schliessen"
#define LANGSTRING_MENU_BREWTIME "Bezugsdauer"
#define LANGSTRING_MENU_PREINFUSIONTIME "Praeinfusionsdauer"
#define LANGSTRING_MENU_PREINFUSIONPAUSETIME "Praeinfusionspause"
#define LANGSTRING_MENU_MACHINESETTINGS "Maschine"
#define LANGSTRING_MENU_BACKFLUSH "Rueckspuelmodus"
#define LANGSTRING_MENU_TEMP_OFFSET "Temperaturoffset"
#define LANGSTRING_MENU_TIMES_AND_WEIGHTS "Zeiten und Gewicht"
#define LANGSTRING_MENU_WEIGHTSETPOINT "Zielgewicht"
#define LANGSTRING_MENU_STANDBY_TIMER "Standby Timer"
#define LANGSTRING_MENU_STANDBY_TIMER_TIME "Standby Time"
#define LANGSTRING_MENU_PIDPARAMS "PID-Parameter"
#define LANGSTRING_MENU_PID_KP "PID Kp"
#define LANGSTRING_MENU_PID_TN "PID Tn"
#define LANGSTRING_MENU_PID_TV "PID Tv"
#define LANGSTRING_MENU_PID_I_MAX "PID Integrator Max"
#define LANGSTRING_MENU_STEAM_KP "Steam Kp"
#define LANGSTRING_MENU_START_USE_PONM "PonM Start"
#define LANGSTRING_MENU_START_KP "PonM Start Kp"
#define LANGSTRING_MENU_START_TN  "PonM Start Tn"
#define LANGSTRING_MENU_PID_BD_ON "PID Brueherkennung"
#define LANGSTRING_MENU_PID_BD_DELAY "PID BD Delay"
#define LANGSTRING_MENU_PID_BD_KP "PID BD Kp"
#define LANGSTRING_MENU_PID_BD_TN "PID BD Tn"
#define LANGSTRING_MENU_PID_BD_TV "PID BD Tv"
#define LANGSTRING_MENU_PID_BD_TIME "PID BD Dauer"
#define LANGSTRING_MENU_PID_BD_SENSITIVITY "PID BD Sensitiv."
#define LANGSTRING_MENU_ON "An"
#define LANGSTRING_MENU_OFF "Aus"
static const char *langstring_pressToSave[] = {"Klicken zum Speichern,", "Halten zum Abbrechen."};
static const char *langstring_autoclose[] = {"Menue schliesst sich in", "2 Sekunden automatisch."};


#elif LANGUAGE == 1 // EN
#if (DISPLAYTEMPLATE <= 4)
    static const char *langstring_set_temp = "Set:   ";
    static const char *langstring_current_temp = "Temp:  ";
    static const char *langstring_brew = "Brew: ";
    static const char *langstring_uptime = "Uptime:  ";
#endif
#if (DISPLAYTEMPLATE >= 20)  //vertical templates
    static const char *langstring_set_temp_rot_ur = "S: ";
    static const char *langstring_current_temp_rot_ur = "T: ";
    static const char *langstring_brew_rot_ur = "B: ";
#endif

static const char *langstring_offlinemode = "Offline";
#if TOF == 1
static const char *langstring_waterempty = "Empty water";
#endif

static const char *langstring_wifirecon = "Wifi reconnect:";
static const char *langstring_connectwifi1 = "1: Connecting Wifi:";
static const char *langstring_nowifi[] = {"No ", "WIFI"};

static const char *langstring_error_tsensor[] = {"Error, Temp: ", "Check Temp. sensor!"};
// static const char *langstring_emergencyStop[] = {"HEATING", "STOPPED"};

static const char *langstring_backflush_press = "Press brew switch";
static const char *langstring_backflush_start = "to start...";
static const char *langstring_backflush_finish = "to finish...";

// ATTENTION: It is important that none of the displayed strings are longer than 
// there is columns in the display, otherwise the ESP will crash!
#define LANGSTRING_MENU_TEMPERATURE "Temperatures"
#define LANGSTRING_MENU_BREWSETPOINT "Brew setpoint"
#define LANGSTRING_MENU_STEAMSETPOINT "Steam setpoint"
#define LANGSTRING_MENU_BACK "Back"
#define LANGSTRING_MENU_CLOSE "Close menu"
#define LANGSTRING_MENU_BREWTIME "Brewtime"
#define LANGSTRING_MENU_PREINFUSIONTIME "Preinfusion time"
#define LANGSTRING_MENU_PREINFUSIONPAUSETIME "Preinfusion pause"
#define LANGSTRING_MENU_MACHINESETTINGS "Machine Settings"
#define LANGSTRING_MENU_BACKFLUSH "Backflush mode"
#define LANGSTRING_MENU_TEMP_OFFSET "Temperature offset"
#define LANGSTRING_MENU_TIMES_AND_WEIGHTS "Times and weight"
#define LANGSTRING_MENU_WEIGHTSETPOINT "Target weight"
#define LANGSTRING_MENU_STANDBY_TIMER "Standby timer"
#define LANGSTRING_MENU_STANDBY_TIMER_TIME "Standby time"
#define LANGSTRING_MENU_PIDPARAMS "PID parameters"
#define LANGSTRING_MENU_PID_KP "PID Kp"
#define LANGSTRING_MENU_PID_TN "PID Tn"
#define LANGSTRING_MENU_PID_TV "PID Tv"
#define LANGSTRING_MENU_PID_I_MAX "PID integrator max"
#define LANGSTRING_MENU_STEAM_KP "Steam Kp"
#define LANGSTRING_MENU_START_USE_PONM "PonM Start"
#define LANGSTRING_MENU_START_KP "PonM Start Kp"
#define LANGSTRING_MENU_START_TN  "PonM Start Tn"
#define LANGSTRING_MENU_PID_BD_ON "PID brew detection"
#define LANGSTRING_MENU_PID_BD_DELAY "PID BD Delay"
#define LANGSTRING_MENU_PID_BD_KP "PID BD Kp"
#define LANGSTRING_MENU_PID_BD_TN "PID BD Tn"
#define LANGSTRING_MENU_PID_BD_TV "PID BD Tv"
#define LANGSTRING_MENU_PID_BD_TIME "PID BD duration"
#define LANGSTRING_MENU_PID_BD_SENSITIVITY "PID BD sensitiv."
#define LANGSTRING_MENU_ON "On"
#define LANGSTRING_MENU_OFF "Off"
static const char *langstring_pressToSave[] = {"Click to save or", "hold to abort."};
static const char *langstring_autoclose[] = {"Menu will autoclose", "in two seconds."};

#elif LANGUAGE == 2 // ES
#if (DISPLAYTEMPLATE <= 4)
    static const char *langstring_set_temp = "Obj:  ";
    static const char *langstring_current_temp = "T:    ";
    static const char *langstring_brew = "Brew: ";
    static const char *langstring_uptime = "Uptime:  ";
#endif
#if (DISPLAYTEMPLATE >= 20)  //vertical templates
    static const char *langstring_set_temp_rot_ur = "O: ";
    static const char *langstring_current_temp_rot_ur = "T: ";
    static const char *langstring_brew_rot_ur = "B: ";
#endif

static const char *langstring_offlinemode = "Offline";
#if TOF == 1
static const char *langstring_waterempty = "Agua vacía";
#endif

static const char *langstring_wifirecon = "Reconecta wifi:";
static const char *langstring_connectwifi1 = "1: Wifi conectado :";
static const char *langstring_nowifi[] = {"No ", "WIFI"};

static const char *langstring_error_tsensor[] = {"Error, Temp: ", "Comprueba sensor T!"};
// static const char *langstring_emergencyStop[] = {"CALENT.", "PARADO "};

static const char *langstring_backflush_press = "Pulsa boton de cafe";
static const char *langstring_backflush_start = "para empezar...";
static const char *langstring_backflush_finish = "para terminar...";

// ATTENTION: It is important that none of the displayed strings are longer than 
// there is columns in the display, otherwise the ESP will crash!
#define LANGSTRING_MENU_TEMPERATURE "Temperaturas"
#define LANGSTRING_MENU_BREWSETPOINT "Temperatura de infusión"
#define LANGSTRING_MENU_STEAMSETPOINT "Temperatura de vapor"
#define LANGSTRING_MENU_BACK "Back"
#define LANGSTRING_MENU_CLOSE "Close menu"
#define LANGSTRING_MENU_BREWTIME "Brewtime"
#define LANGSTRING_MENU_PREINFUSIONTIME "Preinfusion time"
#define LANGSTRING_MENU_PREINFUSIONPAUSETIME "Preinfusion pause"
#define LANGSTRING_MENU_MACHINESETTINGS "Machine Settings"
#define LANGSTRING_MENU_BACKFLUSH "Backflush mode"
#define LANGSTRING_MENU_TEMP_OFFSET "Temperature offset"
#define LANGSTRING_MENU_TIMES_AND_WEIGHTS "Times and weight"
#define LANGSTRING_MENU_WEIGHTSETPOINT "Target weight"
#define LANGSTRING_MENU_STANDBY_TIMER "Standby timer"
#define LANGSTRING_MENU_STANDBY_TIMER_TIME "Standby time"
#define LANGSTRING_MENU_PIDPARAMS "PID parameters"
#define LANGSTRING_MENU_PID_KP "PID Kp"
#define LANGSTRING_MENU_PID_TN "PID Tn"
#define LANGSTRING_MENU_PID_TV "PID Tv"
#define LANGSTRING_MENU_PID_I_MAX "PID integrator max"
#define LANGSTRING_MENU_STEAM_KP "Steam Kp"
#define LANGSTRING_MENU_START_USE_PONM "PonM Start"
#define LANGSTRING_MENU_START_KP "PonM Start Kp"
#define LANGSTRING_MENU_START_TN  "PonM Start Tn"
#define LANGSTRING_MENU_PID_BD_ON "PID brew detection"
#define LANGSTRING_MENU_PID_BD_DELAY "PID BD Delay"
#define LANGSTRING_MENU_PID_BD_KP "PID BD Kp"
#define LANGSTRING_MENU_PID_BD_TN "PID BD Tn"
#define LANGSTRING_MENU_PID_BD_TV "PID BD Tv"
#define LANGSTRING_MENU_PID_BD_TIME "PID BD duration"
#define LANGSTRING_MENU_PID_BD_SENSITIVITY "PID BD sensitiv."
#define LANGSTRING_MENU_ON "On"
#define LANGSTRING_MENU_OFF "Off"
static const char *langstring_pressToSave[] = {"Click to save or", "hold to abort."};
static const char *langstring_autoclose[] = {"Menu will autoclose", "in two seconds."};

#endif
