/********************************************************
   Version 2.8.1 (20.03.2021) 
   * Fix coldstart near setpoint 
   * Port for ESp32 
******************************************************/

/********************************************************
  INCLUDES
******************************************************/
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include "userConfig.h" // needs to be configured by the user
#include <U8g2lib.h>
#include "PID_v1.h" //for PID calculation
#include <DallasTemperature.h>    //Library for dallas temp sensor
#if defined(ESP8266) 
  #include <BlynkSimpleEsp8266.h>
#endif
#if defined(ESP32) 
  #include <BlynkSimpleEsp32.h>
  #include <os.h> 
  hw_timer_t * timer = NULL;
#endif
#include "icon.h"   //user icons for display
#include <ZACwire.h> //NEW TSIC LIB
#include <PubSubClient.h>
#include "TSIC.h"       //Library for TSIC temp sensor
#include <Adafruit_VL53L0X.h> //for TOF 





/********************************************************
  DEFINES
******************************************************/
#define DEBUGMODE   // Debug mode is active if #define DEBUGMODE is set

//#define BLYNK_PRINT Serial    // In detail debugging for blynk
//#define BLYNK_DEBUG

#ifndef DEBUGMODE
#define DEBUG_println(a)
#define DEBUG_print(a)
#define DEBUGSTART(a)
#else
#define DEBUG_println(a) Serial.println(a);
#define DEBUG_print(a) Serial.print(a);
#define DEBUGSTART(a) Serial.begin(a);
#endif
#define HIGH_ACCURACY


/********************************************************
  definitions below must be changed in the userConfig.h file
******************************************************/
int Offlinemodus = OFFLINEMODUS;
const int OnlyPID = ONLYPID;
const int TempSensor = TEMPSENSOR;
const int Brewdetection = BREWDETECTION;
const int fallback = FALLBACK;
const int triggerType = TRIGGERTYPE;
const int VoltageSensorType = VOLTAGESENSORTYPE;
const boolean ota = OTA;
const int grafana = GRAFANA;
const unsigned long wifiConnectionDelay = WIFICINNECTIONDELAY;
const unsigned int maxWifiReconnects = MAXWIFIRECONNECTS;
int machineLogo = MACHINELOGO;
const unsigned long brewswitchDelay = BREWSWITCHDELAY;
int machinestate = 0;

//Display
uint8_t oled_i2c = OLED_I2C;

//TOF
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
int calibration_mode = CALIBRATION_MODE;
uint8_t tof_i2c = TOF_I2C;
int water_full = WATER_FULL;
int water_empty = WATER_EMPTY;
double distance;
double percentage;


// Wifi

const char* hostname = HOSTNAME;
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
const int blynkport = BLYNKPORT;
unsigned int blynkReCnctFlag;  // Blynk Reconnection Flag
unsigned int blynkReCnctCount = 0;  // Blynk Reconnection counter
unsigned long lastBlynkConnectionAttempt = millis();

//backflush values
const unsigned long fillTime = FILLTIME;
const unsigned long flushTime = FLUSHTIME;
int maxflushCycles = MAXFLUSHCYCLES;

//MQTT
WiFiClient net;
PubSubClient mqtt(net);
const char* mqtt_server_ip = MQTT_SERVER_IP;
const int mqtt_server_port = MQTT_SERVER_PORT;
const char* mqtt_username = MQTT_USERNAME;
const char* mqtt_password = MQTT_PASSWORD;
const char* mqtt_topic_prefix = MQTT_TOPIC_PREFIX;
char topic_will[256];
char topic_set[256];
unsigned long lastMQTTConnectionAttempt = millis();
unsigned int MQTTReCnctFlag;  // Blynk Reconnection Flag
unsigned int MQTTReCnctCount = 0;  // Blynk Reconnection counter

//Voltage Sensor
unsigned long previousMillisVoltagesensorreading = millis();
const unsigned long intervalVoltagesensor= 200 ;
int VoltageSensorON, VoltageSensorOFF;

/********************************************************
   declarations
******************************************************/
int pidON = 1 ;                 // 1 = control loop in closed loop
int relayON, relayOFF;          // used for relay trigger type. Do not change!
boolean kaltstart = true;       // true = Rancilio started for first time
boolean emergencyStop = false;  // Notstop bei zu hoher Temperatur
double EmergencyStopTemp = 120; // Temp EmergencyStopTemp
const char* sysVersion PROGMEM  = "Version 2.8.1 MASTER";   //System version
int inX = 0, inY = 0, inOld = 0, inSum = 0; //used for filter()
int bars = 0; //used for getSignalStrength()
boolean brewDetected = 0;
boolean setupDone = false;
int backflushON = 0;            // 1 = activate backflush
int flushCycles = 0;            // number of active flush cycles
int backflushState = 10;        // counter for state machine

/********************************************************
   moving average - brewdetection
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
   PID - values for offline brewdetection
*****************************************************/
double aggbKp = AGGBKP;
double aggbTn = AGGBTN;
double aggbTv = AGGBTV;
#if aggbTn == 0
double aggbKi = 0;
#else
double aggbKi = aggbKp / aggbTn;
#endif
double aggbKd = aggbTv * aggbKp ;
double brewtimersoftware = 45;    // 20-5 for detection
double brewboarder = 150 ;        // border for the detection, be carefull: to low: risk of wrong brew detection and rising temperature
const int PonE = PONE;

/********************************************************
   Analog Input
******************************************************/
const int analogPin = 0; // AI0 will be used
int brewcounter = 10;
int brewswitch = 0;
boolean brewswitchWasOFF = false;
double brewtime = 25000;  //brewtime in ms
double totalbrewtime = 0; //total brewtime set in softare or blynk
double preinfusion = 2000;  //preinfusion time in ms
double preinfusionpause = 5000;   //preinfusion pause time in ms
double bezugsZeit = 0;   //total brewed time
double lastbezugszeitMillis = 0; // for shottimer delay after disarmed button
double lastbezugszeit = 0 ; 
unsigned long startZeit = 0;    //start time of brew
const unsigned long analogreadingtimeinterval = 10 ; // ms
unsigned long previousMillistempanalogreading ; // ms for analogreading

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

double BrewSetPoint = SETPOINT;
double setPoint = BrewSetPoint;
double SteamSetPoint = STEAMSETPOINT;
int    SteamON = 0;
int    SteamFirstON = 0;
double aggKp = AGGKP;
double aggTn = AGGTN;
double aggTv = AGGTV;
double startKp = STARTKP;
double startTn = STARTTN;
#if startTn == 0
double startKi = 0;
#else
double startKi = startKp / startTn;
#endif

#if aggTn == 0
double aggKi = 0;
#else
double aggKi = aggKp / aggTn;
#endif
double aggKd = aggTv * aggKp ;

PID bPID(&Input, &Output, &setPoint, aggKp, aggKi, aggKd, PonE, DIRECT);    //PID initialisation

/********************************************************
   DALLAS TEMP
******************************************************/
OneWire oneWire(ONE_WIRE_BUS);         // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire);   // Pass our oneWire reference to Dallas Temperature.
DeviceAddress sensorDeviceAddress;     // arrays to hold device address

/********************************************************
   Temp Sensors TSIC 306
******************************************************/
uint16_t temperature = 0;     // internal variable used to read temeprature
float Temperatur_C = 0;       // internal variable that holds the converted temperature in °C

