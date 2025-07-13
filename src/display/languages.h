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
static const char* langstring_hot_water;
static const char* langstring_pressure;
static const char* langstring_uptime;
static const char* langstring_offlinemode;
static const char* langstring_wifirecon;
static const char* langstring_connectwifi1;
static const char* langstring_nowifi[2];
static const char* langstring_error_tsensor[5];
static const char* langstring_scale_Failure;
static const char* langstring_backflush_press;
static const char* langstring_backflush_start;
static const char* langstring_backflush_finish;
static const char* langstring_set_temp_ur;
static const char* langstring_current_temp_ur;
static const char* langstring_brew_ur;
static const char* langstring_manual_flush_ur;
static const char* langstring_hot_water_ur;
static const char* langstring_weight_ur;
static const char* langstring_pressure_ur;
static const char* langstring_error_tsensor_ur[5];
static const char* langstring_calibrate_start;
static const char* langstring_calibrate_in_progress;
static const char* langstring_calibrate_complete;

inline void initLangStrings(const Config& config) {

    // English
    if (config.get<int>("display.language") == 1) {
        langstring_set_temp = "Set:   ";
        langstring_current_temp = "Temp:  ";
        langstring_brew = "Brew: ";
        langstring_weight = "Weight: ";
        langstring_manual_flush = "Flush: ";
        langstring_hot_water = "Water: ";
        langstring_pressure = "Pressure: ";
        langstring_uptime = "Uptime:  ";

        langstring_set_temp_ur = "S: ";
        langstring_current_temp_ur = "T: ";
        langstring_brew_ur = "B: ";
        langstring_manual_flush_ur = "F: ";
        langstring_hot_water_ur = "Wp: ";
        langstring_weight_ur = "W: ";
        langstring_pressure_ur = "P: ";

        langstring_offlinemode = "Offline";
        langstring_wifirecon = "Wifi reconnect:";
        langstring_connectwifi1 = "1: Connecting to WiFi:";
        langstring_nowifi[0] = "No ";
        langstring_nowifi[1] = "WiFi";

        langstring_error_tsensor[0] = "Error, Temp: ";
        langstring_error_tsensor[1] = "Check Temp. sensor!";
        langstring_scale_Failure = "Fault";
        langstring_error_tsensor_ur[0] = "Error";
        langstring_error_tsensor_ur[1] = "Temp: ";
        langstring_error_tsensor_ur[2] = "check";
        langstring_error_tsensor_ur[3] = "temp.";
        langstring_error_tsensor_ur[4] = "sensor!";

        langstring_backflush_press = "Press brew switch";
        langstring_backflush_start = "to start...";
        langstring_backflush_finish = "to finish...";

        langstring_calibrate_start = "Calibration coming up\n\nEmpty scale ";
        langstring_calibrate_in_progress = "Calibration in progress. Place known weight on scale in next 10 seconds ";
        langstring_calibrate_complete = "Calibration done!\nNew result: ";
    }
    // Espanol
    else if (config.get<int>("display.language") == 2) {
        langstring_set_temp = "Obj:  ";
        langstring_current_temp = "T:    ";
        langstring_brew = "Brew: ";
        langstring_weight = "Peso: ";
        langstring_manual_flush = "Fregar: ";
        langstring_hot_water = "Agua: ";
        langstring_pressure = "Presión: ";
        langstring_uptime = "Uptime:  ";

        langstring_set_temp_ur = "O: ";
        langstring_current_temp_ur = "T: ";
        langstring_brew_ur = "B: ";
        langstring_manual_flush_ur = "F: ";
        langstring_hot_water_ur = "A: ";
        langstring_weight_ur = "P: ";
        langstring_pressure_ur = "Pr: ";

        langstring_offlinemode = "Offline";
        langstring_wifirecon = "Reconecta wifi:";
        langstring_connectwifi1 = "1: Wifi conectado :";
        langstring_nowifi[0] = "No ";
        langstring_nowifi[1] = "WIFI";

        langstring_error_tsensor[0] = "Error, Temp: ";
        langstring_error_tsensor[1] = "Comprueba sensor T!";
        langstring_scale_Failure = "falla";
        langstring_error_tsensor_ur[0] = "Error";
        langstring_error_tsensor_ur[1] = "Temp: ";
        langstring_error_tsensor_ur[2] = "Comprueba";
        langstring_error_tsensor_ur[3] = "sensor";
        langstring_error_tsensor_ur[4] = "T!";

        langstring_backflush_press = "Pulsa boton de cafe";
        langstring_backflush_start = "para empezar...";
        langstring_backflush_finish = "para terminar...";

        langstring_calibrate_start = "Calibración iniciando\n\nVaciar la balanza ";
        langstring_calibrate_in_progress = "Calibrando. Coloque un peso conocido en la balanza en los próximos 10 segundos ";
        langstring_calibrate_complete = "Calibración completa!\nNuevo resultado: ";
    }
    // German (default)
    else {
        langstring_set_temp = "Soll:  ";
        langstring_current_temp = "Ist:   ";
        langstring_brew = "Bezug: ";
        langstring_weight = "Gewicht: ";
        langstring_manual_flush = "Spuelen: ";
        langstring_hot_water = "Wasser: ";
        langstring_pressure = "Druck: ";
        langstring_uptime = "Uptime:  ";

        langstring_set_temp_ur = "S: ";
        langstring_current_temp_ur = "I: ";
        langstring_brew_ur = "B: ";
        langstring_manual_flush_ur = "S: ";
        langstring_hot_water_ur = "W: ";
        langstring_weight_ur = "G: ";
        langstring_pressure_ur = "D: ";

        langstring_offlinemode = "Offline";
        langstring_wifirecon = "Wifi reconnect:";
        langstring_connectwifi1 = "1: Verbinde WLAN:";
        langstring_nowifi[0] = "Kein ";
        langstring_nowifi[1] = "WLAN";
        langstring_error_tsensor[0] = "Fehler, Temp: ";
        langstring_error_tsensor[1] = "Temp.-Sensor ueberpruefen!";
        langstring_scale_Failure = "Fehler";
        langstring_error_tsensor_ur[0] = "Fehler";
        langstring_error_tsensor_ur[1] = "Temp: ";
        langstring_error_tsensor_ur[2] = "Temp.";
        langstring_error_tsensor_ur[3] = "Sensor";
        langstring_error_tsensor_ur[4] = "ueberpruefen!";

        langstring_backflush_press = "Bruehsch. druecken";
        langstring_backflush_start = "um zu starten...";
        langstring_backflush_finish = "um zu beenden...";

        langstring_calibrate_start = "Kalibrierung startet\n\nWaage leeren ";
        langstring_calibrate_in_progress = "Kalibrierung läuft. Bitte in den nächsten 10 Sekunden ein bekanntes Gewicht auflegen ";
        langstring_calibrate_complete = "Kalibrierung abgeschlossen!\nNeues Ergebnis: ";
    }
}
