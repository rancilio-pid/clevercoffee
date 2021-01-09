/********************************************************
  Version 1.5 (30.12.2020) Last-Change "PIN BELEGUNG" in userConfig.h verschoben
  Config must be configured by the user
******************************************************/

#ifndef _userConfig_H
#define _userConfig_H  

/********************************************************
   Vorab-Konfig
******************************************************/
// 
#define DISPLAY 2            // 0 = deactivated, 1 = SH1106 (e.g. 1.3 "128x64), 2 = SSD1306 (e.g. 0.96" 128x64)
#define DISPLAYTEMPLATE 1    // 1: Standard Display Template, 2: Minimal Template, see git Handbook for further
#define OFFLINEMODUS 0       // 0 = Blynk and WIFI are used; 1 = offline mode (only preconfigured values in code are used!)
#define ONLYPID 1            // 1 = Only PID, no preinfusion; 0 = PID and preinfusion
#define TEMPSENSOR 2         // 2 = TSIC306
#define BREWDETECTION 1      // 0 = off; 1 = Software; 2 = Hardware
#define FALLBACK 1           // 1 = fallback to values stored in eeprom, if blynk is not working; 0 = deactivated
#define TRIGGERTYPE HIGH     // LOW = low trigger, HIGH = high trigger relay
#define OTA true             // true = activate update via OTA
#define PONE 1               // 1 = P_ON_E (default), 0 = P_ON_M (special PID mode, other PID-parameter are needed)
#define GRAFANA 1            // 1 = grafana visualisation. Access required, 0 = off (default)
#define WIFICINNECTIONDELAY 10000 // delay between reconnects in ms
#define MAXWIFIRECONNECTS 5  // maximum number of reconnects; use -1 to set to maximum ("deactivated")
#define MACHINELOGO 1        // 1 = Rancilio, 2 = Gaggia
#define MQTT 0               // 1 = MQTT enabled, 0 = MQTT disabled
#define COLDSTART_PID 1      // 1 = default COLDStart Values , 2 = eigene Werte via Blynk, Expertenmodusaktiv 
#define DISPALYROTATE U8G2_R0   // rotate display clockwise: U8G2_R0 = no rotation; U8G2_R1 = 90°; U8G2_R2 = 180°; U8G2_R3 = 270°

//MQTT
#define MQTT_USERNAME "myuser"
#define MQTT_PASSWORD "mypass"
#define MQTT_TOPIC_PREFIX "custom/Küche."  // topic will be "<MQTT_TOPIC_PREFIX><HOSTNAME>/<READING>"
#define MQTT_SERVER_IP "XXX.XXX.XXX.XXX"       // IP-Address of locally installed mqtt server
#define MQTT_SERVER_PORT 1883    
// Wifi & Blynk 
#define HOSTNAME "rancilio"
#define AUTH "blynkauthcode"
#define D_SSID "wlanname"
#define PASS "wlanpass"

// OTA
#define OTAHOST "Rancilio"   // Name to be shown in ARUDINO IDE Port
#define OTAPASS "otapass"    // Password for OTA updates

#define BLYNKADDRESS "blynk.remoteapp.de"         // IP-Address of used blynk server
#define BLYNKPORT 8080  //Port for blynk server
// define BLYNKADDRESS "raspberrypi.local"


//PID - values for offline brewdetection
#define AGGBKP 50    // Kp
#define AGGBTN 0   // Tn 
#define AGGBTV 20    // Tv

//PID - offline values
#define SETPOINT 95  // Temperatur setpoint
#define AGGKP 69     // Kp
#define AGGTN 399    // Tn
#define AGGTV 0      // Tv
#define STARTKP 50   // Start Kp during coldstart
#define STARTTN 150  // Start Tn during cold start

//backflush values
#define FILLTIME 3000       // time in ms the pump is running
#define FLUSHTIME 6000      // time in ms the 3-way valve is open -> backflush
#define MAXFLUSHCYCLES 5      // number of cycles the backflush should run; 0 = disabled

//PIN BELEGUNG
#define ONE_WIRE_BUS 2  // TEMP SENSOR PIN

#define pinRelayVentil    12    //Output pin for 3-way-valve
#define pinRelayPumpe     13    //Output pin for pump
#define pinRelayHeater    14    //Output pin for heater

#define OLED_RESET 16     //Output pin for dispaly reset pin
#define OLED_SCL 5        //Output pin for dispaly clock pin
#define OLED_SDA 4        //Output pin for dispaly data pin
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels  

#endif // _userConfig_H
