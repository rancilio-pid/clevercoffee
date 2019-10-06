/********************************************************
   Version 1.9.7 BETA (06.10.2019)
  Key facts: major revision
  - Check the PIN Ports in the CODE!
  - Find your changerate of the machine, can be wrong, test it!
******************************************************/

/********************************************************
  INCLUDES
******************************************************/       
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include "userConfig.h" // needs to be configured by the user
#include <Wire.h>
#include <Adafruit_GFX.h>   
#include <Adafruit_SSD1306.h>
#include "PID_v1.h" //for PID calculation
#include <OneWire.h>    //Library for one wire communication to temp sensor
#include <DallasTemperature.h>    //Library for dallas temp sensor
#include "TSIC.h"       //Library for TSIC temp sensor
#include <BlynkSimpleEsp8266.h>
#include "icon.h"   //user icons for display

/********************************************************
  DEFINES
******************************************************/  
//#define DEBUGMODE   // Debug mode is active if #define DEBUGMODE is set

#ifndef DEBUGMODE
#define DEBUG_println(a)
#define DEBUG_print(a)
#define DEBUGSTART(a)
#else
#define DEBUG_println(a) Serial.println(a);
#define DEBUG_print(a) Serial.print(a);
#define DEBUGSTART(a) Serial.begin(a);
#endif

//#define BLYNK_PRINT Serial    // In detail debugging for blynk
//#define BLYNK_DEBUG

#define pinRelayVentil    12    //Output pin for 3-way-valve
#define pinRelayPumpe     13    //Output pin for pump
#define pinRelayHeater    14    //Output pin for heater

#define OLED_RESET 16     //Output pin for dispaly reset pin

#define ONE_WIRE_BUS 2  // Data wire is plugged into port 2 on the Arduino


/********************************************************
   DISPLAY
******************************************************/    
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels  
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire,OLED_RESET);   

/********************************************************
  definitions below must be changed in the userConfig.h file
******************************************************/
int Offlinemodus = OFFLINEMODUS;
const int Display = DISPLAY;
const int OnlyPID = ONLYPID;
const int TempSensor = TEMPSENSOR;
const int Brewdetection = BREWDETECTION;
const int fallback = FALLBACK;
const int triggerType = TRIGGERTYPE;
const boolean ota = OTA;
const int grafana=GRAFANA;
const unsigned long wifiConnectionDelay = WIFICINNECTIONDELAY;
const unsigned int maxWifiReconnects = MAXWIFIRECONNECTS;

// Wifi
const char* auth = AUTH;
const char* ssid = D_SSID;
const char* pass = PASS;
unsigned long lastWifiConnectionAttempt = millis();
unsigned int wifiReconnects = 0; //actual number of reconnects

// OTA
const char* OTAhost = OTAHOST;
const char* OTApass = OTAPASS;

//Blynk
const char* blynkaddress  = BLYNKADDRESS;


/********************************************************
   declarations
******************************************************/
int pidON = 1 ;                 // 1 = control loop in closed loop
int relayON, relayOFF;          // used for relay trigger type. Do not change!
boolean kaltstart = true;       // true = Rancilio started for first time
boolean emergencyStop = false;  // Notstop bei zu hoher Temperatur
const char* sysVersion PROGMEM  = "Version 1.9.7 BETA";   //System version
int inX = 0, inY = 0, inOld = 0, inSum = 0; //used for filter()
int bars = 0; //used for getSignalStrength()

/********************************************************
   moving average - Brüherkennung
*****************************************************/
const int numReadings = 15;             // number of values per Array
double readingstemp[numReadings];        // the readings from Temp
unsigned long readingstime[numReadings];        // the readings from time
double readingchangerate[numReadings];

int readIndex = 1;              // the index of the current reading
double total = 0;               // total sum of readingchangerate[]
double heatrateaverage = 0;     // the average over the numReadings
double changerate = 0;          // local change rate of temprature
double heatrateaveragemin = 0 ;
unsigned long  timeBrewdetection = 0 ;
int timerBrewdetection = 0 ;    // flag is set if brew was detected
int firstreading = 1 ;          // Ini of the field, also used for sensor check

