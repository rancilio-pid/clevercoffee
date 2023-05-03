/*
 * ESP32 Pin Mapping on "clevercoffee_ESP32_minimal" pcb rev 1.2 or higher and "clevercoffee_ESP32" pcb rev 1.2 or higher
 */

/**
 * Input Pins
 */

// Switches/Buttons
#define PINPOWERSWITCH 39
#define PINBREWSWITCH 34
#define PINSTEAMSWITCH 35


// Sensors
#define PINTEMPSENSOR 16
#define PINPRESSURESENSOR 23
#define PINVOLTAGESENSOR 34    // Input pin for voltage sensor (optocoupler to detect brew switch)
#define HXDATPIN 32            // Brew scale data pin
#define HXCLKPIN 33            // Brew scale clock pin


/**
 * Output pins
 */

// Relays
#define PINVALVE 17
#define PINPUMP 27
#define PINHEATER 2

// LEDs
#define LEDPIN 26

// Periphery
#define PINETRIGGER 25          // PIN for E-Trigger relay

/**
 * Bidirectional Pins
 */
#define OLED_SCL 22
#define OLED_SDA 21
