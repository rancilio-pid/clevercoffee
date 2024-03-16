/**
 * @file languages.h
 *
 * @brief Localized strings
 */

#pragma once

#include "userConfig.h"

#if (LANGUAGE == 0) // DE
#if (DISPLAYTEMPLATE <= 4)
static const char* langstring_set_temp = "Soll:  ";
static const char* langstring_current_temp = "Ist:   ";
static const char* langstring_brew = "Brew: ";
static const char* langstring_uptime = "Uptime:  ";
#endif
#if (DISPLAYTEMPLATE >= 20) // vertical templates
static const char* langstring_set_temp_rot_ur = "S: ";
static const char* langstring_current_temp_rot_ur = "I: ";
static const char* langstring_brew_rot_ur = "B: ";
#endif

static const char* langstring_offlinemode = "Offline";
#if TOF == 1
static const char* langstring_waterempty = "Wasser leer";
#endif

static const char* langstring_wifirecon = "Wifi reconnect:";
static const char* langstring_connectwifi1 = "1: Verbinde WLAN:";
static const char* langstring_nowifi[] = {"Kein ", "WLAN"};

static const char* langstring_error_tsensor[] = {"Fehler, Temp: ", "Temp.-Sensor ueberpruefen!"};
// static const char *langstring_emergencyStop[] = {"HEATING", "STOPPED"};

static const char* langstring_backflush_press = "Bruehsch. druecken";
static const char* langstring_backflush_start = "um zu starten...";
static const char* langstring_backflush_finish = "um zu beenden...";

#elif LANGUAGE == 1         // EN
#if (DISPLAYTEMPLATE <= 4)
static const char* langstring_set_temp = "Set:   ";
static const char* langstring_current_temp = "Temp:  ";
static const char* langstring_brew = "Brew: ";
static const char* langstring_uptime = "Uptime:  ";
#endif
#if (DISPLAYTEMPLATE >= 20) // vertical templates
static const char* langstring_set_temp_rot_ur = "S: ";
static const char* langstring_current_temp_rot_ur = "T: ";
static const char* langstring_brew_rot_ur = "B: ";
#endif

static const char* langstring_offlinemode = "Offline";
#if TOF == 1
static const char* langstring_waterempty = "Empty water";
#endif

static const char* langstring_wifirecon = "Wifi reconnect:";
static const char* langstring_connectwifi1 = "1: Connecting Wifi:";
static const char* langstring_nowifi[] = {"No ", "WIFI"};

static const char* langstring_error_tsensor[] = {"Error, Temp: ", "Check Temp. sensor!"};
// static const char *langstring_emergencyStop[] = {"HEATING", "STOPPED"};

static const char* langstring_backflush_press = "Press brew switch";
static const char* langstring_backflush_start = "to start...";
static const char* langstring_backflush_finish = "to finish...";

#elif LANGUAGE == 2         // ES
#if (DISPLAYTEMPLATE <= 4)
static const char* langstring_set_temp = "Obj:  ";
static const char* langstring_current_temp = "T:    ";
static const char* langstring_brew = "Brew: ";
static const char* langstring_uptime = "Uptime:  ";
#endif
#if (DISPLAYTEMPLATE >= 20) // vertical templates
static const char* langstring_set_temp_rot_ur = "O: ";
static const char* langstring_current_temp_rot_ur = "T: ";
static const char* langstring_brew_rot_ur = "B: ";
#endif

static const char* langstring_offlinemode = "Offline";
#if TOF == 1
static const char* langstring_waterempty = "Agua vacía";
#endif

static const char* langstring_wifirecon = "Reconecta wifi:";
static const char* langstring_connectwifi1 = "1: Wifi conectado :";
static const char* langstring_nowifi[] = {"No ", "WIFI"};

static const char* langstring_error_tsensor[] = {"Error, Temp: ", "Comprueba sensor T!"};
// static const char *langstring_emergencyStop[] = {"CALENT.", "PARADO "};

static const char* langstring_backflush_press = "Pulsa boton de cafe";
static const char* langstring_backflush_start = "para empezar...";
static const char* langstring_backflush_finish = "para terminar...";
#endif
