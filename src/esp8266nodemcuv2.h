/*
 * ESP8266 Pin Mapping on "esp8266 pcb rev 1.2"
 */

// Pin Layout
#define PINTEMPSENSOR 2            
#define PINPRESSURESENSOR 0        // Pressure sensor 0: A0 (ESP8266), >0 ONLY ESP32
#define PINPOWERSWITCH 99          // Input pin for powerswitch (use a pin which is LOW on startup or use an pull down resistor)
#define PINVALVE 12
#define PINPUMP 13
#define PINHEATER 14
#define PINETRIGGER 16             // PIN for E-Trigger relay
#define PINBREWSWITCH 15           // For switch, trigger or optocoupler 
#define PINSTEAMSWITCH 99
#define LEDPIN 99                  // LED PIN ON near setpoint
#define OLED_SCL 5                 // Output pin for display clock pin
#define OLED_SDA 4                 // Output pin for display data pin
#define HXDATPIN 99                // weight scale data pin
#define HXCLKPIN 99                // weight scale clock pin

// Check BrewSwitch
#if (defined(ESP8266) && ((PINBREWSWITCH != 15 && PINBREWSWITCH != 0 && PINBREWSWITCH != 16 )))
  #error("WRONG Brewswitch PIN for ESP8266, Only PIN 0, PIN 15 and PIN 16");
#endif

// defined compiler errors
#if (PRESSURESENSOR == 1) && (PINPRESSURESENSOR == 0) && (PINBREWSWITCH == 0)
  #error Change PINBREWSWITCH or PRESSURESENSOR!
#endif