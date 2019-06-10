/********************************************************
   Version 1.8.2 BETA (10.06.2019)
  - Check the PIN Ports in the CODE!
  - Find your brewdetection changerate of the machine, can be wrong, test it!
  - WE CHANGE THE PID WORKING MODE & PID PARAMETER FROM Ki to Tn and Kp zu Tv!
  - VALUES BEFORE 1.8.0. have to be recalculated 
******************************************************/
#include "Arduino.h"
#include <EEPROM.h>
/********************************************************
   Vorab-Konfig
******************************************************/
int Offlinemodus = 0;       // 0=Blynk und WLAN wird benötigt 1=OfflineModus (ACHTUNG EINSTELLUNGEN NUR DIREKT IM CODE MÖGLICH)
int debugmodus = 0;         // 0=Keine Seriellen Debug Werte 1=SeriellenDebug aktiv
int Display = 2;            // 1=U8x8libm, 0=Deaktiviert, 2=Externes 128x64 Display
int OnlyPID = 1;            // 1=Nur PID ohne Preinfussion, 0=PID + Preinfussion
int TempSensor = 2;         // 1=DS19B20; 2=TSIC306
int Brewdetection = 1 ;     // 0=off ,1=Software
int standby = 0 ;           // 0: Old rancilio not needed, 1: new one , E or V5 with standy, not used in the moment
int fallback = 1  ;          // 1: fallback auf eeprom Werte, wenn blynk nicht geht 0: deaktiviert
int triggerType = HIGH;// LOW = low trigger, HIGH = high trigger relay
boolean OTA = true;                // true=activate update via OTA

char auth[] = "blynkauthcode";
char ssid[] = "wlanname";
char pass[] = "wlanpass";

char blynkaddress[]  = "blynk.remoteapp.de" ;
// char blynkaddress[]  = "raspberrypi.local" ;


/********************************************************
   Vorab-Konfig
******************************************************/

#include "Arduino.h"
#include <EEPROM.h>
/*  Variablen werden alle nicht verwendet
unsigned long previousMillisColdstart = 0;
unsigned long previousMillisColdstartPause = 0;
unsigned long ColdstartPause = 0;
unsigned long KaltstartPause = 0;
unsigned long bruehvorganggestartet = 0;
unsigned long warmstart = 0;
unsigned long previousMillisSwing = 0;
*/

int Onoff = 1 ;  // default 1
int relayON, relayOFF;// used for relay trigger type. Do not change!
boolean kaltstart = true;   //true = Rancilio started for first time
/* Nicht notwendig, ist in function deklariert
String displaymessagetext ;     // display Ausgabe
String displaymessagetext2 ;    // display Ausgabe 2
*/
//double eepromcheck ;            // Eeprom Pr�fung   //Variable wird nicht verwendet
//String eepromcheckstring;   //wenn nicht global ben�tigt, dann nur in function deklarieren


/********************************************************
   moving average - Brüherkennung
*****************************************************/
const int numReadings = 15;             // number of values per Array
float readingstemp[numReadings];        // the readings from Temp
float readingstime[numReadings];        // the readings from time
float readingchangerate[numReadings];

int readIndex = 1;              // the index of the current reading
double total = 0;               // the running
int totaltime = 0 ;             // the running time
double heatrateaverage = 0;     // the average over the numReadings
double changerate = 0;          // local change rate of temprature
double heatrateaveragemin = 0 ;
unsigned long  timeBrewdetection = 0 ;
int timerBrewdetection = 0 ;
int i = 0;
int firstreading = 1 ;          // Ini of the field

/********************************************************
   PID - Werte Brüherkennung Offline
*****************************************************/

double aggbKp =  30 ;
double aggbTn = 0 ;
double aggbTv = 3;
double aggbKi=aggbKp/aggbTn ; 
double aggbKd=aggbTv*aggbKp ; 
double brewtimersoftware = 45;    // 20-5 for detection
double brewboarder = 150 ;        // border for the detection,
// be carefull: to low: risk of wrong brew detection
// and rising temperature

boolean emergencyshutdown = false;

/********************************************************
   Analog Schalter Read
******************************************************/
int analogPin = 0; // will be use in case of hardware
int brewcounter = 0;
int brewswitch = 0;