#if (ONE_WIRE_BUS == 16  && TEMPSENSOR  == 2 && defined(ESP8266)) 
TSIC Sensor1(ONE_WIRE_BUS);   // only Signalpin, VCCpin unused by default
#else 
ZACwire<ONE_WIRE_BUS> Sensor2(306);    // set pin "2" to receive signal from the TSic "306"
#endif
/********************************************************
   BLYNK
******************************************************/
//Update Intervall zur App
unsigned long previousMillisBlynk;  // initialisation at the end of init()
const unsigned long intervalBlynk = 1000;
int blynksendcounter = 1;


/********************************************************
  Get Wifi signal strength and set bars for display
*****************************************************/
void getSignalStrength() {
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
   DISPLAY Define & template
******************************************************/
//DISPLAY constructor, change if needed
#if  DISPLAY == 1
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);   //e.g. 1.3"
#endif
#if DISPLAY == 2
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);    //e.g. 0.96"
#endif
//Update für Display
unsigned long previousMillisDisplay;  // initialisation at the end of init()
const unsigned long intervalDisplay = 500;

//Standard Display or vertikal?
#if (DISPLAY == 1 || DISPLAY == 2) // Display is used 
  #if (DISPLAYTEMPLATE < 20) // normal templates
    #include "display.h"  
  #endif  
  #if (DISPLAYTEMPLATE >= 20) // vertical templates 
    #include "Displayrotateupright.h"  
  #endif  
  #if (DISPLAYTEMPLATE == 1)
      #include "Displaytemplatestandard.h"
  #endif    
  #if (DISPLAYTEMPLATE == 2)
      #include "Displaytemplateminimal.h"
  #endif    
  #if (DISPLAYTEMPLATE == 3)
      #include "Displaytemplatetemponly.h"
  #endif   
  #if (DISPLAYTEMPLATE == 20)
      #include "Displaytemplateupright.h"
  #endif   
#endif



/********************************************************
   BLYNK define pins and read values
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
  BrewSetPoint = param.asDouble();
  mqtt_publish("BrewSetPoint", number2string(BrewSetPoint));
}

BLYNK_WRITE(V8) {
  brewtime = param.asDouble() * 1000;
  mqtt_publish("brewtime", number2string(brewtime/1000));
}

BLYNK_WRITE(V9) {
  preinfusion = param.asDouble() * 1000;
  mqtt_publish("preinfusion", number2string(preinfusion/1000));
}

BLYNK_WRITE(V10) {
  preinfusionpause = param.asDouble() * 1000;
  mqtt_publish("preinfusionpause", number2string(preinfusionpause/1000));
}
BLYNK_WRITE(V13)
{
  pidON = param.asInt();
  mqtt_publish("pidON", number2string(pidON));
}
BLYNK_WRITE(V15) 
{
  SteamON = param.asInt();
  if (SteamON == 1) 
  {
  SteamFirstON = 1;  
  }
  if (SteamON == 0) 
  {
  SteamFirstON = 0;  
  }
  mqtt_publish("SteamON", number2string(SteamON));
}
BLYNK_WRITE(V16) {
  SteamSetPoint = param.asDouble();
  mqtt_publish("SteamSetPoint", number2string(SteamSetPoint));
}
BLYNK_WRITE(V25)
{
  calibration_mode = param.asInt();//
}
BLYNK_WRITE(V26)
{
  water_empty = param.asInt();//
}
BLYNK_WRITE(V27)
{
  water_full = param.asInt();//
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
BLYNK_WRITE(V40) {
  backflushON =  param.asInt();
}

#if (COLDSTART_PID == 2)  // 2=?Blynk values, else default starttemp from config
  BLYNK_WRITE(V11) 
    {
    startKp = param.asDouble();
    }
  BLYNK_WRITE(V14)
    {
      startTn = param.asDouble();
    }
 #endif
/********************************************************
  Trigger for Rancilio E Machine
******************************************************/
unsigned long previousMillisETrigger ;  // initialisation at the end of init()
const unsigned long intervalETrigger = ETRIGGERTIME ; // in Seconds
int relayETriggerON, relayETriggerOFF;    
/********************************************************
  Emergency stop inf temp is to high
*****************************************************/
void testEmergencyStop() {
  if (Input > EmergencyStopTemp && emergencyStop == false) {
    emergencyStop = true;
  } else if (Input < 100 && emergencyStop == true) {
    emergencyStop = false;
  }
}


void backflush() {
  if (backflushState != 10 && backflushON == 0) {
    backflushState = 43;    // force reset in case backflushON is reset during backflush!
  } else if ( Offlinemodus == 1 || brewcounter > 10 || maxflushCycles <= 0 || backflushON == 0) {
    return;
  }

  if (pidMode == 1) { //Deactivate PID
    pidMode = 0;
    bPID.SetMode(pidMode);
    Output = 0 ;
  }
  digitalWrite(pinRelayHeater, LOW); //Stop heating

  readAnalogInput();
  unsigned long currentMillistemp = millis();

  if (brewswitch < 1000 && backflushState > 10) {   //abort function for state machine from every state
    backflushState = 43;
  }

  // state machine for brew
  switch (backflushState) {
    case 10:    // waiting step for brew switch turning on
      if (brewswitch > 1000 && backflushON) {
        startZeit = millis();
        backflushState = 20;
      }
      break;
    case 20:    //portafilter filling
      DEBUG_println("portafilter filling");
      digitalWrite(pinRelayVentil, relayON);
      digitalWrite(pinRelayPumpe, relayON);
      backflushState = 21;
      break;
    case 21:    //waiting time for portafilter filling
      if (millis() - startZeit > FILLTIME) {
        startZeit = millis();
        backflushState = 30;
      }
      break;
    case 30:    //flushing
      DEBUG_println("flushing");
      digitalWrite(pinRelayVentil, relayOFF);
      digitalWrite(pinRelayPumpe, relayOFF);
      flushCycles++;
      backflushState = 31;
      break;
    case 31:    //waiting time for flushing
      if (millis() - startZeit > flushTime && flushCycles < maxflushCycles) {
        startZeit = millis();
        backflushState = 20;
      } else if (flushCycles >= maxflushCycles) {
        backflushState = 43;
      }
      break;
    case 43:    // waiting for brewswitch off position
      if (brewswitch < 1000) {
        DEBUG_println("backflush finished");
        digitalWrite(pinRelayVentil, relayOFF);
        digitalWrite(pinRelayPumpe, relayOFF);
        currentMillistemp = 0;
        flushCycles = 0;
        backflushState = 10;
      }
      break;
  }
}



/********************************************************
  Read analog input pin
*****************************************************/
void readAnalogInput() {
  unsigned long currentMillistemp = millis();
  if (currentMillistemp - previousMillistempanalogreading >= analogreadingtimeinterval)
  {
    previousMillistempanalogreading = currentMillistemp;
    brewswitch = filter(analogRead(analogPin));
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
  } else {
    readIndex++;
  }
}


