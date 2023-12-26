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
#define PIN_POWERSWITCH 39
#define PIN_BREWSWITCH 34
#define PIN_STEAMSWITCH 35
#define PIN_WATERSWITCH 36

#define PIN_ROTARY_DT 4         // Rotary encoder data pin
#define PIN_ROTARY_CLK 3        // Rotary encoder clock pin
#define PIN_ROTARY_SW 5         // Rotary encoder switch

// Sensors
#define PIN_TEMPSENSOR 16
#define PIN_WATERSENSOR 23
#define PIN_HXDAT 32            // Brew scale data pin 1
#define PIN_HXDAT2 25           // Brew scale data pin 2
#define PIN_HXCLK 33            // Brew scale clock pin


/**
 * Output pins
 */

// Relays
#define PIN_VALVE 17
#define PIN_PUMP 27
#define PIN_HEATER 2

// LEDs
#define PIN_STATUSLED 26
#define PIN_BREWLED 19
#define PIN_STEAMLED 1

// Periphery
#define PIN_ZC 18               // Dimmer circuit Zero Crossing


/**
 * Bidirectional Pins
 */
#define PIN_I2CSCL 22
#define PIN_I2CSDA 21