int brewtime = 25000;
long aktuelleZeit = 0;
int totalbrewtime = 0;
int preinfusion = 2000;
int preinfusionpause = 5000;
unsigned long bezugsZeit = 0;
unsigned long startZeit = 0;

#define pinRelayVentil    12
#define pinRelayPumpe     13


/********************************************************
   Onoffmachine relay
******************************************************/

#define pinRelaymaschineonoff  99 // Your Pin for the relay
int Onoffmachine = 0 ;
long windowStartOnoff = 0 ;

/********************************************************
   OTA
******************************************************/
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
const char* OTAhost = "Rancilio";
const char* OTApass = "12345";

/********************************************************
   Sensor check
******************************************************/
boolean sensorError = false;
int error = 0;
int maxErrorCounter = 10 ;  //depends on intervaltempmes* , define max seconds for invalid data


/********************************************************
   DISPLAY
******************************************************/
#include <U8x8lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=*/ 5, /* data=*/ 4, /* reset=*/ 16);   //Display initalisieren

// Display 128x64
#include <Wire.h>
#include <Adafruit_GFX.h>
//#include <ACROBOTIC_SSD1306.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 16
Adafruit_SSD1306 display(OLED_RESET);
#define XPOS 0
#define YPOS 1
#define DELTAY 2


#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


/********************************************************
   PID
******************************************************/
#include "PID_v1.h"
#define pinRelayHeater    14
long previousMillistemp = 0;
//long currentMillistemp = 0; // Doppelt deklariert
long intervaltempmestsic = 200 ;
long intervaltempmesds18b20 = 400  ;
int pidMode = 1; //1 = Automatic, 0 = Manual

unsigned int windowSize = 1000;
unsigned long windowStartTime;
double acceleration = 1;
double Input, Output, setPointTemp; //, Coldstart;  //wird nicht verwendet
double previousInput = 0;

double setPoint = 95;
float aggKp = 28.0 / acceleration;
float aggTn = 100 / acceleration;
float aggTv = 0 / acceleration;
float startKp = 60;
double starttemp = 85;
double aggKi=aggKp/aggTn ; 
double aggKd=aggTv*aggKp ; 


PID bPID(&Input, &Output, &setPoint, aggKp, aggKi, aggKd, DIRECT);

/********************************************************
   DALLAS TEMP
******************************************************/
// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress sensorDeviceAddress;

/********************************************************
   B+B Sensors TSIC 306
******************************************************/
#include "TSIC.h"       // include the library
TSIC Sensor1(2);    // only Signalpin, VCCpin unused by default
uint16_t temperature = 0;
float Temperatur_C = 0;

/********************************************************
   BLYNK
******************************************************/
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>

//Zeitserver
#include <TimeLib.h>
#include <WidgetRTC.h>
BlynkTimer timer;
WidgetRTC rtc;
WidgetBridge bridge1(V1);

//Update Intervall zur App
unsigned long previousMillis = 0;
const long interval = 1000;
int blynksendcounter = 1;
//Update für Display
unsigned long previousMillisDisplay = 0;
const long intervalDisplay = 500;

/********************************************************
   BLYNK WERTE EINLESEN und Definition der PINS
******************************************************/



BLYNK_CONNECTED() {
  if (Offlinemodus == 0) {
    Blynk.syncAll();
    rtc.begin();
  }
}

BLYNK_WRITE(V4) {
  aggKp = param.asDouble();
}

BLYNK_WRITE(V5) {
  aggTn = param.asDouble();
}
BLYNK_WRITE(V6) {
  aggTv =  param.asDouble();
}

BLYNK_WRITE(V7) {
  setPoint = param.asDouble();
}

BLYNK_WRITE(V8) {
  brewtime = param.asDouble() * 1000;
}

BLYNK_WRITE(V9) {
  preinfusion = param.asDouble() * 1000;
}

BLYNK_WRITE(V10) {
  preinfusionpause = param.asDouble() * 1000;
}
BLYNK_WRITE(V11) {
  startKp = param.asDouble();
}
BLYNK_WRITE(V12) {
  starttemp = param.asDouble();
}
BLYNK_WRITE(V13)
{
  Onoff = param.asInt();
}
BLYNK_WRITE(V30)
{
  aggbKp = param.asDouble();//
}