/********************************************************
  check sensor value.
  If < 0 or difference between old and new >25, then increase error.
  If error is equal to maxErrorCounter, then set sensorError
*****************************************************/
boolean checkSensor(float tempInput) {
  boolean sensorOK = false;
  boolean badCondition = ( tempInput < 0 || tempInput > 150 || fabs(tempInput - previousInput) > 5);
  if ( badCondition && !sensorError) {
    error++;
    sensorOK = false;
    DEBUG_print("WARN: temperature sensor reading: consec_errors = ");    DEBUG_print(error);    DEBUG_print(", temp_current = ");    DEBUG_println(tempInput);
  } else if (badCondition == false && sensorOK == false) {
    error = 0;
    sensorOK = true;
  }
  if (error >= maxErrorCounter && !sensorError) {
    sensorError = true ;
    DEBUG_print("ERROR: temperature sensor malfunction: emp_current = ");    DEBUG_println(tempInput);
  } else if (error == 0 && sensorError) {
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
  unsigned long currentMillistemp = millis();
  previousInput = Input ;
  if (TempSensor == 1)
  {
    if (currentMillistemp - previousMillistemp >= intervaltempmesds18b20)
    {
      previousMillistemp = currentMillistemp;
      sensors.requestTemperatures();
      if (!checkSensor(sensors.getTempCByIndex(0)) && firstreading == 0) return;  //if sensor data is not valid, abort function; Sensor must be read at least one time at system startup
      Input = sensors.getTempCByIndex(0);
      if (Brewdetection != 0) {
        movAvg();
      } else if (firstreading != 0) {
        firstreading = 0;
      }
    }
  }
  if (TempSensor == 2)
  {
    if (currentMillistemp - previousMillistemp >= intervaltempmestsic)
    {
      previousMillistemp = currentMillistemp;
      /*  variable "temperature" must be set to zero, before reading new data
            getTemperature only updates if data is valid, otherwise "temperature" will still hold old values
      */
      temperature = 0;
       #if (ONE_WIRE_BUS == 16 && defined(ESP8266))
         Sensor1.getTemperature(&temperature);
         Temperatur_C = Sensor1.calc_Celsius(&temperature);
         #endif
       #if (ONE_WIRE_BUS != 16)
        Temperatur_C = Sensor2.getTemp();
       #endif
      //Temperatur_C = 70;
      if (!checkSensor(Temperatur_C) && firstreading == 0) return;  //if sensor data is not valid, abort function; Sensor must be read at least one time at system startup
      Input = Temperatur_C;
      if (Brewdetection != 0) {
        movAvg();
      } else if (firstreading != 0) {
        firstreading = 0;
      }
    }
  }
}

/********************************************************
    PreInfusion, Brew , if not Only PID
******************************************************/
void brew() 
{
  if (OnlyPID == 0) 
  {
    readAnalogInput();
    unsigned long currentMillistemp = millis();

    if (brewswitch < 1000 && brewcounter > 10)
    {
      //abort function for state machine from every state
      brewcounter = 43;
    }

    if (brewcounter > 10 && brewcounter < 43 ) {
      bezugsZeit = currentMillistemp - startZeit;
    }
    if (brewswitch < 1000 && firstreading == 0 ) 
    {   //check if brewswitch was turned off at least once, last time,
      brewswitchWasOFF = true;
      //DEBUG_println("brewswitch value")
      //DEBUG_println(brewswitch)
    }

    totalbrewtime = preinfusion + preinfusionpause + brewtime;    // running every cycle, in case changes are done during brew

    // state machine for brew
    switch (brewcounter) {
      case 10:    // waiting step for brew switch turning on
        if (brewswitch > 1000 && backflushState == 10 && backflushON == 0 && brewswitchWasOFF) {
          startZeit = millis();
          brewcounter = 20;
          lastbezugszeit = 0;
          kaltstart = false;    // force reset kaltstart if shot is pulled
        } else {
          backflush();
        }
        break;
      case 20:    //preinfusioon
        DEBUG_println("Preinfusion");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayON);
        brewcounter = 21;
        break;
      case 21:    //waiting time preinfusion
        if (bezugsZeit > preinfusion) {
          brewcounter = 30;
        }
        break;
      case 30:    //preinfusion pause
        DEBUG_println("preinfusion pause");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayOFF);
        brewcounter = 31;
        break;
      case 31:    //waiting time preinfusion pause
        if (bezugsZeit > preinfusion + preinfusionpause) {
          brewcounter = 40;
        }
        break;
      case 40:    //brew running
        DEBUG_println("Brew started");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayON);
        brewcounter = 41;
        break;
      case 41:    //waiting time brew
        lastbezugszeit = bezugsZeit;
        if (bezugsZeit > totalbrewtime) {
          brewcounter = 42;
        }
        break;
      case 42:    //brew finished
        DEBUG_println("Brew stopped");
        digitalWrite(pinRelayVentil, relayOFF);
        digitalWrite(pinRelayPumpe, relayOFF);
        brewcounter = 43;
        bezugsZeit = 0;
        break;
      case 43:    // waiting for brewswitch off position
        if (brewswitch < 1000) {
          digitalWrite(pinRelayVentil, relayOFF);
          digitalWrite(pinRelayPumpe, relayOFF);
          // lastbezugszeitMillis = millis();  // for shottimer delay after disarmed button
          currentMillistemp = 0;
          brewDetected = 0; //rearm brewdetection
          brewcounter = 10;
          bezugsZeit = 0;
        }
        break;
    }
  }
}

/*******************************************************
  Switch to offline modeif maxWifiReconnects were exceeded
  during boot
*****************************************************/
void initOfflineMode() 
{
  #if DISPLAY != 0
    displayMessage("", "", "", "", "Begin Fallback,", "No Wifi");
  #endif
  DEBUG_println("Start offline mode with eeprom values, no wifi:(");
  Offlinemodus = 1 ;

  EEPROM.begin(1024);  // open eeprom
  double dummy; // check if eeprom values are numeric (only check first value in eeprom)
  EEPROM.get(0, dummy);
  DEBUG_print("check eeprom 0x00 in dummy: ");
  DEBUG_println(dummy);
  if (!isnan(dummy)) {
    EEPROM.get(0, aggKp);
    EEPROM.get(10, aggTn);
    EEPROM.get(20, aggTv);
    EEPROM.get(30, BrewSetPoint);
    EEPROM.get(40, brewtime);
    EEPROM.get(50, preinfusion);
    EEPROM.get(60, preinfusionpause);
    EEPROM.get(90, aggbKp);
    EEPROM.get(100, aggbTn);
    EEPROM.get(110, aggbTv);
    EEPROM.get(120, brewtimersoftware);
    EEPROM.get(130, brewboarder);
  } else {
    #if DISPLAY != 0
      displayMessage("", "", "", "", "No eeprom,", "Values");
     #endif
    DEBUG_println("No working eeprom value, I am sorry, but use default offline value  :)");
    delay(1000);
  }
  // eeeprom schließen
  EEPROM.commit();
}

/*******************************************************
   Check if Wifi is connected, if not reconnect
   abort function if offline, or brew is running
*****************************************************/
void checkWifi() {
  if (Offlinemodus == 1 || brewcounter > 11) return;
  do {
    if ((millis() - lastWifiConnectionAttempt >= wifiConnectionDelay) && (wifiReconnects <= maxWifiReconnects)) {
      int statusTemp = WiFi.status();
      if (statusTemp != WL_CONNECTED) {   // check WiFi connection status
        lastWifiConnectionAttempt = millis();
        wifiReconnects++;
        DEBUG_print("Attempting WIFI reconnection: ");
        DEBUG_println(wifiReconnects);
        if (!setupDone) {
           #if DISPLAY != 0
            displayMessage("", "", "", "", "Wifi reconnect:", String(wifiReconnects));
          #endif
        }
        WiFi.disconnect();
        WiFi.begin(ssid, pass);   // attempt to connect to Wifi network
        int count = 1;
        while (WiFi.status() != WL_CONNECTED && count <= 20) {
          delay(100);   //give WIFI some time to connect
          count++;      //reconnect counter, maximum waiting time for reconnect = 20*100ms
        }
      }
    }
    yield();  //Prevent WDT trigger
  } while ( !setupDone && wifiReconnects < maxWifiReconnects && WiFi.status() != WL_CONNECTED);   //if kaltstart ist still true when checkWifi() is called, then there was no WIFI connection at boot -> connect or offlinemode

  if (wifiReconnects >= maxWifiReconnects && !setupDone) {   // no wifi connection after boot, initiate offline mode (only directly after boot)
    initOfflineMode();
  }

}

