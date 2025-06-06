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
static const char* langstring_brew = "Bezug: ";
static const char* langstring_weight = "Gewicht: ";
static const char* langstring_manual_flush = "Spuelen: ";
static const char* langstring_pressure = "Druck: ";
static const char* langstring_uptime = "Uptime:  ";
#endif
#if (DISPLAYTEMPLATE >= 20) // vertical templates
static const char* langstring_set_temp_rot_ur = "S: ";
static const char* langstring_current_temp_rot_ur = "I: ";
static const char* langstring_brew_rot_ur = "B: ";
static const char* langstring_manual_flush_rot_ur = "S: ";
static const char* langstring_weight_rot_ur = "G: ";
static const char* langstring_pressure_rot_ur = "D: ";
static const char* langstring_error_tsensor_rot_ur[] = {"Fehler", "Temp: ", "Temp.", "Sensor", "ueberpruefen!"};
#endif

static const char* langstring_offlinemode = "Offline";
static const char* langstring_wifirecon = "Wifi reconnect:";
static const char* langstring_connectwifi1 = "1: Verbinde WLAN:";
static const char* langstring_nowifi[] = {"Kein ", "WLAN"};
static const char* langstring_error_tsensor[] = {"Fehler, Temp: ", "Temp.-Sensor ueberpruefen!"};
static const char* langstring_scale_Failure = "Fehler";
// static const char *langstring_emergencyStop[] = {"HEATING", "STOPPED"};

static const char* langstring_backflush_press = "Bruehsch. druecken";
static const char* langstring_backflush_start = "um zu starten...";
static const char* langstring_backflush_finish = "um zu beenden...";

#elif LANGUAGE == 1         // EN
#if (DISPLAYTEMPLATE <= 4)
static const char* langstring_set_temp = "Set:   ";
static const char* langstring_current_temp = "Temp:  ";
static const char* langstring_brew = "Brew: ";
static const char* langstring_weight = "Weight: ";
static const char* langstring_manual_flush = "Flush: ";
static const char* langstring_pressure = "Pressure: ";
static const char* langstring_uptime = "Uptime:  ";
#endif
#if (DISPLAYTEMPLATE >= 20) // vertical templates
static const char* langstring_set_temp_rot_ur = "S: ";
static const char* langstring_current_temp_rot_ur = "T: ";
static const char* langstring_brew_rot_ur = "B: ";
static const char* langstring_manual_flush_rot_ur = "F: ";
static const char* langstring_weight_rot_ur = "W: ";
static const char* langstring_pressure_rot_ur = "P: ";
static const char* langstring_error_tsensor_rot_ur[] = {"Error", "Temp: ", "check", "temp.", "sensor!"};
#endif

static const char* langstring_offlinemode = "Offline";
static const char* langstring_wifirecon = "Wifi reconnect:";
static const char* langstring_connectwifi1 = "1: Connecting Wifi:";
static const char* langstring_nowifi[] = {"No ", "WIFI"};

static const char* langstring_error_tsensor[] = {"Error, Temp: ", "Check Temp. sensor!"};
static const char* langstring_scale_Failure = "Fault";
// static const char *langstring_emergencyStop[] = {"HEATING", "STOPPED"};

static const char* langstring_backflush_press = "Press brew switch";
static const char* langstring_backflush_start = "to start...";
static const char* langstring_backflush_finish = "to finish...";

#elif LANGUAGE == 2         // ES
#if (DISPLAYTEMPLATE <= 4)
static const char* langstring_set_temp = "Obj:  ";
static const char* langstring_current_temp = "T:    ";
static const char* langstring_brew = "Brew: ";
static const char* langstring_weight = "Peso: ";
static const char* langstring_manual_flush = "Fregar: ";
static const char* langstring_pressure = "Presión: ";
static const char* langstring_uptime = "Uptime:  ";
#endif
#if (DISPLAYTEMPLATE >= 20) // vertical templates
static const char* langstring_set_temp_rot_ur = "O: ";
static const char* langstring_current_temp_rot_ur = "T: ";
static const char* langstring_brew_rot_ur = "B: ";
static const char* langstring_manual_flush_rot_ur = "F: ";
static const char* langstring_weight_rot_ur = "P: ";
static const char* langstring_pressure_rot_ur = "Pr: ";
static const char* langstring_error_tsensor_rot_ur[] = {"Error", "Temp: ", "Comprueba", "sensor", "T!"};
#endif

static const char* langstring_offlinemode = "Offline";
static const char* langstring_wifirecon = "Reconecta wifi:";
static const char* langstring_connectwifi1 = "1: Wifi conectado :";
static const char* langstring_nowifi[] = {"No ", "WIFI"};

static const char* langstring_error_tsensor[] = {"Error, Temp: ", "Comprueba sensor T!"};
static const char* langstring_scale_Failure = "falla";
// static const char *langstring_emergencyStop[] = {"CALENT.", "PARADO "};

static const char* langstring_backflush_press = "Pulsa boton de cafe";
static const char* langstring_backflush_start = "para empezar...";
static const char* langstring_backflush_finish = "para terminar...";
#endif