/********************************************************
   PID - Werte Brüherkennung Offline
*****************************************************/
double aggbKp = AGGBKP;
double aggbTn = AGGBTN;
double aggbTv = AGGBTV;
#if (aggbTn == 0)
double aggbKi = 0;
#else
double aggbKi = aggbKp / aggbTn;
#endif
double aggbKd = aggbTv * aggbKp ;
double brewtimersoftware = 45;    // 20-5 for detection
double brewboarder = 150 ;        // border for the detection; to low: risk of wrong brew detection and rising temperature
const int PonE = PONE;

/********************************************************
   Analog Input
******************************************************/
const int analogPin = 0; // AI0 will be used
int brewcounter = 0;
int brewswitch = 0;
double brewtime = 25000;  //brewtime in ms
double totalbrewtime = 0; //total brewtime set in softare or blynk
double preinfusion = 2000;  //preinfusion time in ms
double preinfusionpause = 5000;   //preinfusion pause time in ms
unsigned long bezugsZeit = 0;   //total brewed time
unsigned long startZeit = 0;    //start time of brew

/********************************************************
   Sensor check
******************************************************/
boolean sensorError = false;
int error = 0;
int maxErrorCounter = 10 ;  //depends on intervaltempmes* , define max seconds for invalid data

/********************************************************
   PID
******************************************************/
unsigned long previousMillistemp;  // initialisation at the end of init()
const unsigned long intervaltempmestsic = 400 ;
const unsigned long intervaltempmesds18b20 = 400  ;
int pidMode = 1; //1 = Automatic, 0 = Manual

const unsigned int windowSize = 1000;
unsigned int isrCounter = 0;  // counter for ISR
unsigned long windowStartTime;
double Input, Output;
double setPointTemp;
double previousInput = 0;

double setPoint = SETPOINT;
double aggKp = AGGKP;
double aggTn = AGGTN;
double aggTv = AGGTV;
double startKp = STARTKP;
double startTn = STARTTN;
#if (startTn == 0)
double startKi = 0;
#else
double startKi = startKp / startTn;
#endif

double starttemp = STARTTEMP;
#if (aggTn == 0)
double aggKi = 0;
#else
double aggKi = aggKp / aggTn;
#endif
double aggKd = aggTv * aggKp ;

PID bPID(&Input, &Output, &setPoint, aggKp, aggKi, aggKd, PonE, DIRECT);    //PID initialisation

/********************************************************
   DALLAS TEMP
******************************************************/
OneWire oneWire(ONE_WIRE_BUS);  // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature.
DeviceAddress sensorDeviceAddress;  // arrays to hold device address

/********************************************************
   Temp Sensors TSIC 306
******************************************************/
TSIC Sensor1(2);    // only Signalpin, VCCpin unused by default
uint16_t temperature = 0; //internal variable used to read temeprature
float Temperatur_C = 0; 

/********************************************************
   BLYNK
******************************************************/
//Update Intervall zur App
unsigned long previousMillisBlynk;  // initialisation at the end of init()
const unsigned long intervalBlynk = 1000;
int blynksendcounter = 1;

//Update für Display
unsigned long previousMillisDisplay;  // initialisation at the end of init()
const unsigned long intervalDisplay = 500;