/*******************************************************
   Check if Blynk is connected, if not reconnect
   abort function if offline, or brew is running
   blynk is also using maxWifiReconnects!
*****************************************************/
void checkBlynk() {
  if (Offlinemodus == 1 || brewcounter > 11) return;
  if ((millis() - lastBlynkConnectionAttempt >= wifiConnectionDelay) && (blynkReCnctCount <= maxWifiReconnects)) {
    int statusTemp = Blynk.connected();
    if (statusTemp != 1) {   // check Blynk connection status
      lastBlynkConnectionAttempt = millis();        // Reconnection Timer Function
      blynkReCnctCount++;  // Increment reconnection Counter
      DEBUG_print("Attempting blynk reconnection: ");
      DEBUG_println(blynkReCnctCount);
      Blynk.connect(3000);  // Try to reconnect to the server; connect() is a blocking function, watch the timeout!
    }
  }
}



/*******************************************************
   Check if MQTT is connected, if not reconnect
   abort function if offline, or brew is running
   MQTT is also using maxWifiReconnects!
*****************************************************/
void checkMQTT(){
  if (Offlinemodus == 1 || brewcounter > 11) return;
  if ((millis() - lastMQTTConnectionAttempt >= wifiConnectionDelay) && (MQTTReCnctCount <= maxWifiReconnects)) {
    int statusTemp = mqtt.connected();
    if (statusTemp != 1) {   // check Blynk connection status
      lastMQTTConnectionAttempt = millis();        // Reconnection Timer Function
      MQTTReCnctCount++;  // Increment reconnection Counter
      DEBUG_print("Attempting MQTT reconnection: ");
      DEBUG_println(MQTTReCnctCount);
      if (mqtt.connect(hostname, mqtt_username, mqtt_password,topic_will,0,0,"exit") == true);{
        mqtt.subscribe(topic_set);
        DEBUG_println("Subscribe to MQTT Topics");
      }  // Try to reconnect to the server; connect() is a blocking function, watch the timeout!
    }
  }
}

/*******************************************************
   Convert double, float int and uint to char
   for MQTT Publish
*****************************************************/
char number2string_double[22];
char* number2string(double in) {
  snprintf(number2string_double, sizeof(number2string_double), "%0.2f", in);
  return number2string_double;
}
char number2string_float[22];
char* number2string(float in) {
  snprintf(number2string_float, sizeof(number2string_float), "%0.2f", in);
  return number2string_float;
}
char number2string_int[22];
char* number2string(int in) {
  snprintf(number2string_int, sizeof(number2string_int), "%d", in);
  return number2string_int;
}
char number2string_uint[22];
char* number2string(unsigned int in) {
  snprintf(number2string_uint, sizeof(number2string_uint), "%u", in);
  return number2string_uint;
}

/*******************************************************
   Publish Data to MQTT
*****************************************************/
bool mqtt_publish(char* reading, char* payload) {
  if (MQTT == 1){
    char topic[120];
    snprintf(topic, 120, "%s%s/%s", mqtt_topic_prefix, hostname, reading);
    mqtt.publish(topic,payload,true);
  }
  }

/********************************************************
  send data to Blynk server
*****************************************************/

void sendToBlynk() {
  if (Offlinemodus == 1) return;

  unsigned long currentMillisBlynk = millis();
  unsigned long currentMillistemp = 0;

  if (currentMillisBlynk - previousMillisBlynk >= intervalBlynk) {

    //MQTT
    if (MQTT == 1) {
      checkMQTT();
    }

    previousMillisBlynk = currentMillisBlynk;
    if (Blynk.connected()) {
      if (blynksendcounter == 1) {
        Blynk.virtualWrite(V2, Input);
        mqtt_publish("temperature", number2string(Input));
      }
      if (blynksendcounter == 2) {
        Blynk.virtualWrite(V23, Output);
      }
      if (blynksendcounter == 3) {
        Blynk.virtualWrite(V17, setPoint);
        //MQTT
        mqtt_publish("setPoint", number2string(setPoint));
      }
      if (blynksendcounter == 4) {
        Blynk.virtualWrite(V35, heatrateaverage);
      }
      if (blynksendcounter == 5) {
        Blynk.virtualWrite(V36, heatrateaveragemin);
      }
      if (grafana == 1 && blynksendcounter >= 6) {
        Blynk.virtualWrite(V60, Input, Output, bPID.GetKp(), bPID.GetKi(), bPID.GetKd(), setPoint );
         if (MQTT == 1)
         {
            mqtt_publish("HeaterPower", number2string(Output));
            mqtt_publish("Kp", number2string(bPID.GetKp()));
            mqtt_publish("Ki", number2string(bPID.GetKi()));
            mqtt_publish("pidON", number2string(pidON));
            mqtt_publish("brewtime", number2string(brewtime/1000));
            mqtt_publish("preinfusionpause", number2string(preinfusionpause/1000));
            mqtt_publish("preinfusion", number2string(preinfusion/1000));
            mqtt_publish("SteamON", number2string(SteamON));
         }
        blynksendcounter = 0;
      } else if (grafana == 0 && blynksendcounter >= 5) {
        blynksendcounter = 0;
      }
      blynksendcounter++;
    }
  }
}

