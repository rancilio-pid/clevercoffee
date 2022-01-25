/*
 * Common pin mappings
 */

#if defined(ESP32)
    #include "esp32devkit1v4.h"
#elif defined(ESP8266)
    #include "esp8266nodemcuv2.h"
#else
    #error("MCU not supported");
#endif

// OneWire
#define ONE_WIRE_BUS 2

// I/O Pins
#define PINBREWSWITCH 0            // 0: A0 (ESP8266) ; >0 : DIGITAL PIN, ESP32 OR ESP8266: ONLY USE PIN15 AND PIN16!
#define PINPRESSURESENSOR 99       // Pressuresensor 0: A0 (ESP8266), >0 ONLY ESP32
#define pinRelayVentil 12          // Output pin for 3-way-valve
#define pinRelayPumpe 13           // Output pin for pump
#define pinRelayHeater 14          // Output pin for heater
#define PINVOLTAGESENSOR  15       // Input pin for volatage sensor
#define PINETRIGGER 16             // PIN for E-Trigger relay
#define STEAMONPIN 17              // STEAM active
#define LEDPIN    18               // LED PIN ON near setpoint 

#define HXDATPIN 99                // Weight scale data pin
#define HXCLKPIN 99                // Weight scale clock pin