/**
 * @file pinmapping.h
 *
 * @brief Default GPIO pin mapping
 *
 */

#pragma once

/**
 * Input Pins
 */

// Switches/Buttons
#define PIN_POWERSWITCH 99
#define PIN_BREWSWITCH 16
#define PIN_STEAMSWITCH 17

#define PIN_ROTARY_DT 3         // Rotary encoder data pin
#define PIN_ROTARY_CLK 4        // Rotary encoder clock pin
#define PIN_ROTARY_SW 5        // Rotary encoder switch

// Sensors
#define PIN_TEMPSENSOR 16
#define PIN_PRESSURESENSOR 36
#define PIN_WATERSENSOR 23
#define PIN_FLOWSENSOR 1
#define PIN_HXDAT 35            // Brew scale data pin
#define PIN_HXCLK 34            // Brew scale clock pin


/**
 * Output pins
 */

// Relays
#define PIN_VALVE 12
#define PIN_PUMP 14
#define PIN_HEATER 25

// LEDs
#define PIN_STATUSLED 99

// Periphery
#define PIN_ZC 99               // Dimmer circuit Zero Crossing
#define PIN_PSM 99              // Dimmer circuit PSM


/**
 * Bidirectional Pins
 */
#define PIN_I2CSCL 18
#define PIN_I2CSDA 19
