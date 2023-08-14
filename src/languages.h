/**
 * @file languages.h
 *
 * @brief Localized strings
 */

#pragma once

#include "userConfig.h"

#if (LANGUAGE == 0) // DE
#if (DISPLAYTEMPLATE <= 3)
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

#elif LANGUAGE == 1 // EN
#if (DISPLAYTEMPLATE <= 3)
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

#elif LANGUAGE == 2 // ES
#if (DISPLAYTEMPLATE <= 3)
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
#endif