BLYNK_WRITE(V31) {
  aggbTn = param.asDouble();
}
BLYNK_WRITE(V32) {
  aggbTv =  param.asDouble();
}
BLYNK_WRITE(V33) {
  brewtimersoftware =  param.asDouble();
}
BLYNK_WRITE(V34) {
  brewboarder =  param.asDouble();
}
/********************************************************
 VOID BLynk send Data
*****************************************************/

void blynksenddata() {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        if (Offlinemodus == 0) {
          if(Blynk.connected()){
            Blynk.run();
            blynksendcounter++;
            if (blynksendcounter == 1) {
            Blynk.virtualWrite(V2, Input);
            Blynk.syncVirtual(V2);
            }
            if (blynksendcounter == 2) {
            Blynk.virtualWrite(V23, Output);
            Blynk.syncVirtual(V23);
            }
             if (blynksendcounter == 3) {
            Blynk.virtualWrite(V3, setPoint);
            Blynk.syncVirtual(V3);
            }
            if (blynksendcounter == 4) {
            Blynk.virtualWrite(V35, heatrateaverage);
            Blynk.syncVirtual(V35);
            }
            if (blynksendcounter == 5) {
            Blynk.virtualWrite(V36, heatrateaveragemin);
            Blynk.syncVirtual(V36);   
            blynksendcounter=0;
            }       
          //  Blynk.virtualWrite(V43, debug_timediff);
          //  Blynk.syncVirtual(V43);
          //  debug_timediff = millis()-previousMillis ;
         }
        }
      }
}
/********************************************************
  VOID Displayausgabe
*****************************************************/

void displaymessage(String displaymessagetext, String displaymessagetext2, int Display) {
  if (Display == 2) {
    /********************************************************
       DISPLAY AUSGABE
    ******************************************************/
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(displaymessagetext);
    display.print(displaymessagetext2);
    display.display();
  }
  if (Display == 1) {
    /********************************************************
       DISPLAY AUSGABE
    ******************************************************/
    u8x8.clear();
    u8x8.setFont(u8x8_font_chroma48medium8_r);  //Ausgabe vom aktuellen Wert im Display
    u8x8.setCursor(0, 0);
    u8x8.println(displaymessagetext);
    u8x8.print(displaymessagetext2);
  }

}
/********************************************************
  VOID Brueerkennung
*****************************************************/

void brueherkennung() {
  if (firstreading == 1) {
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
      readingstemp[thisReading] = Input;
      readingstime[thisReading] = 0;
      readingchangerate[thisReading] = 0;
    }
    firstreading = 0 ;
  }

  readingstime[readIndex] = millis() ;
  readingstemp[readIndex] = Input ;

  if (readIndex == numReadings - 1) {
    changerate = (readingstemp[numReadings - 1] - readingstemp[0]) / (readingstime[numReadings - 1] - readingstime[0]) * 10000;
  } else {
    changerate = (readingstemp[readIndex] - readingstemp[readIndex + 1]) / (readingstime[readIndex] - readingstime[readIndex + 1]) * 10000;
  }

  readingchangerate[readIndex] = changerate ;
  total = 0 ;
  for (i = 0; i < numReadings; i++)
  {
    total += readingchangerate[i];
  }

  heatrateaverage = total / numReadings * 100 ;
  if (heatrateaveragemin > heatrateaverage) {
    heatrateaveragemin = heatrateaverage ;
  }

  if (debugmodus == 1) {
    Serial.print("Input: ");
    Serial.print(Input, 4);
    Serial.print(",");
    Serial.print("heataveragemin: ");
    Serial.print(heatrateaveragemin, 4);
    Serial.print(",");
    Serial.print("heataverage: ");
    Serial.print(heatrateaverage, 4);
    Serial.println(",");
  }
  if (readIndex >= numReadings - 1) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }
  readIndex++;

}

boolean checkSensor(float tempInput){
  boolean OK = true;
   /********************************************************
    sensor error
  ******************************************************/
    if ((tempInput < 0 || abs(tempInput-previousInput) > 25) && !sensorError) { 
    error++;
    OK = false;     
  } else if (tempInput > 0 ){
    error = 0;
    OK = true;
  }
  
   if (error >= maxErrorCounter && !sensorError) {
    sensorError = true ;
    Serial.print("Sensor Error:");
    Serial.println(Input);
  } else if (error == 0){
    sensorError = false ;
  }  
  return OK;
}