/********************************************************
    Brewdetection
******************************************************/
void brewdetection() 
{
  if (brewboarder == 0) return; //abort brewdetection if deactivated

  // Brew detecion == 1 software solution , == 2 hardware == 3 Voltagesensor 
  if (Brewdetection == 1) 
  {  // Bezugstimmer für SW aktivieren
     if (timerBrewdetection == 1)
    {
     bezugsZeit = millis() - timeBrewdetection ;
     }
    // Bezugstimmer für SW deaktivieren nach ende BD PID
    if (millis() - timeBrewdetection > brewtimersoftware * 1000 && timerBrewdetection == 1 )
    {
      timerBrewdetection = 0 ;    //rearm brewdetection
     // lastbezugszeitMillis = millis(); // Bezugszeit für Delay 
      bezugsZeit = 0 ;
     }
  } else if (Brewdetection == 2) 
  {
    if (millis() - timeBrewdetection > brewtimersoftware * 1000 && timerBrewdetection == 1 ) 
    {
      timerBrewdetection = 0 ;  //rearm brewdetection
    }
  } else if (Brewdetection == 3) 
  {
    // Bezugszeit hochzaehlen    
    if (( digitalRead(PINVOLTAGESENSOR) == VoltageSensorON) && brewDetected == 1)
       {
       bezugsZeit = millis() - startZeit ;
       lastbezugszeit = bezugsZeit ;
       }
    //  OFF: Bezug zurücksetzen
    if 
     ((digitalRead(PINVOLTAGESENSOR) == VoltageSensorOFF) && brewDetected == 1)
      {
        brewDetected = 0;
        bezugsZeit = 0 ; 
        startZeit = 0;
        DEBUG_println("HW Brew - Voltage Sensor - End") ;
     //   lastbezugszeitMillis = millis(); // Bezugszeit für Delay 
      }
    if (millis() - timeBrewdetection > brewtimersoftware * 1000 && timerBrewdetection == 1) // reset PID Brew
    {
      timerBrewdetection = 0 ;    //rearm brewdetection
    }
  }


  if (Brewdetection == 1) 
  {
    if (heatrateaverage <= -brewboarder && timerBrewdetection == 0 && (fabs(Input - BrewSetPoint) < 5) ) // BD PID only +/- 2 Grad Celsius
    {
      DEBUG_println("SW Brew detected") ;
      timeBrewdetection = millis() ;
      timerBrewdetection = 1 ;
    }
  } else if (Brewdetection == 2) 
  {
    if (brewcounter > 10 && brewDetected == 0 && brewboarder != 0) 
    {
      DEBUG_println("HW Brew detected") ;
      timeBrewdetection = millis() ;
      timerBrewdetection = 1 ;
      brewDetected = 1;
    }  
  } else if (Brewdetection == 3) // voltage sensor 
  {

    unsigned long currentMillisVoltagesensorreading = millis();   
   // if (
   //     (currentMillisVoltagesensorreading - previousMillisVoltagesensorreading >= (intervalVoltagesensor)) //Abfrageinterval
   //     && brewDetected == 0 // nur einmalig auslösen
   //    )
   // {
      previousMillisVoltagesensorreading = millis();
      if (digitalRead(PINVOLTAGESENSOR) == VoltageSensorON && brewDetected == 0 ) 
      {
        DEBUG_println("HW Brew - Voltage Sensor -  Start") ;
        timeBrewdetection = millis() ;
        startZeit = millis() ;
        timerBrewdetection = 1 ;
        brewDetected = 1;
        lastbezugszeit = 0 ;
      }
    //}
  }
}

/********************************************************
  after ~28 cycles the input is set to 99,66% if the real input value
  sum of inX and inY multiplier must be 1
  increase inX multiplier to make the filter faster
*****************************************************/
int filter(int input) {
  inX = input * 0.3;
  inY = inOld * 0.7;
  inSum = inX + inY;
  inOld = inSum;

  return inSum;
}


/********************************************************
    Timer 1 - ISR for PID calculation and heat realay output
******************************************************/

 #include "ISR.h"  

/********************************************************
    MQTT Callback Function: set Parameters through MQTT
******************************************************/


void mqtt_callback(char* topic, byte* data, unsigned int length) {
  char topic_str[255];
  os_memcpy(topic_str, topic, sizeof(topic_str));
  topic_str[255] = '\0';
  char data_str[length+1];
  os_memcpy(data_str, data, length);
  data_str[length] = '\0';
  char topic_pattern[255];
  char configVar[120];
  char cmd[64];
  double data_double;
  int data_int;



 // DEBUG_print("mqtt_parse(%s, %s)\n", topic_str, data_str);
  snprintf(topic_pattern, sizeof(topic_pattern), "%s%s/%%[^\\/]/%%[^\\/]", mqtt_topic_prefix, hostname);
  DEBUG_println(topic_pattern);
  if ( (sscanf( topic_str, topic_pattern , &configVar, &cmd) != 2) || (strcmp(cmd, "set") != 0) ) {
  DEBUG_print(topic_str);
    return;
  }
  DEBUG_println(topic_str);
  DEBUG_println(data_str);
  if (strcmp(configVar, "BrewSetPoint") == 0) {
    sscanf(data_str, "%lf", &data_double);
    mqtt_publish("BrewSetPoint", number2string(BrewSetPoint));
    if (Blynk.connected()) { Blynk.virtualWrite(V7, String(data_double));}
    BrewSetPoint = data_double;
    return;
  }
  if (strcmp(configVar, "brewtime") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (Blynk.connected()) { Blynk.virtualWrite(V8, String(data_double));}
    mqtt_publish("brewtime", number2string(brewtime/1000));
    brewtime = data_double * 1000 ;
    return;
  }
  if (strcmp(configVar, "preinfusion") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (Blynk.connected()) { Blynk.virtualWrite(V9, String(data_double));}
    mqtt_publish("preinfusion", number2string(preinfusion/1000));
    preinfusion = data_double * 1000;
    return;
  }
  if (strcmp(configVar, "preinfusionpause") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (Blynk.connected()) { Blynk.virtualWrite(V10, String(data_double));}
    mqtt_publish("preinfusionpause", number2string(preinfusionpause/1000));
    preinfusionpause = data_double * 1000;
    return;
  }
    if (strcmp(configVar, "pidON") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (Blynk.connected())  { Blynk.virtualWrite(V13,String(data_double));}
    mqtt_publish("pidON", number2string(pidON));
    pidON = data_double ;
    return;
  }

}
/*******************************************************
  Trigger for E-Silvia
*****************************************************/
//unsigned long previousMillisETrigger ;  // initialisation at the end of init()
//const unsigned long intervalETrigger = ETriggerTime ; // in Seconds
void ETriggervoid() 
{
  //Static variable only one time is 0 
  static int ETriggeractive = 0;
  unsigned long currentMillisETrigger = millis();
  if (ETRIGGER == 1) // E Trigger is active from userconfig
  { 
    // 
    if (currentMillisETrigger - previousMillisETrigger >= (1000*intervalETrigger))  //s to ms * 1000
    {  // check 
      ETriggeractive = 1 ;
      previousMillisETrigger = currentMillisETrigger;

      digitalWrite(PINETRIGGER, relayETriggerON);
    }
    // 10 Seconds later
    else if (ETriggeractive == 1 && previousMillisETrigger+(10*1000) < (currentMillisETrigger))
    {
    digitalWrite(PINETRIGGER, relayETriggerOFF);
    ETriggeractive = 0;
    }
  } 
}
  /********************************************************
   SteamON
  ******************************************************/
void checkSteamON() 
{
// check digital GIPO  
  if (digitalRead(STEAMONPIN) == HIGH) 
  {
    SteamON = 1;
    
  } 
  if (digitalRead(STEAMONPIN) == LOW && SteamFirstON == 0) // if via blynk on, then SteamFirstON == 1, prevent override
  {
    SteamON = 0;
    
  }
  if (SteamON == 1) 
  {
    EmergencyStopTemp = 145;  
    setPoint = SteamSetPoint ;
  }
   if (SteamON == 0) 
  {
    EmergencyStopTemp = 120;  
    setPoint = BrewSetPoint ;
  }
}

