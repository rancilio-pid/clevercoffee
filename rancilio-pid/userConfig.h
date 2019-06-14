/********************************************************
  Version 1.0 (13.06.2019)
  Config must be configured by the user
******************************************************/

#ifndef _userConfig_H
#define _userConfig_H  

/********************************************************
   Vorab-Konfig
******************************************************/
#define OFFLINEMODUS 0       // 0=Blynk und WLAN wird benötigt 1=OfflineModus (ACHTUNG EINSTELLUNGEN NUR DIREKT IM CODE MÖGLICH)
#define DISPLAY 2            // 1=U8x8libm, 0=Deaktiviert, 2=Externes 128x64 Display
#define ONLYPID 0            // 1=Nur PID ohne Preinfussion, 0=PID + Preinfussion
#define TEMPSENSOR 2         // 1=DS19B20; 2=TSIC306
#define BREWDETECTION 1      // 0 = off ,1 = Software, 2 = Hardware
#define FALLBACK 1           // 1: fallback auf eeprom Werte, wenn blynk nicht geht 0: deaktiviert
#define TRIGGERTYPE HIGH     // LOW = low trigger, HIGH = high trigger relay
#define OTA true             // true=activate update via OTA
#define PONE 1               // 1 = P_ON_E (normal), 0 = P_ON_M (spezieller PID Modus, ACHTUNG andere Formel zur Berechnung)

// Wifi
#define AUTH "blynkauthcode"
#define D_SSID "wlanname"
#define PASS "wlanpass"

// OTA
#define OTAHOST "Rancilio"   // Name to be shown in ARUDINO IDE Port
#define OTAPASS "otapass"    // Password for OTA updtates

#define BLYNKADDRESS "blynk.remoteapp.de"         // IP-Address of used blynk server
// define BLYNKADDRESS "raspberrypi.local"


//PID - Werte für Brüherkennung Offline
#define AGGBKP 40    // Kp
#define AGGBTN 200   // Tn Must never be zero! Otherwise div/0 for aggbKi
#define AGGBTV 10    // Tv

//PID - Werte für Regelung Offline
#define SETPOINT 95  // Temperatur Sollwert
#define AGGKP 40     // Kp
#define AGGTN 200    // Must never be zero! Otherwise div/0 for aggKi
#define AGGTV 0      // Tv
#define STARTKP 40   // Start Kp während Kaltstart
#define STARTTN 100  // Start Tn während Kaltstart
#define STARTTEMP 85 // Temperaturschwelle für deaktivieren des Start Kp


#endif // _userConfig_H
