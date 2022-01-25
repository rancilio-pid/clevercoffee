/********************************************************
  Version 3.0.1 Alpha

  Values must be configured by the user
******************************************************/
#define SYSVERSION '3.0.1 ALPHA'

/********************************************************
   Define area, do not change anything here
******************************************************/

#ifndef _userConfig_H
#define _userConfig_H

// List of supported machines
enum MACHINE {
  RancilioSilvia,   // MACHINEID 0
  RancilioSilviaE,  // MACHINEID 1
  Gaggia,           // MACHINEID 2
  QuickMill         // MACHINEID 3
};

/********************************************************
   Preconfiguration
******************************************************/

// Machine
#define MACHINEID 0                //	see above list of supported machines

// Display
#define DISPLAY 2                  // 0 = deactivated, 1 = SH1106 (e.g. 1.3 "128x64), 2 = SSD1306 (e.g. 0.96" 128x64)
#define OLED_I2C 0x3C		           // I2C address for OLED, 0x3C by default
#define DISPLAYTEMPLATE 3          // 1 = Standard Display Template, 2 = Minimal Template, 3 = only Temperatur, 4 = Scale Template, 20 = vertical Display see git Handbook for further information
#define DISPLAYROTATE U8G2_R0      // rotate display clockwise: U8G2_R0 = no rotation; U8G2_R1 = 90°; U8G2_R2 = 180°; U8G2_R3 = 270°
#define SHOTTIMER 1                // 0 = deactivated, 1 = activated 2 = with scale
#define HEATINGLOGO 0              // 0 = deactivated, 1 = Rancilio, 2 = Gaggia
#define OFFLINEGLOGO 1             // 0 = deactivated, 1 = activated
#define LANGUAGE 1                 // LANGUAGE = 0 (DE), LANGUAGE = 1 (EN), LANGUAGE = 2 (ES)
#define SCREEN_WIDTH 128           // OLED display width, in pixels
#define SCREEN_HEIGHT 64           // OLED display height, in pixels

// Connectivity 
#define CONNECTMODE 1              // 0 = offline 1 = WIFI-MODE 2 = AP-MODE (not working in the moment)
#define BLYNK 1                   // 0 = no Blynk , 1 = Blynk
#define MQTT 1                     // 1 = MQTT enabled, 0 = MQTT disabled
#define GRAFANA 2                  // 2= custom Grafana 1 = grafana visualisation (access required), 0 = off (default)
#define INFLUXDB 1                 // 1 = INFLUX enabled, 0 = INFLUX disabled

// PID & Hardware
#define ONLYPID 1                  // 1 = Only PID, 0 = PID and preinfusion
#define ONLYPIDSCALE 0             // 0 = off , 1= OnlyPID with Scale
#define BREWMODE 1                 // 1 = NORMAL preinfusion ; 2 = Scale with weight
#define BREWDETECTION 1            // 0 = off, 1 = Software (Onlypid 1), 2 = Hardware (Onlypid 0), 3 = Sensor/Hardware for Only PID
#define BREWSWITCHTYPE 1           //  1 = normal Switch, 2 = Trigger Switch
#define COLDSTART_PID 1            // 1 = default coldstart values, 2 = custom values via blynk (expert mode activated)
#define TRIGGERTYPE HIGH           // LOW = low trigger, HIGH = high trigger relay
#define VOLTAGESENSORTYPE HIGH     // BREWDETECTION 3 configuration
#define PINMODEVOLTAGESENSOR INPUT // Mode INPUT_PULLUP, INPUT or INPUT_PULLDOWN_16 (Only Pin 16)
#define PRESSURESENSOR 0           // 1 = pressure sensor connected to A0; PINBREWSWITCH must be set to the connected input!
#define USELED         1         

// TOF sensor for water level
#define TOF 0                      // 0 = no TOF sensor connected; 1 = water level by TOF sensor
#define TOF_I2C 0x29               // I2C address of TOF sensor; 0x29 by default
#define CALIBRATION_MODE 0         // 1 = enable to obtain water level calibration values; 0 = disable for normal PID operation; can also be done in Blynk

// E-Trigger
#define ETRIGGER 0                 // 0 = no trigger (for Rancilio except Rancilio E), 1 = trigger for CPU of Rancilio E
#define TRIGGERRELAYTYPE HIGH      // LOW = low trigger, HIGH = high trigger relay for E-Trigger

// Accesspoint
#define APWIFISSID "Rancilio"
#define APWIFIKEY "Rancilio"

/// Wifi
#define HOSTNAME "wifi-hostname"
#define D_SSID "myssid"
#define PASS "mypass"

// OTA
#define OTA true                   // true = OTA activated, false = OTA deactivated
#define OTAHOST "ota_hostname"         // Name to be shown in ARUDINO IDE Port
#define OTAPASS "otapass"          // Password for OTA updtates

// MQTT
#define MQTT_USERNAME "mymqttuser"
#define MQTT_PASSWORD "mymqttpass"
#define MQTT_TOPIC_PREFIX "custom/Küche."  // topic will be "<MQTT_TOPIC_PREFIX><HOSTNAME>/<READING>"
#define MQTT_SERVER_IP "XXX.XXX.XXX.XXX"  // IP-Address of locally installed mqtt server
#define MQTT_SERVER_PORT 1883

// BLYNK
#define AUTH "blynk_auth"
#define BLYNKADDRESS "blynk.clevercoffee.de"    // blynk-server IP-Address
#define BLYNKPORT 8080                          // blynk-server portver

// INFLUX
#define INFLUXDB_URL "http://influxdb.clevercoffee.de:8086"
#define INFLUXDB_DB_NAME "clevercoffee"
#define INTERVALINFLUX 5000


// Historic (no settings)
#define PONE 1                     // 1 = P_ON_E (default), 0 = P_ON_M (special PID mode, other PID-parameter are needed)
#define TEMPSENSOR 2               // 2 = TSIC306 1=DS18B20

// Check BrewSwitch
#if (defined(ESP8266) && ((PINBREWSWITCH != 15 && PINBREWSWITCH != 0 && PINBREWSWITCH != 16 )))
  #error("WRONG Brewswitch PIN for ESP8266, Only PIN 15 and PIN 16");
#endif


// defined compiler errors
#if (PRESSURESENSOR == 1) && (PINPRESSURESENSOR == 0) && (PINBREWSWITCH == 0)
#error Change PINBREWSWITCH or PRESSURESENSOR!
#endif

#endif // _userConfig_H