void machinestatevoid() 
{
  /* 
  00 = init
  10 = kaltstart
  19 = Setpoint -1 Celsius
  20 = Setpoint überschritten, idel at setpoint
  30 = Bezug 
  35 = Nachlauf BD
  40 = Dampf
  80 = Emergency Stop
  90 = PID Offline
  100 = Sensorerror
  */
  //DEBUG_println(machinestate);
  switch (machinestate) 
  {
    // init
    case 0: 
      if (Input < (BrewSetPoint-1) )
      {
        machinestate = 10 ; // kaltstart
      }
      if (Input >= (BrewSetPoint-1) )
      {
        machinestate = 19 ; // machine is hot, jump to other state
      }
      
      if (emergencyStop)
      {
        machinestate = 80 ; // Emergency Stop
      }
     if (pidON == 0)
      {
        machinestate = 90 ; // offline
      }
     if(sensorError)
      {
        machinestate = 100 ;// sensorerror
      }
    break;

     // kaltstart
    case 10: 
      if (Input >= (BrewSetPoint-1) )
      {
        machinestate = 19 ;
      }
      if (SteamON == 1)
      {
        machinestate = 40 ; // Steam
      }

      if
      (
       (bezugsZeit > 0 && ONLYPID == 1) || // Bezugszeit bei Only PID  
       (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42) 
      )
      
      {
        machinestate = 30 ; // Brew
      }

      if (SteamON == 1)
      {
        machinestate = 40 ; // switch to  Steam
      }
      if (emergencyStop)
      {
        machinestate = 80 ; // Emergency Stop
      }
     if (pidON == 0)
      {
        machinestate = 90 ; // offline
      }
     if(sensorError)
      {
        machinestate = 100 ;// sensorerror
      }
      // Setpoint -1 Celsius
      case 19: 
      if (Input >= (BrewSetPoint))
      {
        machinestate = 20 ;
      }
      if
      (
       (bezugsZeit > 0 && ONLYPID == 1) || // Bezugszeit bei Only PID  
       (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42) 
      )
      {
        machinestate = 30 ; // Brew
      }

      
      if (SteamON == 1)
      {
        machinestate = 40 ; // Steam
      }

      if (SteamON == 1)
      {
        machinestate = 40 ; // switch to  Steam
      }
      if (emergencyStop)
      {
        machinestate = 80 ; // Emergency Stop
      }
     if (pidON == 0)
      {
        machinestate = 90 ; // offline
      }
     if(sensorError)
      {
        machinestate = 100 ;// sensorerror
      }  
    break;
    // normal PID
    case 20: 
      if
      (
       (bezugsZeit > 0 && ONLYPID == 1) || // Bezugszeit bei Only PID  
       (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42) 
      )
      {
        machinestate = 30 ; // Brew
      }
      if (SteamON == 1)
      {
        machinestate = 40 ; // Steam
      }

      if (emergencyStop)
      {
        machinestate = 80 ; // Emergency Stop
      }
     if (pidON == 0)
      {
        machinestate = 90 ; // offline
      }
     if(sensorError)
      {
        machinestate = 100 ;// sensorerror
      }
    break;
     // Brew
    case 30:
      if
      (
       (bezugsZeit > 35*1000 && Brewdetection == 1 && ONLYPID == 1  ) ||  // 35 sec later and BD PID aktive SW Solution
       (bezugsZeit == 0      && Brewdetection == 3 && ONLYPID == 1  ) ||  // Voltagesensor reset bezugsZeit == 0
       ((brewcounter == 10 || brewcounter == 43)   && ONLYPID == 0  ) // switchoff BD PID aktive
      )
      {
       if ((ONLYPID == 1 && Brewdetection == 3) || ONLYPID == 0 ) // only delay of shotimer for voltagesensor or brewcounter
       {
         machinestate = 31 ;
         lastbezugszeitMillis = millis() ; // for delay
        
       }
       if (ONLYPID == 1 && Brewdetection == 1 && timerBrewdetection == 1) //direct to PID BD
       {
         machinestate = 35 ;
       }
      } 
      if (SteamON == 1)
      {
        machinestate = 40 ; // Steam
      }

      if (emergencyStop)
      {
        machinestate = 80 ; // Emergency Stop
      }
     if (pidON == 0)
      {
        machinestate = 90 ; // offline
      }
     if(sensorError)
      {
        machinestate = 100 ;// sensorerror
      }
    break;
    // Sec after shot finish
    case 31: //lastbezugszeitMillis
      if ( millis()-lastbezugszeitMillis > BREWSWITCHDELAY )
      {
       machinestate = 35 ;
       lastbezugszeit = 0 ;
      }
      if (SteamON == 1)
      {
        machinestate = 40 ; // Steam
      }

      if (emergencyStop)
      {
        machinestate = 80 ; // Emergency Stop
      }
     if (pidON == 0)
      {
        machinestate = 90 ; // offline
      }
     if(sensorError)
      {
        machinestate = 100 ;// sensorerror
      }
    break;
    // BD PID
    case 35:
      if (timerBrewdetection == 0)
      {
        machinestate = 20 ; // switch to normal PID
      }
      if
      (
       (bezugsZeit > 0 && ONLYPID == 1  && Brewdetection == 3) || // New Brew inner BD only by Only PID AND Voltage Sensor
       (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42) 
      )
      {
        machinestate = 30 ; // Brew
      }
      
      if (SteamON == 1)
      {
        machinestate = 40 ; // switch to  Steam
      }

      if (emergencyStop)
      {
        machinestate = 80 ; // Emergency Stop
      }
      if (pidON == 0)
      {
        machinestate = 90 ; // offline
      }
     if(sensorError)
      {
        machinestate = 100 ;// sensorerror
      }
    break;
    // Steam
    case 40:
      if (SteamON == 0)
      {
        machinestate = 20 ; //  switch to normal
      }

     if (emergencyStop)
      {
        machinestate = 80 ; // Emergency Stop
      }
     if (pidON == 0)
      {
        machinestate = 90 ; // offline
      }
     if(sensorError)
      {
        machinestate = 100 ;// sensorerror
      }
    break;
    // emergencyStop 
    case 80: 
      if (!emergencyStop)
      {
        machinestate = 20 ; // normal PID
      }
      if (pidON == 0)
      {
        machinestate = 90 ; // offline
      }
      if(sensorError)
      {
        machinestate = 100 ;
      }
    break;
    // PID offline
    case 90: 
      if (pidON == 1)
      {
        if(kaltstart) 
        {
        machinestate = 10 ; // kaltstart 
        }
        if(!kaltstart) 
        {
        machinestate = 20 ; // normal PID
        }
      }
      
      if(sensorError)
      {
        machinestate = 100 ;
      }
    break;
    // sensor error
    case 100:
    // Nothing
    break;
  } // switch case
} // end void