void temp_emergencyshutdown() {
 if (Input > 130) { 
 emergencyshutdown =  true ;
 }
   
}


void setup() {
  Serial.begin(115200);
  while (! Serial); // Wait untilSerial is ready
  /********************************************************
    Define trigger type
  ******************************************************/
    if(triggerType)
    {
      relayON = HIGH;
      relayOFF = LOW;
    }else{
      relayON = LOW;
      relayOFF = HIGH; 
    }

  /********************************************************
    Ini Pins
  ******************************************************/
  pinMode(pinRelayVentil, OUTPUT);
  pinMode(pinRelayPumpe, OUTPUT);
  pinMode(pinRelayHeater, OUTPUT);
  digitalWrite(pinRelayVentil, relayOFF);
  digitalWrite(pinRelayPumpe, relayOFF);
  digitalWrite(pinRelayHeater, LOW);
  


  if (Display == 1) {
    /********************************************************
      DISPLAY Intern
    ******************************************************/
    u8x8.begin();
    u8x8.setPowerSave(0);
  }
  if (Display == 2) {
    /********************************************************
      DISPLAY 128x64
    ******************************************************/
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)  for AZ Deliv. Display
    //display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
    display.clearDisplay();
  }
  displaymessage("Version 1.8.1 BETA","10.06.2019", Display);
  delay(2000);

  /********************************************************
     BLYNK & Fallback offline
  ******************************************************/
  if (Offlinemodus == 0) {

    if (fallback == 0) {
      displaymessage("Connect to Blynk", "no Fallback", Display);
      Blynk.begin(auth, ssid, pass, blynkaddress, 8080);

    }

    if (fallback == 1) {
      unsigned long started = millis();
      displaymessage("1: Connect Wifi to:", ssid, Display);
      // wait 10 seconds for connection:
      WiFi.begin(ssid, pass);
      Serial.print("Connecting to ");
      Serial.print(ssid);
      Serial.println(" ...");
      //delay(10000);
      
      while ((WiFi.status() != WL_CONNECTED) && (millis() - started < 20000))
      {
        yield();    //Prevent Watchdog trigger
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        displaymessage("2: Wifi connected, ", "try Blynk   ", Display);
        Serial.println("Wifi works, now try Blynk connection");
        delay(2000);
        Blynk.config(auth, blynkaddress, 8080) ;
        Blynk.connect(30000);

        // Blnky works:
        if (Blynk.connected() == true) {
          displaymessage("3: Blynk connected", "", Display);
          delay(2000) ;
          //Serial.println(WiFi.status());

          // Werte in den eeprom schreiben
          // ini eeprom mit begin
          Blynk.run();
          EEPROM.begin(1024);
          Serial.println("Blynk is online, new values to eeprom");
          EEPROM.put(0, aggKp);
          EEPROM.put(10, aggTn);
          EEPROM.put(20, aggTv);
          EEPROM.put(30, setPoint);
          EEPROM.put(40, brewtime);
          EEPROM.put(50, preinfusion);
          EEPROM.put(60, preinfusionpause);
          EEPROM.put(70, startKp);
          EEPROM.put(80, starttemp);
          EEPROM.put(90, aggbKp);
          EEPROM.put(100, aggbTn);
          EEPROM.put(110, aggbTv);
          EEPROM.put(120, brewtimersoftware);
          EEPROM.put(130, brewboarder);
          // eeprom schlie�en
          EEPROM.commit();
        }
      }
      if (WiFi.status() != WL_CONNECTED || Blynk.connected() != true) {
        displaymessage("Begin Fallback,", "No Blynk/Wifi", Display);
        delay(2000);
        Serial.println("Start offline mode with eeprom values, no wifi or blynk :(");
        Offlinemodus = 1 ;
        // eeprom öffnen
        EEPROM.begin(1024);
        // eeprom werte prüfen, ob numerisch
        EEPROM.get(0, aggKp);
        String eepromcheckstring = String(aggKp, 1);
        Serial.println(aggKp);
        Serial.println(eepromcheckstring);
        if (isDigit(eepromcheckstring.charAt(1)) == true) {
          EEPROM.get(0, aggKp);
          EEPROM.get(10, aggTn);
          EEPROM.get(20, aggTv);
          EEPROM.get(30, setPoint);
          EEPROM.get(40, brewtime);
          EEPROM.get(50, preinfusion);
          EEPROM.get(60, preinfusionpause);
          EEPROM.get(70, startKp);
          EEPROM.get(80, starttemp);
          EEPROM.get(90, aggbKp);
          EEPROM.get(100, aggbTn);
          EEPROM.get(110, aggbTv);
          EEPROM.get(120, brewtimersoftware);
          EEPROM.get(130, brewboarder);
        }
        else
        {
          displaymessage("No eeprom,", "Value", Display);
          Serial.println("No working eeprom value, I am sorry, but use default offline value  :)");
          delay(2000);
        }
        // eeeprom schließen
        EEPROM.commit();
      }
    }
    //Serial.println(WiFi.localIP());
  }

  /********************************************************
     OTA
  ******************************************************/
  if (OTA){
    //Check if wifi is already connected, e.g. because of Blynk
    if(WiFi.status() != WL_CONNECTED &&  Offlinemodus == 0 ){
    /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
    would try to act as both a client and an access-point and could cause
    network-issues with your other WiFi-devices on your WiFi-network. */
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, pass);
      Serial.println("WiFi connected");
      while (WiFi.waitForConnectResult() != WL_CONNECTED) {
      WiFi.begin(ssid, pass);
      Serial.println("Retrying connection...");
      }
    }
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    ArduinoOTA.setHostname(OTAhost);  //  Device name for OTA
    ArduinoOTA.setPassword(OTApass);  //  Password for OTA
    ArduinoOTA.begin();
  }


  /********************************************************
     Ini PID
  ******************************************************/

  windowStartTime = millis();
  //Coldstart = 1;  // wird nicht verwendet

  setPointTemp = setPoint;
  bPID.SetSampleTime(windowSize);
  bPID.SetOutputLimits(0, windowSize);
  bPID.SetMode(AUTOMATIC);


  /********************************************************
     TEMP SENSOR
  ******************************************************/
  if (TempSensor == 1) {
    sensors.begin();
    sensors.getAddress(sensorDeviceAddress, 0);
    sensors.setResolution(sensorDeviceAddress, 10) ;
    sensors.requestTemperatures();
  }

  /********************************************************
    movingaverage ini array
  ******************************************************/
  if (Brewdetection == 1) {
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
      readingstemp[thisReading] = 0;
      readingstime[thisReading] = 0;
      readingchangerate[thisReading] = 0;
    }

  }
  /********************************************************
     Onoffmachine relay
  ******************************************************/
  if (standby == 1) {
    pinMode(pinRelaymaschineonoff, OUTPUT) ;
  }
}
void refreshTemp(){
  /********************************************************
    Temp. Request
  ******************************************************/
   previousInput = Input ;
  unsigned long currentMillistemp = millis();
  if (TempSensor == 1)
  {
    if (currentMillistemp - previousMillistemp > intervaltempmesds18b20)
    {
      previousMillistemp = currentMillistemp;
    sensors.requestTemperatures();
    if (!checkSensor(sensors.getTempCByIndex(0))) return;  //if sensor data is not valid, abort function
      Input = sensors.getTempCByIndex(0);

      if (Brewdetection == 1) {
        brueherkennung();
      }
    }
  }
  if (TempSensor == 2)
  {
    if (currentMillistemp - previousMillistemp > intervaltempmestsic)
    {
      previousMillistemp = currentMillistemp;
    /*  variable "temperature" must be set to zero, before reading new data
       *  getTemperature only updates if data is valid, otherwise "temperature" will still hold old values
       */
      Sensor1.getTemperature(&temperature);
      Temperatur_C = Sensor1.calc_Celsius(&temperature);
    if (!checkSensor(Temperatur_C)) return;  //if sensor data is not valid, abort function
      Input = Temperatur_C;
      // Input = random(50,70) ;// test value

      if (Brewdetection == 1)
      {
        brueherkennung();
      }
    }
  }
}

