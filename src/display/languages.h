/**
 * @file languages.h
 *
 * @brief Localized strings
 */

#pragma once

#include "Config.h"

static const char* langstring_set_temp;
static const char* langstring_current_temp;
static const char* langstring_brew;
static const char* langstring_weight;
static const char* langstring_manual_flush;
static const char* langstring_pressure;
static const char* langstring_uptime;
static const char* langstring_offlinemode;
static const char* langstring_wifirecon;
static const char* langstring_connectwifi1;
static const char* langstring_nowifi[32];
static const char* langstring_error_tsensor[64];
static const char* langstring_scale_Failure;
static const char* langstring_backflush_press;
static const char* langstring_backflush_start;
static const char* langstring_backflush_finish;

inline void initLangStrings(Config& config) {

    // English
    if (config.get<int>("display.language") == 1) {
        langstring_set_temp = "Set:   ";
        langstring_current_temp = "Temp:  ";
        langstring_brew = "Brew: ";
        langstring_weight = "Weight: ";
        langstring_manual_flush = "Flush: ";
        langstring_pressure = "Pressure: ";
        langstring_uptime = "Uptime:  ";

        langstring_offlinemode = "Offline";
        langstring_wifirecon = "Wifi reconnect:";
        langstring_connectwifi1 = "1: Connecting to WiFi:";
        langstring_nowifi[0] = "No ";
        langstring_nowifi[1] = "WiFi";

        langstring_error_tsensor[0] = "Error, Temp: ";
        langstring_error_tsensor[1] = "Check Temp. sensor!";

        langstring_scale_Failure = "Fault";

        langstring_backflush_press = "Press brew switch";
        langstring_backflush_start = "to start...";
        langstring_backflush_finish = "to finish...";
    }
    // Espanol
    else if (config.get<int>("display.language") == 2) {
        langstring_set_temp = "Obj:  ";
        langstring_current_temp = "T:    ";
        langstring_brew = "Brew: ";
        langstring_weight = "Peso: ";
        langstring_manual_flush = "Fregar: ";
        langstring_pressure = "Presión: ";
        langstring_uptime = "Uptime:  ";

        langstring_offlinemode = "Offline";
        langstring_wifirecon = "Reconecta wifi:";
        langstring_connectwifi1 = "1: Wifi conectado :";
        langstring_nowifi[0] = "No ";
        langstring_nowifi[1] = "WIFI";

        langstring_error_tsensor[0] = "Error, Temp: ";
        langstring_error_tsensor[1] = "Comprueba sensor T!";
        langstring_scale_Failure = "falla";

        langstring_backflush_press = "Pulsa boton de cafe";
        langstring_backflush_start = "para empezar...";
        langstring_backflush_finish = "para terminar...";
    }
    // German (default)
    else {
        langstring_set_temp = "Soll:  ";
        langstring_current_temp = "Ist:   ";
        langstring_brew = "Bezug: ";
        langstring_weight = "Gewicht: ";
        langstring_manual_flush = "Spuelen: ";
        langstring_pressure = "Druck: ";
        langstring_uptime = "Uptime:  ";

        langstring_offlinemode = "Offline";
        langstring_wifirecon = "Wifi reconnect:";
        langstring_connectwifi1 = "1: Verbinde WLAN:";
        langstring_nowifi[0] = "Kein ";
        langstring_nowifi[1] = "WLAN";
        langstring_error_tsensor[0] = "Fehler, Temp: ";
        langstring_error_tsensor[1] = "Temp.-Sensor ueberpruefen!";
        langstring_scale_Failure = "Fehler";

        langstring_backflush_press = "Bruehsch. druecken";
        langstring_backflush_start = "um zu starten...";
        langstring_backflush_finish = "um zu beenden...";
    }
}