void setup() {
  DEBUGSTART(115200);

  if (MQTT == 1) {
    //MQTT
    snprintf(topic_will, sizeof(topic_will), "%s%s/%s", mqtt_topic_prefix, hostname, "will");
    snprintf(topic_set, sizeof(topic_set), "%s%s/+/%s", mqtt_topic_prefix, hostname, "set");
    mqtt.setServer(mqtt_server_ip, mqtt_server_port);
    mqtt.setCallback(mqtt_callback);
    checkMQTT();
  }

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

  if (TRIGGERRELAYTYPE)
  {
    relayETriggerON = HIGH;
    relayETriggerOFF  = LOW;
  } else {
    relayETriggerON  = LOW;
    relayETriggerOFF  = HIGH;
  }
  if (VOLTAGESENSORTYPE)
  {
    VoltageSensorON = HIGH;
    VoltageSensorOFF  = LOW;
  } else {
    VoltageSensorON = LOW;
    VoltageSensorOFF  = HIGH;
  }

  /********************************************************
    Init Pins
  ******************************************************/
  pinMode(pinRelayVentil, OUTPUT);
  pinMode(pinRelayPumpe, OUTPUT);
  pinMode(pinRelayHeater, OUTPUT);
  pinMode(STEAMONPIN, INPUT);
  digitalWrite(pinRelayVentil, relayOFF);
  digitalWrite(pinRelayPumpe, relayOFF);
  digitalWrite(pinRelayHeater, LOW);
  if (ETRIGGER == 1) // IF Etrigger selected 
  { 
    pinMode(PINETRIGGER, OUTPUT);
  }
  if (BREWDETECTION == 3) // IF Voltage sensor selected 
  { 
    pinMode(PINVOLTAGESENSOR, PINMODEVOLTAGESENSOR);
  }

  /********************************************************
    DISPLAY 128x64
  ******************************************************/
  #if DISPLAY != 0
    u8g2.setI2CAddress(oled_i2c * 2);
    u8g2.begin();
    u8g2_prepare();
    displayLogo(sysVersion, "");
    delay(2000);
  #endif

  /********************************************************
    VL530L0x TOF sensor
  ******************************************************/
  if (TOF != 0) { 
  lox.begin(tof_i2c); // initialize TOF sensor at I2C address
  lox.setMeasurementTimingBudgetMicroSeconds(2000000);
  }

  /********************************************************
     BLYNK & Fallback offline
  ******************************************************/
  if (Offlinemodus == 0) 
  {
    #if defined(ESP8266)
      WiFi.hostname(hostname);
    #endif
    unsigned long started = millis();
    #if DISPLAY != 0
      displayLogo("1: Connect Wifi to:", ssid);
    #endif
    /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
      would try to act as both a client and an access-point and could cause
      network-issues with your other WiFi-devices on your WiFi-network. */
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);   //needed, otherwise exceptions are triggered \o.O/
    WiFi.begin(ssid, pass);
    #if defined(ESP32) // ESP32
     WiFi.setHostname(hostname); // for ESP32port
    #endif
    DEBUG_print("Connecting to ");
    DEBUG_print(ssid);
    DEBUG_println(" ...");

    // wait up to 20 seconds for connection:
    while ((WiFi.status() != WL_CONNECTED) && (millis() - started < 20000))
    {
      yield();    //Prevent Watchdog trigger
    }

    checkWifi();    //try to reconnect

    if (WiFi.status() == WL_CONNECTED)
    {
      DEBUG_println("WiFi connected");
      DEBUG_println("IP address: ");
      DEBUG_println(WiFi.localIP());
      DEBUG_println("Wifi works, now try Blynk (timeout 30s)");
      if (fallback == 0) {
        #if DISPLAY != 0
          displayLogo("Connect to Blynk", "no Fallback");
        #endif
      } else if (fallback == 1) {
        #if DISPLAY != 0
          displayLogo("2: Wifi connected, ", "try Blynk   ");
        #endif
      }
      delay(1000);

      //try blynk connection
      Blynk.config(auth, blynkaddress, blynkport) ;
      Blynk.connect(30000);

      if (Blynk.connected() == true) 
      {
        #if DISPLAY != 0
          displayLogo("3: Blynk connected", "sync all variables...");
        #endif
        DEBUG_println("Blynk is online");
        if (fallback == 1) 
        {
          DEBUG_println("sync all variables and write new values to eeprom");
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
          Blynk.syncVirtual(V15);
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
          EEPROM.put(30, BrewSetPoint);
          EEPROM.put(40, brewtime);
          EEPROM.put(50, preinfusion);
          EEPROM.put(60, preinfusionpause);
          EEPROM.put(90, aggbKp);
          EEPROM.put(100, aggbTn);
          EEPROM.put(110, aggbTv);
          EEPROM.put(120, brewtimersoftware);
          EEPROM.put(130, brewboarder);
          // eeprom schließen
          EEPROM.commit();
        }
      } else 
      {
        DEBUG_println("No connection to Blynk");
        EEPROM.begin(1024);  // open eeprom
        double dummy; // check if eeprom values are numeric (only check first value in eeprom)
        EEPROM.get(0, dummy);
        DEBUG_print("check eeprom 0x00 in dummy: ");
        DEBUG_println(dummy);
        if (!isnan(dummy)) 
        {
          #if DISPLAY != 0
           displayLogo("3: Blynk not connected", "use eeprom values..");
          #endif 
          EEPROM.get(0, aggKp);
          EEPROM.get(10, aggTn);
          EEPROM.get(20, aggTv);
          EEPROM.get(30, BrewSetPoint);
          EEPROM.get(40, brewtime);
          EEPROM.get(50, preinfusion);
          EEPROM.get(60, preinfusionpause);
          EEPROM.get(90, aggbKp);
          EEPROM.get(100, aggbTn);
          EEPROM.get(110, aggbTv);
          EEPROM.get(120, brewtimersoftware);
          EEPROM.get(130, brewboarder);
        } 
      }
    }
    else 
    { 
      #if DISPLAY != 0
        displayLogo("No ", "WIFI");
      #endif
      DEBUG_println("No WIFI");
      WiFi.disconnect(true);
      delay(1000);
    }
  }

  /********************************************************
     OTA
  ******************************************************/
  if (ota && Offlinemodus == 0 && WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.setHostname(OTAhost);  //  Device name for OTA
    ArduinoOTA.setPassword(OTApass);  //  Password for OTA
    ArduinoOTA.begin();
  }


  /********************************************************
     Ini PID
  ******************************************************/

  //setPointTemp = BrewSetPoint;
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
    Input = sensors.getTempCByIndex(0);
  }

  if (TempSensor == 2) {
    temperature = 0;
    #if (ONE_WIRE_BUS == 16)
         Sensor1.getTemperature(&temperature);
         Input = Sensor1.calc_Celsius(&temperature);
    #endif
    #if (ONE_WIRE_BUS != 16)
        Input = Sensor2.getTemp();
     #endif
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
  if (TempSensor == 2) {
    temperature = 0;
    #if (ONE_WIRE_BUS == 16)
         Sensor1.getTemperature(&temperature);
         Input = Sensor1.calc_Celsius(&temperature);
    #endif
    #if (ONE_WIRE_BUS != 16)
        Input = Sensor2.getTemp();
     #endif
  }

  //Initialisation MUST be at the very end of the init(), otherwise the time comparision in loop() will have a big offset
  unsigned long currentTime = millis();
  previousMillistemp = currentTime;
  windowStartTime = currentTime;
  previousMillisDisplay = currentTime;
  previousMillisBlynk = currentTime;
  previousMillisETrigger = currentTime; 
  previousMillisVoltagesensorreading = currentTime;
  setupDone = true;

  #if defined(ESP8266) 
    /********************************************************
      Timer1 ISR - Initialisierung
      TIM_DIV1 = 0,   //80MHz (80 ticks/us - 104857.588 us max)
      TIM_DIV16 = 1,  //5MHz (5 ticks/us - 1677721.4 us max)
      TIM_DIV256 = 3  //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
    ******************************************************/
      timer1_isr_init();
      timer1_attachInterrupt(onTimer1ISR);
      //timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
      //timer1_write(50000); // set interrupt time to 10ms
      timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
      timer1_write(6250); // set interrupt time to 20ms
  #endif
  #if defined(ESP32) // ESP32
        /********************************************************
    Timer1 ISR - Initialisierung
    TIM_DIV1 = 0,   //80MHz (80 ticks/us - 104857.588 us max)
    TIM_DIV16 = 1,  //5MHz (5 ticks/us - 1677721.4 us max)
    TIM_DIV256 = 3  //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
  ******************************************************/
    timer = timerBegin(0, 80, true); //m
    timerAttachInterrupt(timer, &onTimer, true);//m
    timerAlarmWrite(timer, 10000, true);//m
    timerAlarmEnable(timer);//m
  #endif
  
}
void loop() {
  if (calibration_mode == 1 && TOF == 1) {
      loopcalibrate();
  } else {
      looppid();
  }
}