void loop() {

ArduinoOTA.handle();  // For OTA

refreshTemp();
temp_emergencyshutdown();
  /********************************************************
    PreInfusion, Brew , if not Only PID
  ******************************************************/

  //  Serial.print(brewswitch);
  //  Serial.print(",");
  //  Serial.println(OnlyPID);
  if (OnlyPID == 0) {
    brewswitch = analogRead(analogPin);
    unsigned long aktuelleZeit = millis();
    if (brewswitch > 1000 && brewcounter == 0) {
      startZeit = millis();
      brewcounter = brewcounter + 1;
    }
    if (brewcounter >= 1){
      bezugsZeit = aktuelleZeit - startZeit;
    }
    
    totalbrewtime = preinfusion + preinfusionpause + brewtime;
    //Serial.println(brewcounter);
    if (brewswitch > 1000 && bezugsZeit < totalbrewtime && brewcounter >= 1) {
      if (bezugsZeit < preinfusion) {
        //Serial.println("preinfusion");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayON);
        //digitalWrite(pinRelayHeater, relayOFF);
      }
      if (bezugsZeit > preinfusion && bezugsZeit < preinfusion + preinfusionpause) {
        //Serial.println("Pause");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayOFF);
        //digitalWrite(pinRelayHeater, relayOFF);
      }
      if (bezugsZeit > preinfusion + preinfusionpause) {
        //Serial.println("Brew");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayON);
        //digitalWrite(pinRelayHeater, relayON);
      }
    } else {
      digitalWrite(pinRelayVentil, relayOFF);
      digitalWrite(pinRelayPumpe, relayOFF);
      //Serial.println("aus");
    }
    if (brewswitch < 1000 && brewcounter >= 1) {
      brewcounter = 0;
      aktuelleZeit = 0;
      bezugsZeit = 0;
    }
  }
  
  /********************************************************
    change of rate
  ******************************************************/
  //Sicherheitsabfrage
  if (!sensorError) {
    // Brew detecion == 1 software solution , == 2 hardware
    if (Brewdetection == 1 || Brewdetection == 2) {
      if (millis() - timeBrewdetection > 50 * 1000) {
        timerBrewdetection = 0 ;
      }
    }

    if (Brewdetection == 1) {
      // only pid
      if (heatrateaverage <= -brewboarder && brewboarder != 0 && timerBrewdetection == 0 && OnlyPID == 1 ) {
        //   Serial.println("Brewdetected") ;
        timeBrewdetection = millis() ;
        timerBrewdetection = 1 ;
      } // bei Vollausbau
        if (OnlyPID == 0 && brewswitch > 1000 && brewboarder != 0  ) {
        //   Serial.println("Brewdetected") ;
        timeBrewdetection = millis() ;
        timerBrewdetection = 1 ;
      }
    }
    if (Input < starttemp && kaltstart == true) {
      bPID.SetTunings(startKp, 0, 0);
    } else {
    // calc ki kd
    if (aggTn != 0)
    { aggKi=aggKp/aggTn ; } else { aggKi = 0 ;}
     aggKd=aggTv*aggKp ;
      bPID.SetTunings(aggKp, aggKi, aggKd,P_ON_M);
      kaltstart = false;
    }
    if ( millis() - timeBrewdetection  < brewtimersoftware * 1000 && timerBrewdetection == 1) {
    if (aggbTn != 0)
    {   aggbKi=aggbKp/aggbTn ;} else {aggbKi = 0 ;}
    aggbKd=aggbTv*aggbKp ;      
      bPID.SetTunings(aggbKp, aggbKi, aggbKd,P_ON_M) ;
      //   Serial.println("PIDMODEBREW") ;
    }

    bPID.Compute();
  
    //check if PID should run or not. If not, set to manuel and force output to zero
    if (Onoff == 0 && pidMode == 1) {
      pidMode = 0;
      bPID.SetMode(pidMode);
      Output = 0 ;
    }else if (Onoff == 1 && pidMode == 0) {
      pidMode = 1;
      bPID.SetMode(pidMode);
    }
    if (emergencyshutdown == true )
    {
     Output = 0 ;
     digitalWrite(pinRelayHeater, LOW);
    }
    if (millis() - windowStartTime > windowSize) {
      windowStartTime += windowSize;
    }
    if (Output < millis() - windowStartTime)    {
      digitalWrite(pinRelayHeater, LOW);
      //Serial.println("Power off!");
      blynksenddata() ;

    } else {
      digitalWrite(pinRelayHeater, HIGH);
      //Serial.println("Power on!");
      if (Output > 900) {
      blynksenddata() ;  
      }
    }

    /********************************************************
      Sendet Daten ans Display
    ******************************************************/
    unsigned long currentMillisDisplay = millis();
    if (currentMillisDisplay - previousMillisDisplay >= intervalDisplay) {
      previousMillisDisplay = currentMillisDisplay;

      if (Display == 1 && !sensorError && !emergencyshutdown) {

        /********************************************************
           DISPLAY AUSGABE
        ******************************************************/
        u8x8.setFont(u8x8_font_chroma48medium8_r);  //Ausgabe vom aktuellen Wert im Display
        u8x8.setCursor(0, 0);
        u8x8.print("               ");
        u8x8.setCursor(0, 1);
        u8x8.print("               ");
        u8x8.setCursor(0, 2);
        u8x8.print("               ");
        u8x8.setCursor(0, 0);
        u8x8.setCursor(1, 0);
        u8x8.print(bPID.GetKp());
        u8x8.setCursor(6, 0);
        u8x8.print(",");
        u8x8.setCursor(7, 0);
        u8x8.print(bPID.GetKi());
        u8x8.setCursor(11, 0);
        u8x8.print(",");
        u8x8.setCursor(12, 0);
        u8x8.print(bPID.GetKd());
        u8x8.setCursor(0, 1);
        u8x8.print("Input:");
        u8x8.setCursor(9, 1);
        u8x8.print("   ");
        u8x8.setCursor(9, 1);
        u8x8.print(Input);
        u8x8.setCursor(0, 2);
        u8x8.print("SetPoint:");
        u8x8.setCursor(10, 2);
        u8x8.print("   ");
        u8x8.setCursor(10, 2);
        u8x8.print(setPoint);
        u8x8.setCursor(0, 3);
        u8x8.print(round((Input * 100) / setPoint));
        u8x8.setCursor(4, 3);
        u8x8.print("%");
        u8x8.setCursor(6, 3);
        u8x8.print(Output);
      }
      if (Display == 2 && !sensorError &&!emergencyshutdown) {
        /********************************************************
           DISPLAY AUSGABE
        ******************************************************/
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.clearDisplay();
        display.setCursor(0, 0);
       display.print("Ist-T: ");
     //  display.print("  ");
        display.setTextSize(2);
        display.println(Input);
        display.setTextSize(1);
       display.print("Soll-T:");
       display.setTextSize(2);
    //    display.print("/");
    //    display.print(" ");
        display.print(setPoint);
      //  display.print("PID-Outlet:");
      //  display.println(Output/10);
    //    display.print("PID:");
     //   display.print(" ");
     //   display.print(bPID.GetKp());
     //   display.print(",");
     //   display.print(bPID.GetKi());
     //   display.print(",");
     //   display.println(bPID.GetKd());
     display.setTextSize(1);
        display.println();
        display.println();
        display.println("Bezugszeit:");
        display.setTextSize(2);
        display.print("      ");
        display.print(bezugsZeit/1000);
     //   display.print("/");
     //   display.println(totalbrewtime/1000);
        display.setTextSize(1);
        display.setCursor(0, 48);
        //display.print(preinfusion/1000);
      //  display.print("/");
     //   display.print(preinfusionpause/1000);
      //  display.print("/");
     //   display.print(brewtime/1000);
        display.display();
      }

    }
  } else if (sensorError) {
    
  //Deactivate PID
    if (pidMode == 1) {
      pidMode = 0;
      bPID.SetMode(pidMode);
      Output = 0 ;
    }
    
  //Stop heating
    digitalWrite(pinRelayHeater, LOW);  
    
    if (Display == 2) {
      /********************************************************
         DISPLAY AUSGABE
      ******************************************************/
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Error:");
      display.print("  ");
      display.println(Input);
      display.print("Check Temp. Sensor!");
      display.display();
    }
    if (Display == 1) {
      /********************************************************
         DISPLAY AUSGABE
      ******************************************************/
      u8x8.setFont(u8x8_font_chroma48medium8_r);  //Ausgabe vom aktuellen Wert im Display
      u8x8.setCursor(0, 0);
      u8x8.print("               ");
      u8x8.setCursor(0, 1);
      u8x8.print("               ");
      u8x8.setCursor(0, 2);
      u8x8.print("               ");
      u8x8.setCursor(0, 1);
      u8x8.print("Error: Temp.");
      u8x8.setCursor(0, 2);
      u8x8.print(Input);
    }
  }
   if (emergencyshutdown) {
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Emergency Shutdown");
      display.print("Temp > 130!");
      display.display();
   }
}
