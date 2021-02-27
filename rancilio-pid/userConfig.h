/********************************************************
  Version 2.1 (14.02.2021) 
  Last Change: code cleanup
  Values must be configured by the user
******************************************************/

#ifndef _userConfig_H
#define _userConfig_H  

/********************************************************
   Preconfiguration
******************************************************/
// Display
#define DISPLAY 2                  // 0 = deactivated, 1 = SH1106 (e.g. 1.3 "128x64), 2 = SSD1306 (e.g. 0.96" 128x64)
#define DISPLAYTEMPLATE 3          // 1 = Standard Display Template, 2 = Minimal Template, 3 = only Temperatur, 20 = vertical Display see git Handbook for further information
#define MACHINELOGO 1              // 1 = Rancilio, 2 = Gaggia
#define DISPALYROTATE U8G2_R0      // rotate display clockwise: U8G2_R0 = no rotation; U8G2_R1 = 90°; U8G2_R2 = 180°; U8G2_R3 = 270°
#define SHOTTIMER 1                // 0 = deactivated, 1 = activated
#define HEATINGLOGO 0              // 0 = deactivated, 1 = Rancilio, 2 = Gaggia 
#define OFFLINEGLOGO 1             // 0 = deactivated, 1 = activated
#define BREWSWITCHDELAY 3000       // time in ms

// Offline mode
#define OFFLINEMODUS 0             // 0 = Blynk and Wifi are used, 1 = offline mode (only preconfigured values in code are used!)
#define FALLBACK 1                 // 1 = fallback to values stored in eeprom, 0 = deactivated
#define GRAFANA 1                  // 1 = grafana visualisation (access required), 0 = off (default)

// PID & Hardware
#define ONLYPID 1                  // 1 = Only PID, 0 = PID and preinfusion
#define BREWDETECTION 1            // 0 = off, 1 = Software (Onlypid 0), 2 = Hardware (Onlypid 0), 3 = Sensor/Hardware for Only PID 
#define COLDSTART_PID 1            // 1 = default coldstart values, 2 = custom values via blynk (expert mode activated) 
#define TRIGGERTYPE HIGH           // LOW = low trigger, HIGH = high trigger relay
// BREWDETECTION 3 configuration
#define VOLTAGESENSORTYPE HIGH 
#define PINMODEVOLTAGESENSOR INPUT // Mode INPUT_PULLUP, INPUT or INPUT_PULLDOWN_16 (Only Pin 16)

// E-Trigger
#define ETRIGGER 0                 // 0 = no trigger (for Rancilio except Rancilio E), 1 = trigger for CPU of Rancilio E
#define ETRIGGERTIME 60            // seconds, time between trigger signal
#define TRIGGERRELAYTYPE HIGH      // LOW = low trigger, HIGH = high trigger relay for E-Trigger

// Wifi 
#define HOSTNAME "Rancilio"
#define D_SSID "myssid"
#define PASS "mypass"
#define MAXWIFIRECONNECTS 5        // maximum number of reconnection attempts, use -1 to deactivate
#define WIFICINNECTIONDELAY 10000  // delay between reconnects in ms

// OTA
#define OTA true                   // true = OTA activated, false = OTA deactivated
#define OTAHOST "Rancilio"         // Name to be shown in ARUDINO IDE Port
#define OTAPASS "otapass"          // Password for OTA updtates

// MQTT
#define MQTT 0                     // 1 = MQTT enabled, 0 = MQTT disabled
#define MQTT_USERNAME "myuser"
#define MQTT_PASSWORD "mypass"
#define MQTT_TOPIC_PREFIX "custom/Küche."  // topic will be "<MQTT_TOPIC_PREFIX><HOSTNAME>/<READING>"
#define MQTT_SERVER_IP "XXX.XXX.XXX.XXX"  // IP-Address of locally installed mqtt server
#define MQTT_SERVER_PORT 1883    

// BLynk
#define AUTH "myauth"
#define BLYNKADDRESS "blynk.clevercoffee.de"  // blynk-server IP-Address
#define BLYNKPORT 8080             // blynk-server port

// PID - offline values
#define SETPOINT 95                // Temperatur setpoint
#define AGGKP 69                   // Kp normal
#define AGGTN 399                  // Tn
#define AGGTV 0                    // Tv

// PID coldstart
#define STARTKP 50                 // Start Kp during coldstart
#define STARTTN 150                // Start Tn during cold start

// PID - offline brewdetection values
#define AGGBKP 50                  // Kp
#define AGGBTN 0                   // Tn 
#define AGGBTV 20                  // Tv

// Backflush values
#define FILLTIME 3000              // time in ms the pump is running
#define FLUSHTIME 6000             // time in ms the 3-way valve is open -> backflush
#define MAXFLUSHCYCLES 5           // number of cycles the backflush should run, 0 = disabled

// Pin Layout
#define ONE_WIRE_BUS 2             // Temp sensor pin
#define pinRelayVentil 12          // Output pin for 3-way-valve
#define pinRelayPumpe 13           // Output pin for pump
#define pinRelayHeater 14          // Output pin for heater
#define PINVOLTAGESENSOR  15    //Input pin for volatage sensor
//#define OLED_RESET 16              // Output pin for dispaly reset pin
#define PINETRIGGER 16             // PIN for E-Trigger relay
#define OLED_SCL 5                 // Output pin for dispaly clock pin
#define OLED_SDA 4                 // Output pin for dispaly data pin
#define SCREEN_WIDTH 128           // OLED display width, in pixels
#define SCREEN_HEIGHT 64           // OLED display height, in pixels  

// Historic (no settings)
#define PONE 1                     // 1 = P_ON_E (default), 0 = P_ON_M (special PID mode, other PID-parameter are needed)
#define TEMPSENSOR 2               // 2 = TSIC306 1=DS18B20

#endif // _userConfig_H