// TOF Calibration_mode 
void loopcalibrate() {
//Deactivate PID
    if (pidMode == 1) 
    {
      pidMode = 0;
      bPID.SetMode(pidMode);
      Output = 0 ;false;
    }
if (Blynk.connected()) {  // If connected run as normal
      Blynk.run();
      blynkReCnctCount = 0; //reset blynk reconnects if connected
    } else  {
      checkBlynk();
    }
    digitalWrite(pinRelayHeater, LOW); //Stop heating to be on the safe side ...

  VL53L0X_RangingMeasurementData_t measure;  //TOF Sensor measurement
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!
  distance = measure.RangeMilliMeter;  //write new distence value to 'distance'
   #if DISPLAY !=0
    displayDistance(distance);
  #endif
}

void looppid() {
  //Only do Wifi stuff, if Wifi is connected
  if (WiFi.status() == WL_CONNECTED && Offlinemodus == 0) { 

    //MQTT
    if (MQTT == 1) {
      checkMQTT();
      if (mqtt.connected() == 1)
      {
        mqtt.loop();
      }
    }

    ArduinoOTA.handle();  // For OTA
    // Disable interrupt it OTA is starting, otherwise it will not work
    ArduinoOTA.onStart([]() {
      
      #if defined(ESP8266) 
      timer1_disable();
      #endif
      #if defined(ESP32) 
      timerAlarmDisable(timer);
      #endif
      digitalWrite(pinRelayHeater, LOW); //Stop heating
    });
    ArduinoOTA.onError([](ota_error_t error) {
      #if defined(ESP8266) 
      timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
      #endif
      #if defined(ESP32) 
      timerAlarmDisable(timer);
      #endif
    });
    // Enable interrupts if OTA is finished
    ArduinoOTA.onEnd([]() {
      #if defined(ESP8266) 
       timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
      #endif
      #if defined(ESP32)
        timerAlarmEnable(timer);
      #endif
    });

    if (Blynk.connected()) {  // If connected run as normal
      Blynk.run();
      blynkReCnctCount = 0; //reset blynk reconnects if connected
    } else  {
      checkBlynk();
    }
    wifiReconnects = 0;   //reset wifi reconnects if connected
  } else {
    checkWifi();
  }
    if (TOF != 0) {
        VL53L0X_RangingMeasurementData_t measure;  //TOF Sensor measurement
        lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!
        distance = measure.RangeMilliMeter;  //write new distance value to 'distance'
        if (distance <= 1000)
        {
        percentage = (100 / (water_empty - water_full))* (water_empty - distance); //calculate percentage of waterlevel
        }
    }
  // voids
    refreshTemp();   //read new temperature values
    testEmergencyStop();  // test if Temp is to high
    brew();   //start brewing if button pressed
    checkSteamON(); // check for steam
    sendToBlynk();
    machinestatevoid() ; // calc machinestate
   if(ETRIGGER == 1) // E-Trigger active then void Etrigger() 
    { 
      ETriggervoid();
    }  
  

  //check if PID should run or not. If not, set to manuel and force output to zero
  if (pidON == 0 && pidMode == 1) {
    pidMode = 0;
    bPID.SetMode(pidMode);
    Output = 0 ;
  } else if (pidON == 1 && pidMode == 0 && !sensorError && !emergencyStop && backflushState == 10) {
    pidMode = 1;
    bPID.SetMode(pidMode);
  }

  //Sicherheitsabfrage
  if (!sensorError && Input > 0 && !emergencyStop && backflushState == 10 && (backflushON == 0 || brewcounter > 10)) {
    brewdetection();  //if brew detected, set PID values
      #if DISPLAY != 0
          unsigned long currentMillisDisplay = millis();
          if (currentMillisDisplay - previousMillisDisplay >= 100) 
          {
          displayShottimer() ;
          }
          if (currentMillisDisplay - previousMillisDisplay >= intervalDisplay) {
            previousMillisDisplay = currentMillisDisplay;
            #if DISPLAYTEMPLATE < 20 // not in vertikal template
              heatinglogo(); 
              OFFlogo(); 
              steamLogo();
            #endif
            printScreen();  // refresh display
      #endif
      }
    //Set PID if first start of machine detected, and no SteamON
    if ((Input < BrewSetPoint) && kaltstart && SteamON == 0) {
      if (startTn != 0) {
        startKi = startKp / startTn;
      } else {
        startKi = 0 ;
      }
      bPID.SetTunings(startKp, startKi, 0, P_ON_M);
    // normal PID
    } else if (timerBrewdetection == 0 && SteamON == 0) {    //Prevent overwriting of brewdetection values
      // calc ki, kd
      if (aggTn != 0) {
        aggKi = aggKp / aggTn ;
      } else {
        aggKi = 0 ;
      }
      aggKd = aggTv * aggKp ;
      bPID.SetTunings(aggKp, aggKi, aggKd, PonE);
      kaltstart = false;
    }
    // BD PID
    if ( millis() - timeBrewdetection  < brewtimersoftware * 1000 && timerBrewdetection == 1 && SteamON == 0) {
      // calc ki, kd
      if (aggbTn != 0) {
        aggbKi = aggbKp / aggbTn ;
      } else {
        aggbKi = 0 ;
      }
      aggbKd = aggbTv * aggbKp ;
      bPID.SetTunings(aggbKp, aggbKi, aggbKd, PonE) ;
    }
    // Steamon
    if (SteamON == 1)
    {
       if (aggTn != 0) {
        aggKi = aggKp / aggTn ;
      } else {
        aggKi = 0 ;
      }
      aggKi = 0 ;
      aggKd = aggTv * aggKp ;
      bPID.SetTunings(150, 0, 0, PonE);
    }

  } else if (sensorError) 
  {
    //Deactivate PID
    if (pidMode == 1) 
    {
      pidMode = 0;
      bPID.SetMode(pidMode);
      Output = 0 ;
    }
    digitalWrite(pinRelayHeater, LOW); //Stop heating
      #if DISPLAY != 0
        displayMessage("Error, Temp: ", String(Input), "Check Temp. Sensor!", "", "", ""); //DISPLAY AUSGABE
      #endif 
  } else if (emergencyStop) 
  {
    //Deactivate PID
    if (pidMode == 1) 
    {
      pidMode = 0;
      bPID.SetMode(pidMode);
      Output = 0 ;
    }

    digitalWrite(pinRelayHeater, LOW); //Stop heating
    #if DISPLAY != 0
      displayEmergencyStop();
    #endif 
  } 
  else if (backflushON || backflushState > 10) {
    if (backflushState == 43) {
      #if DISPLAY != 0
        displayMessage("Backflush finished", "Please reset brewswitch...", "", "", "", "");
      #endif 
    } else if (backflushState == 10) {
      #if DISPLAY != 0
        displayMessage("Backflush activated", "Please set brewswitch...", "", "", "", "");
      #endif
    } else if ( backflushState > 10) {
      #if DISPLAY != 0
        displayMessage("Backflush running:", String(flushCycles), "from", String(maxflushCycles), "", "");
      #endif
    }
  }

}