/********************************************************
   BLYNK WERTE EINLESEN und Definition der PINS
******************************************************/
BLYNK_CONNECTED() {
  if (Offlinemodus == 0) {
    Blynk.syncAll();
    //rtc.begin();
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
  pidON = param.asInt();
}
BLYNK_WRITE(V14)
{
  startTn = param.asDouble();
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
  Notstop wenn Temp zu hoch
*****************************************************/
void testEmergencyStop(){
  if (Input > 120){
    emergencyStop = true;
  } else if (Input < 100) {
    emergencyStop = false;
  }
}

/********************************************************
  Displayausgabe
*****************************************************/
void displaymessage(String displaymessagetext, String displaymessagetext2) {
  if (Display == 2) {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.clearDisplay();
    display.setCursor(0, 47);
    display.println(displaymessagetext);
    display.print(displaymessagetext2);
    //Rancilio startup logo
    display.drawBitmap(41,2, startLogo_bits,startLogo_width, startLogo_height, WHITE);
    //draw circle
    //display.drawCircle(63, 24, 20, WHITE);
    display.display();
   // display.fadeout();
   // display.fadein();
  }
}

/********************************************************
  Moving average - brewdetection (SW)
*****************************************************/
void movAvg() {
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
  for (int i = 0; i < numReadings; i++)
  {
    total += readingchangerate[i];
  }

  heatrateaverage = total / numReadings * 100 ;
  if (heatrateaveragemin > heatrateaverage) {
    heatrateaveragemin = heatrateaverage ;
  }

  if (readIndex >= numReadings - 1) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }
  readIndex++;

}

/********************************************************
  check sensor value. If < 0 or difference between old and new >25, then increase error.
  If error is equal to maxErrorCounter, then set sensorError
*****************************************************/
boolean checkSensor(float tempInput) {
  boolean sensorOK = false;
  /********************************************************
    sensor error
  ******************************************************/
  if ((tempInput < 0 || abs(tempInput - previousInput) > 25) && !sensorError) {
    error++;
    sensorOK = false;
    DEBUG_print("Error counter: ");
    DEBUG_println(error);
    DEBUG_print("temp delta: ");
    DEBUG_println(tempInput);
  } else if (tempInput > 0) {
    error = 0;
    sensorOK = true;
  }
  if (error >= maxErrorCounter && !sensorError) {
    sensorError = true ;
    DEBUG_print("Sensor Error");
    DEBUG_println(Input);
  } else if (error == 0) {
    sensorError = false ;
  }

  return sensorOK;
}

/********************************************************
  Refresh temperature.
  Each time checkSensor() is called to verify the value.
  If the value is not valid, new data is not stored.
*****************************************************/
void refreshTemp() {
  /********************************************************
    Temp. Request
  ******************************************************/
  unsigned long currentMillistemp = millis();
  previousInput = Input ;
  if (TempSensor == 1)
  {
    if (currentMillistemp - previousMillistemp >= intervaltempmesds18b20)
    {
      previousMillistemp += intervaltempmesds18b20;
      sensors.requestTemperatures();
      if (!checkSensor(sensors.getTempCByIndex(0)) && firstreading == 0) return;  //if sensor data is not valid, abort function
      Input = sensors.getTempCByIndex(0);
      if (Brewdetection == 1) {
        movAvg();
      } else {
        firstreading = 0;
      }
    }
  }
  if (TempSensor == 2)
  {
    if (currentMillistemp - previousMillistemp >= intervaltempmestsic)
    {
      previousMillistemp += intervaltempmestsic;
      /*  variable "temperature" must be set to zero, before reading new data
            getTemperature only updates if data is valid, otherwise "temperature" will still hold old values
      */
      temperature = 0;
      Sensor1.getTemperature(&temperature);
      Temperatur_C = Sensor1.calc_Celsius(&temperature);
      if (!checkSensor(Temperatur_C) && firstreading == 0) return;  //if sensor data is not valid, abort function
      Input = Temperatur_C;
      // Input = random(50,70) ;// test value
      if (Brewdetection == 1)
      {
        movAvg();
      } else {
        firstreading = 0;
      }
    }
  }
}

/********************************************************
    PreInfusion, Brew , if not Only PID
******************************************************/
void brew() {
  if (OnlyPID == 0) {
    brewswitch = filter(analogRead(analogPin));
    unsigned long aktuelleZeit = millis();
    if (brewswitch > 1000 && brewcounter == 0) {
      startZeit = millis();
      brewcounter = brewcounter + 1;
    }
    if (brewcounter >= 1) {
      bezugsZeit = aktuelleZeit - startZeit;
    }

    totalbrewtime = preinfusion + preinfusionpause + brewtime;
    if (brewswitch > 1000 && bezugsZeit < totalbrewtime && brewcounter >= 1) {
      if (bezugsZeit < preinfusion) {
        //DEBUG_println("preinfusion");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayON);
      }
      if (bezugsZeit > preinfusion && bezugsZeit < preinfusion + preinfusionpause) {
        //DEBUG_println("Pause");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayOFF);
      }
      if (bezugsZeit > preinfusion + preinfusionpause) {
        //DEBUG_println("Brew");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayON);
      }
    } else {
      //DEBUG_println("aus");
      digitalWrite(pinRelayVentil, relayOFF);
      digitalWrite(pinRelayPumpe, relayOFF);
    }
    if (brewswitch < 1000 && brewcounter >= 1) {
      brewcounter = 0;
      aktuelleZeit = 0;
      bezugsZeit = 0;
    }
  }
}

/********************************************************
   Check if Wifi is connected, if not reconnect
 *****************************************************/
 void checkWifi(){
  if (Offlinemodus == 1 || brewcounter == 1) return;
  if ((millis() - lastWifiConnectionAttempt >= wifiConnectionDelay) && (wifiReconnects >= maxWifiReconnects)) {
    int statusTemp = WiFi.status();
    // check WiFi connection:
    if (statusTemp != WL_CONNECTED) {       
      lastWifiConnectionAttempt = millis();      
      // attempt to connect to Wifi network:
      WiFi.begin(ssid, pass); 
      delay(5000);    //will not work without delay
      wifiReconnects++;    
    }
  }
}

/********************************************************
    send data to display
******************************************************/
void printScreen() {
  if (Display == 2 && !sensorError) {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.clearDisplay();
    display.setCursor(0, 0);
    if (Offlinemodus == 0) {
      getSignalStrength();
      if (WiFi.status() == WL_CONNECTED){
        display.drawBitmap(0,0,antenna_OK,8,8,WHITE);
        for (int b=0; b <= bars; b++) {
          display.drawFastVLine(5 + (b*2),8 - (b*2),b*2,WHITE);
        }
      } else {
        display.drawBitmap(0,0,antenna_NOK,8,8,WHITE);
        display.setCursor(48, 0);
        display.print("Reconect: ");
        display.print(wifiReconnects);
      }
      if (Blynk.connected()){
        display.drawBitmap(20,0,blynk_OK,11,8,WHITE);
      } else {
        display.drawBitmap(20,0,blynk_NOK,8,8,WHITE);
      }
    } else {
      display.print("Offlinemodus");
    }    
    display.println("");
    display.println("");
    display.print("S:");
    display.setTextSize(2);
    if (setPoint < 100){
      display.print(setPoint, 1);
    }else{
      display.print(setPoint, 0);
    }         
    display.setTextSize(1);
    display.print(" I:");
    display.setTextSize(2);
    if (Input < 100){
      display.println(Input, 1);
    }else{
      display.println(Input, 0);
    }
    display.setTextSize(1);
    //display.setCursor(0, 32);
    display.print("Q: ");
    display.print(Output / 10, 1);
    display.print(" %, ");
    display.print(bPID.GetKp(), 1);
    display.print(",");
    display.println(bPID.GetKp() / bPID.GetKi(), 0);
    display.print("Bezugszeit:");
    display.setTextSize(2);
    display.print(bezugsZeit / 1000);
    display.print("/");
    display.println(totalbrewtime / 1000);
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(preinfusion / 1000);
    display.print("/");   
    display.print(preinfusionpause / 1000);
    display.print("/");      
    display.print(brewtime / 1000);
    display.display();
  }
}

/********************************************************
  send data to Blynk server
*****************************************************/
void sendToBlynk() {
  if (Offlinemodus != 0) return;
  unsigned long currentMillisBlynk = millis();
  if (currentMillisBlynk - previousMillisBlynk >= intervalBlynk) {
    previousMillisBlynk += intervalBlynk;
    if (Blynk.connected()) {
      if (grafana == 1) {
        Blynk.virtualWrite(V60, Input, Output,bPID.GetKp(),bPID.GetKi(),bPID.GetKd(),setPoint );
        }
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
      if (blynksendcounter >= 5) {
        Blynk.virtualWrite(V36, heatrateaveragemin);
        Blynk.syncVirtual(V36);
        blynksendcounter = 0;
      }
      blynksendcounter++;
    }
  }
}

/********************************************************
    Brewdetection
******************************************************/
void brewdetection() {
  if (brewboarder == 0) return; //abort brewdetection if deactivated

  // Brew detecion == 1 software solution , == 2 hardware
  if (Brewdetection == 1 || Brewdetection == 2) {
    if (millis() - timeBrewdetection > brewtimersoftware * 1000) {
      timerBrewdetection = 0 ;
        if (OnlyPID == 1) {
      bezugsZeit = 0 ;
        }
    }
  }

  if (Brewdetection == 1) {
    if (heatrateaverage <= -brewboarder && timerBrewdetection == 0 ) {
      DEBUG_println("SW Brew detected") ;
      timeBrewdetection = millis() ;
      timerBrewdetection = 1 ;
    }
  }
}

/********************************************************
  nach ca. 28 Zyklen ist der input zu 99,99% erreicht
*****************************************************/
int filter(int input){
  inX = input * 0.3;
  inY = inOld * 0.7;
  inSum = inX + inY;
  inOld = inSum;
  
  return inSum;
}

/********************************************************
  Get Wifi signal strength and set bars for display
*****************************************************/
void getSignalStrength(){
  if (Offlinemodus == 1) return;
  
  long rssi;
  if (WiFi.status() == WL_CONNECTED) {
    rssi = WiFi.RSSI();  
  } else {
    rssi = -100;
  }

  if (rssi >= -50) { 
    bars = 4;
  } else if (rssi < -50 & rssi >= -65) {
    bars = 3;
  } else if (rssi < -65 & rssi >= -75) {
    bars = 2;
  } else if (rssi < -75 & rssi >= -80) {
    bars = 1;
  } else {
    bars = 0;
  }
}

/********************************************************
    Timer 1 - ISR für PID Berechnung und Heizrelais-Ausgabe
******************************************************/
void ICACHE_RAM_ATTR onTimer1ISR() {
  timer1_write(50000); // set interrupt time to 10ms

  if (Output <= isrCounter) {
    digitalWrite(pinRelayHeater, LOW);
    //DEBUG_println("Power off!");
  } else {
    digitalWrite(pinRelayHeater, HIGH);
    //DEBUG_println("Power on!");
  }

  isrCounter += 10; // += 10 because one tick = 10ms
  //set PID output as relais commands
  if (isrCounter > windowSize) {
    isrCounter = 0;
  }

  //run PID calculation
  bPID.Compute();
}

void setup() {
  DEBUGSTART(115200);
  /********************************************************
    Define trigger type
  ******************************************************/
  if (triggerType)
  {
    relayON = HIGH;
    relayOFF = LOW;
  } else {
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

  if (Display == 2) {
    /********************************************************
      DISPLAY 128x64
    ******************************************************/
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)  for AZ Deliv. Display
    //display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
    display.clearDisplay();
  }
  displaymessage(sysVersion, "");
  delay(2000);

  /********************************************************
     BLYNK & Fallback offline
  ******************************************************/
  if (Offlinemodus == 0) {

    if (fallback == 0) {

      displaymessage("Connect to Blynk", "no Fallback");
      Blynk.begin(auth, ssid, pass, blynkaddress, 8080);
    }

    if (fallback == 1) {
      unsigned long started = millis();
      displaymessage("1: Connect Wifi to:", ssid);
      // wait 10 seconds for connection:
      /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
        would try to act as both a client and an access-point and could cause
        network-issues with your other WiFi-devices on your WiFi-network. */
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, pass);
      DEBUG_print("Connecting to ");
      DEBUG_print(ssid);
      DEBUG_println(" ...");

      while ((WiFi.status() != WL_CONNECTED) && (millis() - started < 20000))
      {
        yield();    //Prevent Watchdog trigger
      }

      if (WiFi.status() == WL_CONNECTED) {
        DEBUG_println("WiFi connected");
        DEBUG_println("IP address: ");
        DEBUG_println(WiFi.localIP());
        displaymessage("2: Wifi connected, ", "try Blynk   ");
        DEBUG_println("Wifi works, now try Blynk connection");
        delay(2000);
        Blynk.config(auth, blynkaddress, 8080) ;
        Blynk.connect(30000);

        // Blnky works:
        if (Blynk.connected() == true) {
          displaymessage("3: Blynk connected", "sync all variables...");
          DEBUG_println("Blynk is online, new values to eeprom");
         // Blynk.run() ; 
          Blynk.syncVirtual(V4);
          Blynk.syncVirtual(V5);
          Blynk.syncVirtual(V6);
          Blynk.syncVirtual(V7);
          Blynk.syncVirtual(V8);
          Blynk.syncVirtual(V9);
          Blynk.syncVirtual(V10);
          Blynk.syncVirtual(V11);
          Blynk.syncVirtual(V12);
          Blynk.syncVirtual(V13);
          Blynk.syncVirtual(V14);
          Blynk.syncVirtual(V30);
          Blynk.syncVirtual(V31);
          Blynk.syncVirtual(V32);
          Blynk.syncVirtual(V33);
          Blynk.syncVirtual(V34);
         // Blynk.syncAll();  //sync all values from Blynk server
          // Werte in den eeprom schreiben
          // ini eeprom mit begin
          EEPROM.begin(1024);
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
          // eeprom schließen
          EEPROM.commit();
        }
      }
      if (WiFi.status() != WL_CONNECTED || Blynk.connected() != true) {
        displaymessage("Begin Fallback,", "No Blynk/Wifi");
        delay(2000);
        DEBUG_println("Start offline mode with eeprom values, no wifi or blynk :(");
        Offlinemodus = 1 ;
        // eeprom öffnen
        EEPROM.begin(1024);
        // eeprom werte prüfen, ob numerisch
        double dummy;
        EEPROM.get(0, dummy);
        DEBUG_print("check eeprom 0x00 in dummy: ");
        DEBUG_println(dummy);
        if (!isnan(dummy)) {
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
          displaymessage("No eeprom,", "Value");
          DEBUG_println("No working eeprom value, I am sorry, but use default offline value  :)");
          delay(2000);
        }
        // eeeprom schließen
        EEPROM.commit();
      }
    }
  }

  /********************************************************
     OTA
  ******************************************************/
  if (ota && Offlinemodus == 0 ) {
    //wifi connection is done during blynk connection
    ArduinoOTA.setHostname(OTAhost);  //  Device name for OTA
    ArduinoOTA.setPassword(OTApass);  //  Password for OTA
    ArduinoOTA.begin();
  }


  /********************************************************
     Ini PID
  ******************************************************/

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

  //Initialisation MUST be at the very end of the init(), otherwise the time comparision in loop() will have a big offset
  unsigned long currentTime = millis();
  previousMillistemp = currentTime;
  windowStartTime = currentTime;
  previousMillisDisplay = currentTime;
  previousMillisBlynk = currentTime;

  /********************************************************
    Timer1 ISR - Initialisierung
    TIM_DIV1 = 0,   //80MHz (80 ticks/us - 104857.588 us max)
    TIM_DIV16 = 1,  //5MHz (5 ticks/us - 1677721.4 us max)
    TIM_DIV256 = 3  //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
  ******************************************************/
  timer1_isr_init();
  timer1_attachInterrupt(onTimer1ISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(50000); // set interrupt time to 10ms

}

void loop() {
  if (WiFi.status() == WL_CONNECTED){
    if (Offlinemodus == 1) return;
    ArduinoOTA.handle();  // For OTA
    // Disable interrupt it OTA is starting, otherwise it will not work
    ArduinoOTA.onStart([](){
      timer1_disable();
      digitalWrite(pinRelayHeater, LOW); //Stop heating
    });
    ArduinoOTA.onError([](ota_error_t error) {
      timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
    });
    // Enable interrupts if OTA is finished
    ArduinoOTA.onEnd([](){
      timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
    });
    
    Blynk.run(); //Do Blynk magic stuff
    wifiReconnects = 0;
  } else {
    if (Offlinemodus == 1) return;
    checkWifi();
  }

  refreshTemp();   //read new temperature values
  testEmergencyStop();  // test if Temp is to high
  brew();   //start brewing if button pressed

  //check if PID should run or not. If not, set to manuel and force output to zero
  if (pidON == 0 && pidMode == 1) {
    pidMode = 0;
    bPID.SetMode(pidMode);
    Output = 0 ;
  } else if (pidON == 1 && pidMode == 0) {
    pidMode = 1;
    bPID.SetMode(pidMode);
  }


  //Sicherheitsabfrage
  if (!sensorError && Input > 0 && !emergencyStop) {

    //Set PID if first start of machine detected
    if (Input < starttemp && kaltstart) {
      if (startTn != 0) {
        startKi = startKp / startTn;
      } else {
        startKi = 0 ;
      }
      bPID.SetTunings(startKp, startKi, 0);      
    } else if(timerBrewdetection == 0) {
      // calc ki, kd
      if (aggTn != 0) {
        aggKi = aggKp / aggTn ;
      } else {
        aggKi = 0 ;
      }
      aggKd = aggTv * aggKp ;
      bPID.SetTunings(aggKp, aggKi, aggKd);
      kaltstart = false;
    }

    //if brew detected, set PID values
    brewdetection();
    if ( millis() - timeBrewdetection  < brewtimersoftware * 1000 && timerBrewdetection == 1) {
      // calc ki, kd
      if (aggbTn != 0) {
        aggbKi = aggbKp / aggbTn ;
      } else {
        aggbKi = 0 ;
      }
      aggbKd = aggbTv * aggbKp ;
      bPID.SetTunings(aggbKp, aggbKi, aggbKd) ;
      if(OnlyPID == 1){
      bezugsZeit= millis() - timeBrewdetection ;
      }
    }

    sendToBlynk();

    //update display if time interval xpired
    unsigned long currentMillisDisplay = millis();
    if (currentMillisDisplay - previousMillisDisplay >= intervalDisplay) {
      previousMillisDisplay += intervalDisplay;
      printScreen();
    }

  } else if (sensorError) {

    //Deactivate PID
    if (pidMode == 1) {
      pidMode = 0;
      bPID.SetMode(pidMode);
      Output = 0 ;
    }

    digitalWrite(pinRelayHeater, LOW); //Stop heating

    //DISPLAY AUSGABE
    if (Display == 2) {
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Error: Temp = ");
      display.println(Input);
      display.print("Check Temp. Sensor!");
      display.display();
    }
  } else if (emergencyStop){

    //Deactivate PID
    if (pidMode == 1) {
      pidMode = 0;
      bPID.SetMode(pidMode);
      Output = 0 ;
    }
        
    digitalWrite(pinRelayHeater, LOW); //Stop heating

    //DISPLAY AUSGABE
    if (Display == 2) {
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Emergency Stop!");
      display.println("");
      display.println("Temp > 120");
      display.print("Temp: ");
      display.println(Input);
      display.print("Resume if Temp < 100");
      display.display();
    }
  }
}
