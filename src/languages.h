/**
 * @file languages.h
 *
 * @brief Localized strings
 */

#pragma once

#include "userConfig.h"

#if (LANGUAGE == 0) // DE
#if (DISPLAYTEMPLATE == 1) || (DISPLAYTEMPLATE == 2)
    static const char *langstring_set_temp = "Soll:  ";
    static const char *langstring_current_temp = "Ist:   ";
    static const char *langstring_brew = "Brew:  ";
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

static const char *langstring_bckffinished[] = {"Backflush beendet", "Bitte Bruehschalter abschalten..."};
static const char *langstring_bckfactivated[] = {"Backflush aktiviert", "Bruehschalter betaetigen ..."};
static const char *langstring_bckfrunning[] = {"Backflush aktiv:", "seit"};

#define LANGSTRING_MENU_TEMPERATURE "Temperaturen"
#define LANGSTRING_MENU_TEMPERATURE "Temperaturen"
#define LANGSTRING_MENU_BREWSETPOINT "Bruehtemperatur"
#define LANGSTRING_MENU_STEAMSETPOINT "Dampftemperatur"
#define LANGSTRING_MENU_BACK "Zurueck"
#define LANGSTRING_MENU_CLOSE "Menue schliessen"
#define LANGSTRING_MENU_TIMES "Zeiten"
#define LANGSTRING_MENU_BREWTIME "Bezugsdauer"
#define LANGSTRING_MENU_PREINFUSIONTIME "Praeinfusionsdauer"
#define LANGSTRING_MENU_PREINFUSIONPAUSETIME "Praeinfusion Pause"
#define LANGSTRING_MENU_MACHINESETTINGS "Maschine"
#define LANGSTRING_MENU_BACKFLUSH "Backflush-Modus"
#define LANGSTRING_MENU_ON "An"
#define LANGSTRING_MENU_OFF "Aus"
static const char *langstring_pressToSave[] = {"Button klicken zum ", "Speichern."};
static const char *langstring_autoclose[] = {"Menue schliesst sich in", "2 Sekunden automatisch."};


#elif LANGUAGE == 1 // EN
#if (DISPLAYTEMPLATE == 1) || (DISPLAYTEMPLATE == 2)
    static const char *langstring_set_temp = "Set:   ";
    static const char *langstring_current_temp = "Temp:  ";
    static const char *langstring_brew = "Brew:  ";
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

static const char *langstring_bckffinished[] = {"Backflush finished", "Please reset brew switch..."};
static const char *langstring_bckfactivated[] = {"Backflush activated", "Please set brew switch..."};
static const char *langstring_bckfrunning[] = {"Backflush running:", "from"};

LANGSTRING_MENU_TEMPERATURE "Temperatures"
LANGSTRING_MENU_BREWSETPOINT "Brew setpoint"
LANGSTRING_MENU_STEAMSETPOINT "Steam setpoint"
LANGSTRING_MENU_BACK "Back"
LANGSTRING_MENU_CLOSE "Close menu"
LANGSTRING_MENU_TIMES "Timings"
LANGSTRING_MENU_BREWTIME "Brewtime"
LANGSTRING_MENU_PREINFUSIONTIME "Preinfusion time"
LANGSTRING_MENU_PREINFUSIONPAUSETIME "Preinfusion pause"
LANGSTRING_MENU_MACHINESETTINGS "Machine Settings"
LANGSTRING_MENU_BACKFLUSH "Backflush mode"
LANGSTRING_MENU_ON "On"
LANGSTRING_MENU_OFF "Off"
static const char *langstring_pressToSave[] = {"Press encoder to save", "and return"};
static const char *langstring_autoclose[] = {"Menu will autoclose", "in two seconds."};

#elif LANGUAGE == 2 // ES
#if (DISPLAYTEMPLATE == 1) || (DISPLAYTEMPLATE == 2)
    static const char *langstring_set_temp = "Obj:  ";
    static const char *langstring_current_temp = "T:    ";
    static const char *langstring_brew = "Brew:  ";
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

static const char *langstring_bckffinished[] = {"Backflush terminado", "Apague el boton de cafe..."};
static const char *langstring_bckfactivated[] = {"Backflush activado ", "Encienda boton de cafe.."};
static const char *langstring_bckfrunning[] = {"Backflush activo: ", "desde"};

LANGSTRING_MENU_TEMPERATURE "Temperaturas"
LANGSTRING_MENU_BREWSETPOINT "Temperatura de infusión"
LANGSTRING_MENU_STEAMSETPOINT "Temperatura de vapor"
LANGSTRING_MENU_BACK "Back"
LANGSTRING_MENU_CLOSE "Close menu"
LANGSTRING_MENU_TIMES "Timings"
LANGSTRING_MENU_BREWTIME "Brewtime"
LANGSTRING_MENU_PREINFUSIONTIME "Preinfusion time"
LANGSTRING_MENU_PREINFUSIONPAUSETIME "Preinfusion pause"
LANGSTRING_MENU_MACHINESETTINGS "Machine Settings"
LANGSTRING_MENU_BACKFLUSH "Backflush mode"
LANGSTRING_MENU_BACKFLUSH "Backflush mode"
LANGSTRING_MENU_ON "On"
LANGSTRING_MENU_OFF "Off"
static const char *langstring_pressToSave[] = {"Press encoder to save", "and return"};
static const char *langstring_autoclose[] = {"Menu will autoclose", "in two seconds."};

#endif
