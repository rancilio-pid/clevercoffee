/********************************************************
 * Version 2.0.1 FINAL BETA
 *   
 * - Check the PIN Ports in the CODE!
 * 
 * This enhancement implementation is based on the
 * great work of the rancilio-pid (http://rancilio-pid.de/)
 * team. Hopefully it will be merged upstream soon. In case
 * of questions just contact, Tobias <medlor@web.de>
 * 
 *****************************************************/

#include "icon.h"

//Libraries for OTA
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

//Libraries for MQTT
#include <PubSubClient.h>

#include "userConfig.h" // needs to be configured by the user
//#include "Arduino.h"
#include <EEPROM.h>

#include "rancilio-pid.h"

RemoteDebug Debug;

//Define pins for outputs
#define pinRelayVentil    12
#define pinRelayPumpe     13
#define pinRelayHeater    14
#define pinLed            15

const char* sysVersion PROGMEM  = "Version 2.0.1 Beta";

/********************************************************
  definitions below must be changed in the userConfig.h file
******************************************************/
int Offlinemodus = OFFLINEMODUS;
const int Display = DISPLAY;
const int OnlyPID = ONLYPID;
const int TempSensor = TEMPSENSOR;
const int TempSensorRecovery = TEMPSENSORRECOVERY;
const int brewDetection = BREWDETECTION;
const int fallback = FALLBACK;
const int triggerType = TRIGGERTYPE;
const boolean ota = OTA;
const int grafana=GRAFANA;

// Wifi
const char* hostname = HOSTNAME;
const char* auth = AUTH;
const char* ssid = D_SSID;
const char* pass = PASS;

unsigned long lastWifiConnectionAttempt = millis();
const unsigned long wifiConnectionDelay = 10000; // try to reconnect every 10 seconds
unsigned int wifiReconnects = 0; //number of reconnects

// OTA
const char* OTAhost = OTAHOST;
const char* OTApass = OTAPASS;

//Blynk
const char* blynkaddress = BLYNKADDRESS;
const int blynkport = BLYNKPORT;

// MQTT
const int MQTT_MAX_PUBLISH_SIZE = 120; //see https://github.com/knolleary/pubsubclient/blob/master/src/PubSubClient.cpp
const int mqtt_enable = MQTT_ENABLE;
const char* mqtt_server_ip = MQTT_SERVER_IP;
const int mqtt_server_port = MQTT_SERVER_PORT;
const char* mqtt_username = MQTT_USERNAME;
const char* mqtt_password = MQTT_PASSWORD;
const char* mqtt_topic_prefix = MQTT_TOPIC_PREFIX;
char topic_will[256];
char topic_set[256];
unsigned long lastMQTTStatusReportTime = 0;
unsigned long lastMQTTStatusReportInterval = 1000; //mqtt send status-report every 1 second
const boolean mqtt_flag_retained = true;
unsigned long mqtt_dontPublishUntilTime = 0;
unsigned long mqtt_dontPublishBackoffTime = 10000; // Failsafe: dont publish if there are errors for 10 seconds
unsigned long mqtt_lastReconnectAttemptTime = 0;
unsigned int mqtt_reconnectAttempts = 0;
unsigned long mqtt_reconnect_incremental_backoff = 10000 ; //Failsafe: add 10sec to reconnect time after each connect-failure.
unsigned int mqtt_max_incremental_backoff = 5 ; // At most backoff <mqtt_max_incremenatl_backoff>+1 times (<mqtt_reconnect_incremental_backoff>ms): 60sec

WiFiClient espClient;
PubSubClient mqtt_client(espClient);
 
/********************************************************
   Vorab-Konfig
******************************************************/
int pidON = 1 ;             // 1 = control loop in closed loop
int relayON, relayOFF;      // used for relay trigger type. Do not change!
int activeState = 3;        // 1:= Coldstart required (maschine is cold) 
                            // 2:= Stabilize temperature after coldstart
                            // 3:= (default) Inner Zone detected (temperature near setPoint)
                            // 4:= Brew detected
                            // 5:= Outer Zone detected (temperature outside of "inner zone")
boolean emergencyStop = false; // Notstop bei zu hoher Temperatur

/********************************************************
   history of temperatures
*****************************************************/
const int numReadings = 75;             // number of values per Array
double readingstemp[numReadings];       // the readings from Temp
float readingstime[numReadings];        // the readings from time
int readIndex = 0;                      // the index of the current reading
int totaltime = 0 ;                     // the running time
unsigned long  timeBrewDetection = 0 ;
int timerBrewDetection = 0 ;
int i = 0;

/********************************************************
   PID Variables
*****************************************************/
const unsigned int windowSizeSeconds = 5;            // How often should PID.compute() run? must be >= 1sec
unsigned int windowSize = windowSizeSeconds * 1000;  // 1000=100% heater power => resolution used in TSR() and PID.compute().
unsigned int isrCounter = windowSize - 500;          // counter for ISR
double Input = 0, Output = 0;
double previousInput = 0;

unsigned long previousMillistemp;  // initialisation at the end of init()
unsigned long previousMillistemp2;
const long refreshTempInterval = 1000;
int pidMode = 1;                   //1 = Automatic, 0 = Manual

double setPoint = SETPOINT;
double starttemp = STARTTEMP;     //TODO add auto-tune

// State 1: Coldstart PID values
const int coldStartStep1ActivationOffset = 5;
// ... none ...

// State 2: Coldstart stabilization PID values
// ... none ...

// State 3: Inner Zone PID values
double aggKp = AGGKP;
double aggTn = AGGTN;
double aggTv = AGGTV;
#if (aggTn == 0)
double aggKi = 0;
#else
double aggKi = aggKp / aggTn;
#endif
double aggKd = aggTv * aggKp ;

// State 4: Brew PID values
// ... none ...

// State 5: Outer Zone Pid values
double aggoKp = AGGOKP;
double aggoTn = AGGOTN;
double aggoTv = AGGOTV;
#if (aggoTn == 0)
double aggoKi = 0;
#else
double aggoKi = aggoKp / aggoTn;
#endif
double aggoKd = aggoTv * aggoKp ;
const double outerZoneTemperatureDifference = 1;

/********************************************************
   PID with Bias (steadyPower) Temperature Controller
*****************************************************/
#include "PIDBias.h"
double steadyPower = STEADYPOWER; // in percent. TODO config + eeprom
int burstShot      = 0;   // this is 1, when the user wants to immediatly set the heater power to the value specified in burstPower
double burstPower  = 20;  // in percent
int testTrigger    = 0;

// If the espresso hardware itself is cold, we need additional power for steadyPower to hold the water temperature
double steadyPowerOffset   = STEADYPOWER_OFFSET;  // heater power (in percent) which should be added to steadyPower during steadyPowerOffset_Time
int steadyPowerOffset_Time = STEADYPOWER_OFFSET_TIME * 1000;  // timeframe (in ms) for which steadyPowerOffset_Activated should be active
unsigned long steadyPowerOffset_Activated = 0;

PIDBias bPID(&Input, &Output, &steadyPower, &setPoint, aggKp, aggKi, aggKd, &Debug);

/********************************************************
   Analog Schalter Read
******************************************************/
double brewtime          = BREWTIME * 1000;
double preinfusion       = PREINFUSION * 1000;
double preinfusionpause  = PREINFUSION_PAUSE * 1000;
const int analogPin      = 0; // will be use in case of hardware
int brewing              = 0;
int brewswitch           = 0;
bool waitingForBrewSwitchOff = false;
double totalbrewtime     = 0;
unsigned long bezugsZeit = 0;
unsigned long startZeit  = 0;
unsigned long previousBrewCheck = 0;
unsigned long lastBrewMessage   = 0;

/********************************************************
   Sensor check
******************************************************/
boolean sensorError = false;
int error           = 0;
int maxErrorCounter = 10 ;  //define maximum number of consecutive polls (of intervaltempmes* duration) to have errors

/********************************************************
 * Rest
 *****************************************************/
#ifdef EMERGENCY_TEMP
const unsigned int emergency_temperature = EMERGENCY_TEMP;  // temperature at which the emergency shutdown should take place. DONT SET IT ABOVE 120 DEGREE!!
#else
const unsigned int emergency_temperature = 120;             // fallback
#endif
double brewDetectionSensitivity = BREWDETECTION_SENSITIVITY ; // if temperature decreased within the last 6 seconds by this amount, then we detect a brew.
#ifdef BREW_READY_DETECTION
const int brew_ready_led_enabled = BREW_READY_LED;
float marginOfFluctuation = float(BREW_READY_DETECTION);
#else
const int brew_ready_led_enabled = 0;   // 0 = disable functionality
float marginOfFluctuation = 0;          // 0 = disable functionality
#endif
bool brewReady = false;
char debugline[100];
unsigned long output_timestamp = 0;

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
   DALLAS TEMP
******************************************************/
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2         // Data wire is plugged into port 2 on the Arduino
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.
DeviceAddress sensorDeviceAddress; // arrays to hold device address

/********************************************************
   B+B Sensors TSIC 306
******************************************************/
#include "TSIC.h"    // include the library
TSIC Sensor1(2);     // only Signalpin, VCCpin unused by default
uint16_t temperature = 0;
float Temperatur_C = 0;
int refreshTempPreviousTimeSpend = 0;

/********************************************************
   BLYNK
******************************************************/
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
#define BLYNK_GREEN     "#23C48E"
#define BLYNK_YELLOW    "#ED9D00"
#define BLYNK_RED       "#D3435C"
unsigned long previousMillisBlynk;  // initialisation at the end of init()
const long intervalBlynk = 1000;    //Update Intervall zur App
int blynksendcounter = 1;
unsigned long previousMillisDisplay;  // initialisation at the end of init()
const long intervalDisplay = 500;     //Update für Display

/******************************************************
 * Receive following BLYNK PIN values from app/server
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
BLYNK_WRITE(V12) {
  starttemp = param.asDouble();
}
BLYNK_WRITE(V13) {
  pidON = param.asInt();
}
BLYNK_WRITE(V30) {
  aggoKp = param.asDouble();
}
BLYNK_WRITE(V31) {
  aggoTn = param.asDouble();
}
BLYNK_WRITE(V32) {
  aggoTv =  param.asDouble();
}
BLYNK_WRITE(V34) {
  brewDetectionSensitivity =  param.asDouble();
}
BLYNK_WRITE(V40) {
  burstShot =  param.asInt();
}
BLYNK_WRITE(V41) {
  steadyPower =  param.asDouble();
  // TODO fix this bPID.SetSteadyPowerDefault(steadyPower); //TOBIAS: working?
}
BLYNK_WRITE(V42) {
  steadyPowerOffset =  param.asDouble();
}
BLYNK_WRITE(V43) {
  steadyPowerOffset_Time =  param.asInt() * 1000;
}
BLYNK_WRITE(V44) {
  burstPower =  param.asDouble();
}

/******************************************************
 * Type Definition of "sending" BLYNK PIN values from 
 * hardware to app/server (only defined if required)
 ******************************************************/
WidgetLED brewReadyLed(V14);

/********************************************************
  MQTT
*****************************************************/
#include <math.h>
#include <float.h>
bool almostEqual(float a, float b) {
    return fabs(a - b) <= FLT_EPSILON;
}
char* bool2string(bool in) {
  char ret[22];
  if (in) {
    return "1";
  } else {
    return "0";
  }
}
char* number2string(double in) {
  char ret[22];
  snprintf(ret, sizeof(ret), "%0.2f", in);
  return ret;
}
char* number2string(float in) {
  char ret[22];
  snprintf(ret, sizeof(ret), "%0.2f", in);
  return ret;
}
char* number2string(int in) {
  char ret[22];
  snprintf(ret, sizeof(ret), "%d", in);
  return ret;
}
char* number2string(unsigned int in) {
  char ret[22];
  snprintf(ret, sizeof(ret), "%u", in);
  return ret;
}

char* mqtt_build_topic(char* reading) {
  char* topic = (char *) malloc(sizeof(char) * 256);
  snprintf(topic, sizeof(topic), "%s%s/%s", mqtt_topic_prefix, hostname, reading);
  return topic;
}

boolean mqtt_publish(char* reading, char* payload) {
  if (!mqtt_enable) return true;
  char topic[MQTT_MAX_PUBLISH_SIZE];
  if (!mqtt_client.connected()) {
    ERROR_print("Not connected to mqtt server. Cannot publish(%s)\n", payload);
    return false;
  }
  snprintf(topic, MQTT_MAX_PUBLISH_SIZE, "%s%s/%s", mqtt_topic_prefix, hostname, reading);
  if (strlen(topic) + strlen(payload) >= MQTT_MAX_PUBLISH_SIZE) {
    ERROR_print("mqtt_publish() wants to send too much data (len=%u)\n", strlen(topic) + strlen(payload));
    return false;
  } else {
    unsigned long currentMillis = millis();
    if (currentMillis > mqtt_dontPublishUntilTime) {
      boolean ret = mqtt_client.publish(topic, payload, mqtt_flag_retained);
      if (ret == false) { //TODO test this code block later (faking an error, eg millis <30000?)
        mqtt_dontPublishUntilTime = millis() + mqtt_dontPublishBackoffTime;
        ERROR_print("Error on publish. Wont publish the next %ul ms\n", mqtt_dontPublishBackoffTime);
      }
      return ret;
    } else { //TODO test this code block later (faking an error)
      ERROR_print("Data not published (still for the next %ul ms)\n", mqtt_dontPublishUntilTime - currentMillis);
      return false;
    }
  }
}

boolean mqtt_reconnect() {
  if (!mqtt_enable) return true;
  espClient.setTimeout(2000); // set timeout for mqtt connect()/write() to 2 seconds (default 5 seconds).
  if (mqtt_client.connect(hostname, mqtt_username, mqtt_password, topic_will, 0, 0, "unexpected exit")) {
    DEBUG_print("Connected to mqtt server\n");
    mqtt_publish("events", "Connected to mqtt server");
    mqtt_client.subscribe(topic_set);
  } else {
    DEBUG_print("Cannot connect to mqtt server (consecutive failures=#%u)\n", mqtt_reconnectAttempts);
  }
  return mqtt_client.connected();
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  DEBUG_print("Message arrived [%s]: %s\n", topic, (const char *)payload);
  // OPTIONAL TODO: business logic to activate rancilio functions from external (eg brewing, PidOn, startup, PID parameters,..)
}

/********************************************************
  Emergency Stop when temp too high
*****************************************************/
void testEmergencyStop(){
  if (getCurrentTemperature() >= emergency_temperature){
    if (emergencyStop != true) {
      snprintf(debugline, sizeof(debugline), "EmergencyStop because temperature>%u (temperature=%0.2f)", emergency_temperature, getCurrentTemperature());
      ERROR_println(debugline);
      mqtt_publish("events", debugline);
    }
    emergencyStop = true;
  } else if (getCurrentTemperature() < 100) {
    if (emergencyStop == true) {
      snprintf(debugline, sizeof(debugline), "EmergencyStop ended because temperature<100 (temperature=%0.2f)", getCurrentTemperature());
      ERROR_println(debugline);
      mqtt_publish("events", debugline);
    }
    emergencyStop = false;
  }
}

/********************************************************
  history temperature data
*****************************************************/
void updateTemperatureHistory(double myInput) {
  if (readIndex >= numReadings -1) {
    readIndex = 0;
  } else {
    readIndex++;
  }
  readingstime[readIndex] = millis();
  readingstemp[readIndex] = myInput;
}

//calculate the temperature difference between NOW and a datapoint in history
double pastTemperatureChange(int lookback) {
  double temperatureDiff;
  if (lookback >= numReadings) lookback=numReadings -1;
  int offset = lookback % numReadings;
  int historicIndex = (readIndex - offset);
  if ( historicIndex < 0 ) {
    historicIndex += numReadings;
  }
  //ignore not yet initialized values
  if (readingstime[readIndex] == 0 || readingstime[historicIndex] == 0) return 0;
  if (brewDetectionSensitivity <= 30) {
    temperatureDiff = (readingstemp[readIndex] - readingstemp[historicIndex]);
  } else { // use previous factor on brewDetectionSensitivity threshold (compatibility to old brewDetectionSensitivity values using a factor of 100)
    temperatureDiff = (readingstemp[readIndex] - readingstemp[historicIndex]) * 100;
  }
  return temperatureDiff;
}

//calculate the average temperature over the last (lookback) temperatures samples
double getAverageTemperature(int lookback) {
  double averageInput = 0;
  int count = 0;
  if (lookback >= numReadings) lookback=numReadings -1;
  for (int offset = 0; offset < lookback; offset++) {
    int thisReading = readIndex - offset;
    if (thisReading < 0) thisReading = numReadings + thisReading;
    if (readingstime[thisReading] == 0) break;
    averageInput += readingstemp[thisReading];
    count += 1;
  }
  if (count > 0) {
    return averageInput / count;
  } else {
    DEBUG_print("getAverageTemperature() returned 0");
    return 0;
  }
}

double getCurrentTemperature() {
  return readingstemp[readIndex];
}

//returns heater utilization in percent
double convertOutputToUtilisation(double Output) {
  return (100 * Output) / windowSize;
}

//returns heater utilization in Output
double convertUtilisationToOutput(double utilization) {
  return (utilization / 100 ) * windowSize;
}

bool checkBrewReady(double setPoint, float marginOfFluctuation, int lookback) {
  if (almostEqual(marginOfFluctuation, 0)) return false;
  if (lookback >= numReadings) lookback=numReadings -1;
  for (int offset = 0; offset < lookback; offset++) {
    int thisReading = readIndex - offset;
    if (thisReading < 0) thisReading = numReadings + thisReading;
    if (readingstime[thisReading] == 0) return false;
    if (fabs(setPoint - readingstemp[thisReading]) > (marginOfFluctuation + FLT_EPSILON)) return false;
  }
  return true;
}

void refreshBrewReadyHardwareLed(boolean brewReady) {
  static boolean lastBrewReady = false;
  if (!brew_ready_led_enabled) return;
  if (brewReady != lastBrewReady) {
    digitalWrite(pinLed, brewReady);
    lastBrewReady = brewReady;
  }
}

/********************************************************
  check sensor value. If < 0 or difference between old and new >10, then increase error.
  If error is equal to maxErrorCounter, then set sensorError
*****************************************************/
boolean checkSensor(float tempInput, float temppreviousInput) {
  boolean sensorOK = false;
  /********************************************************
    sensor error
  ******************************************************/
  if ( ( tempInput < 0 || tempInput > 150 || fabs(tempInput - temppreviousInput) > 5) && !sensorError) {
    error++;
    sensorOK = false;
    DEBUG_print("temperature sensor reading: consec_errors=%d, temp_current=%0.2f, temp_prev=%0.2f\n", error, tempInput, temppreviousInput);
  } else {
    error = 0;
    sensorOK = true;
  }

  if (error >= maxErrorCounter && !sensorError) {
    sensorError = true;
    snprintf(debugline, sizeof(debugline), "temperature sensor malfunction: temp_current=%0.2f, temp_prev=%0.2f", tempInput, previousInput);
    ERROR_println(debugline);
    mqtt_publish("events", debugline);
  } else if (error == 0 && TempSensorRecovery == 1) { //Safe-guard: prefer to stop heating forever if sensor is flapping!
    sensorError = false;
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
  previousInput = getCurrentTemperature() ;
  long millis_elapsed = currentMillistemp - previousMillistemp ;
  if ( millis_elapsed <0 ) millis_elapsed = 0;
  if (TempSensor == 1)
  {
    if ( floor(millis_elapsed / refreshTempInterval) >= 2) {
      snprintf(debugline, sizeof(debugline), "Temporary main loop() hang. Number of temp polls missed=%g, millis_elapsed=%lu", floor(millis_elapsed / refreshTempInterval) -1, millis_elapsed);
      ERROR_println(debugline);
      mqtt_publish("events", debugline);
    }
    if (millis_elapsed >= refreshTempInterval)
    {
      sensors.requestTemperatures();
      previousMillistemp = currentMillistemp;
      if (!checkSensor(sensors.getTempCByIndex(0), previousInput)) return;  //if sensor data is not valid, abort function
      updateTemperatureHistory(sensors.getTempCByIndex(0));
      Input = getAverageTemperature(5);
    }
  }
  if (TempSensor == 2)
  {
    if ( floor(millis_elapsed / refreshTempInterval) >= 2) {
      snprintf(debugline, sizeof(debugline), "Temporary main loop() hang. Number of temp polls missed=%g, millis_elapsed=%lu", floor(millis_elapsed / refreshTempInterval) -1, millis_elapsed);
      ERROR_println(debugline);
      mqtt_publish("events", debugline);
    }
    if (millis_elapsed >= refreshTempInterval)
    {
      // variable "temperature" must be set to zero, before reading new data
      // getTemperature only updates if data is valid, otherwise "temperature" will still hold old values
      temperature = 0;
      //unsigned long start = millis();
      Sensor1.getTemperature(&temperature);
      unsigned long stop = millis();
      previousMillistemp = stop;
      
      // temperature must be between 0x000 and 0x7FF(=DEC2047)
      Temperatur_C = Sensor1.calc_Celsius(&temperature); 

      //DEBUG_print("millis=%lu | previousMillistemp=%lu | diff=%lu | Temperatur_C=%0.3f | time_spend=%lu\n", millis(), previousMillistemp, currentMillistemp - previousMillistemp2, Temperatur_C, stop - start);  //TOBIAS
      //previousMillistemp2 = currentMillistemp;
      
      // Temperature_C must be -50C < Temperature_C <= 150C
      if (!checkSensor(Temperatur_C, previousInput)) {
        return;  //if sensor data is not valid, abort function
      }
      updateTemperatureHistory(Temperatur_C);
      Input = getAverageTemperature(5);
    }
  }
}

/********************************************************
    PreInfusion, Brew , if not Only PID
******************************************************/
void brew() {
  if (OnlyPID == 0) {
    unsigned long aktuelleZeit = millis();
    
    if ( aktuelleZeit >= previousBrewCheck + 50 ) {  //50ms
      previousBrewCheck = aktuelleZeit;
      brewswitch = analogRead(analogPin);

      //if (aktuelleZeit >= output_timestamp + 500) {
      //  DEBUG_print("brew(): brewswitch=%u | brewing=%u | waitingForBrewSwitchOff=%u\n", brewswitch, brewing, waitingForBrewSwitchOff);
      //  output_timestamp = aktuelleZeit;
      //}
      if (brewswitch > 700 && not (brewing == 0 && waitingForBrewSwitchOff) ) {
        totalbrewtime = preinfusion + preinfusionpause + brewtime;
        
        if (brewing == 0) {
          brewing = 1;
          startZeit = aktuelleZeit;
          waitingForBrewSwitchOff = true;
          DEBUG_print("brewswitch=on - Starting brew()\n");
        }
        bezugsZeit = aktuelleZeit - startZeit; 
  
        //if (aktuelleZeit >= lastBrewMessage + 500) {
        //  lastBrewMessage = aktuelleZeit;
        //  DEBUG_print("brew(): bezugsZeit=%lu totalbrewtime=%0.1f\n", bezugsZeit/1000, totalbrewtime/1000);
        //}
        if (bezugsZeit <= totalbrewtime) {
          if (bezugsZeit <= preinfusion) {
            //DEBUG_println("preinfusion");
            digitalWrite(pinRelayVentil, relayON);
            digitalWrite(pinRelayPumpe, relayON);
          } else if (bezugsZeit > preinfusion && bezugsZeit <= preinfusion + preinfusionpause) {
            //DEBUG_println("Pause");
            digitalWrite(pinRelayVentil, relayON);
            digitalWrite(pinRelayPumpe, relayOFF);
          } else if (bezugsZeit > preinfusion + preinfusionpause) {
            //DEBUG_println("Brew");
            digitalWrite(pinRelayVentil, relayON);
            digitalWrite(pinRelayPumpe, relayON);
          }
        } else {
          DEBUG_print("End brew()\n");
          brewing = 0;
        }
      }
  
      if (brewswitch <= 700) {
        if (waitingForBrewSwitchOff) {
          DEBUG_print("brewswitch=off\n");
        }
        waitingForBrewSwitchOff = false;
        brewing = 0;
        bezugsZeit = 0;
      }
      if (brewing == 0) {
          //DEBUG_println("aus");
          digitalWrite(pinRelayVentil, relayOFF);
          digitalWrite(pinRelayPumpe, relayOFF);
      }
    }
  }
}

 /********************************************************
   Check if Wifi is connected, if not reconnect
 *****************************************************/
 void checkWifi(){
   if (Offlinemodus == 1) return;
   int statusTemp = WiFi.status();
   // check WiFi connection:
   if (statusTemp != WL_CONNECTED) {
     // (optional) "offline" part of code
      // check delay:
     if (millis() - lastWifiConnectionAttempt >= wifiConnectionDelay) {
       lastWifiConnectionAttempt = millis();      
       // attempt to connect to Wifi network:
       WiFi.hostname(hostname);
       WiFi.begin(ssid, pass); 
       delay(5000);    //will not work without delay
       wifiReconnects++; 
     }
    }
 }

/********************************************************
  send data to Blynk server
*****************************************************/
void sendToBlynk() {
  if (Offlinemodus != 0) return;
  unsigned long currentMillisBlynk = millis();
  if (currentMillisBlynk - previousMillisBlynk >= intervalBlynk) {
    previousMillisBlynk = currentMillisBlynk;
    if (Blynk.connected()) {
      if (brewReady) {
        brewReadyLed.setColor(BLYNK_GREEN);
      } else if (marginOfFluctuation != 0 && checkBrewReady(setPoint, marginOfFluctuation * 2, 40)) {
        brewReadyLed.setColor(BLYNK_YELLOW);
      } else {
        brewReadyLed.on();
        brewReadyLed.setColor(BLYNK_RED);
      }
      if (grafana == 1) {
        Blynk.virtualWrite(V60, Input, Output, bPID.GetKp(), bPID.GetKi(), bPID.GetKd(), setPoint );
      }
      if (blynksendcounter == 1) {
        //Blynk.virtualWrite(V2, (float)(Input * 100L) / 100.0);
        Blynk.virtualWrite(V2, String(Input, 2));
        Blynk.syncVirtual(V2);
        Blynk.virtualWrite(V3, setPoint);
        Blynk.syncVirtual(V3);
        Blynk.virtualWrite(V11, String(Input - setPoint, 2));
        Blynk.syncVirtual(V11);
      }
      if (blynksendcounter == 2) {
        Blynk.virtualWrite(V23, String(convertOutputToUtilisation(Output), 2));
        Blynk.syncVirtual(V23);
        Blynk.virtualWrite(V35, String(pastTemperatureChange(10)/2, 2));
        Blynk.syncVirtual(V35);
      }
      if (blynksendcounter >= 3) {
        Blynk.virtualWrite(V41, steadyPower);
        Blynk.syncVirtual(V41);
        blynksendcounter = 0;
      }
      blynksendcounter++;
    } else {
      DEBUG_println("Wifi working but blynk not connected..\n");
    }
  }
}

/********************************************************
    state Detection
******************************************************/
void updateState() {
  switch (activeState) {
    case 1: // state 1 running, that means full heater power. Check if target temp is reached
    {
      bPID.SetFilterSumOutputI(100);
      if (Input >= starttemp) {
        snprintf(debugline, sizeof(debugline), "** End of Coldstart. Transition to step 2 (constant steadyPower)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        bPID.SetSumOutputI(0);
        activeState = 2;
      }
      break;
    }
    case 2: // state 2 running, that means heater is on steadyState and we are waiting to temperature to stabilize
    {
      bPID.SetFilterSumOutputI(30);
      double tempChange = pastTemperatureChange(20);
      if ( (Input - setPoint >= 0) || (Input - setPoint <= -20) || (Input - setPoint <= 0  && tempChange <= 0.3)) {
        snprintf(debugline, sizeof(debugline), "** End of stabilizing. Transition to step 3 (normal mode)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        bPID.SetSumOutputI(0);
        activeState = 3;
      }
      break;
    }
    case 4: // state 4 running = Brew running
    {
      bPID.SetFilterSumOutputI(100);
      bPID.SetAutoTune(false);
      if (Input > setPoint - outerZoneTemperatureDifference ||
          pastTemperatureChange(10) >= 0.5) {
        snprintf(debugline, sizeof(debugline), "** End of Brew. Transition to step 3 (normal mode)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        bPID.SetAutoTune(true);
        bPID.SetSumOutputI(0);
        timerBrewDetection = 0 ;
        activeState = 3;
      }
      break;
    }
    case 5: // state 5 in outerZone
    {
      bPID.SetFilterSumOutputI(9);
      if (Input > setPoint - outerZoneTemperatureDifference) {
        snprintf(debugline, sizeof(debugline), "** End of outerZone. Transition to step 3 (normal mode)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        bPID.SetSumOutputI(0);
        timerBrewDetection = 0 ;
        activeState = 3;
      }
      break;
    }
    case 3: // normal PID mode
    default:
    {
      //set maximum allowed filterSumOutputI based on error/marginOfFluctuation
      if ( Input >= setPoint - marginOfFluctuation) {
        bPID.SetFilterSumOutputI(1.0);
      } else if ( Input >= setPoint - 0.5) {
        bPID.SetFilterSumOutputI(4.5);
      } else {
        bPID.SetFilterSumOutputI(6);
      } 
      
      /* STATE 1 (COLDSTART) DETECTION */
      if ( Input <= starttemp - coldStartStep1ActivationOffset) {
        snprintf(debugline, sizeof(debugline), "** End of normal mode. Transition to step 1 (coldstart)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        bPID.SetSteadyPowerOffset(steadyPowerOffset);
        steadyPowerOffset_Activated = millis();
        bPID.SetAutoTune(false);  //do not tune during powerOffset
        DEBUG_print("Enable steadyPowerOffset (steadyPower += %0.2f)\n", steadyPowerOffset);
        bPID.SetSumOutputI(0);
        activeState = 1;
        break;
      }

      /* STATE 4 (BREW) DETECTION */
      if (brewDetectionSensitivity != 0 && brewDetection == 1) {
        //enable brew-detection if not already running and diff temp is > brewDetectionSensitivity
        if (pastTemperatureChange(6) <= -brewDetectionSensitivity &&
            Input < setPoint - outerZoneTemperatureDifference) {
          testTrigger = 0;
          if (OnlyPID == 1) {
            bezugsZeit = 0 ;
          }
          timeBrewDetection = millis() ;
          timerBrewDetection = 1 ;
          mqtt_publish("brewDetected", "1");
          snprintf(debugline, sizeof(debugline), "** End of normal mode. Transition to step 4 (brew)");
          DEBUG_println(debugline);
          mqtt_publish("events", debugline);
          bPID.SetSumOutputI(0);
          activeState = 4;
          break;
        }
      }

      /* STATE 5 (OUTER ZONE) DETECTION */
      if ( (Input > starttemp - coldStartStep1ActivationOffset && 
            Input < setPoint - outerZoneTemperatureDifference) || testTrigger  ) {
        snprintf(debugline, sizeof(debugline), "** End of normal mode. Transition to step 5 (outerZone)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        bPID.SetSumOutputI(0);
        activeState = 5;
        break;
      }

      break;
    }
  }
  
  // steadyPowerOffset_Activated handling
  if ( steadyPowerOffset_Activated >0 ) {
    if (Input - setPoint >= 0.4) {
      bPID.SetSteadyPowerOffset(0);
      steadyPowerOffset_Activated = 0;
      snprintf(debugline, sizeof(debugline), "Disabled steadyPowerOffset because its too large or starttemp too high");
      ERROR_println(debugline);
      mqtt_publish("events", debugline);
      bPID.SetAutoTune(true);
    }
    if (millis() >= steadyPowerOffset_Activated + steadyPowerOffset_Time) {
      //DEBUG_print("millis=%lu | steadyPowerOffset_Activated=%0.2f | steadyPowerOffset_Time=%d\n", millis(), steadyPowerOffset_Activated, steadyPowerOffset_Time);
      bPID.SetSteadyPowerOffset(0);
      steadyPowerOffset_Activated = 0;
      DEBUG_print("Disable steadyPowerOffset (steadyPower -= %0.2f)\n", steadyPowerOffset);
      bPID.SetAutoTune(true);
    }
  }
}

void ICACHE_RAM_ATTR onTimer1ISR() {
  timer1_write(50000); // set interrupt time to 10ms
  //run PID calculation
  if ( bPID.Compute() ) {
    isrCounter = 0;  // Attention: heater might not shutdown if bPid.SetSampleTime(), windowSize, timer1_write() and are not set correctly!
    DEBUG_print("Input=%6.2f | error=%5.2f delta=%5.2f | Output=%6.2f = b:%5.2f + p:%5.2f + i:%5.2f(%5.2f) + d:%5.2f\n", 
      Input,
      (setPoint - Input),
      pastTemperatureChange(10)/2,
      convertOutputToUtilisation(Output),
      steadyPower + ((steadyPowerOffset_Activated) ? steadyPowerOffset: 0),
      convertOutputToUtilisation(bPID.GetOutputP()),
      convertOutputToUtilisation(bPID.GetSumOutputI()),
      convertOutputToUtilisation(bPID.GetOutputI()),
      convertOutputToUtilisation(bPID.GetOutputD())
      );
  }
  if (isrCounter >= Output) {
    digitalWrite(pinRelayHeater, LOW);
  } else {
    digitalWrite(pinRelayHeater, HIGH);
  }
  //increase counter until fail-safe is reached
  if (isrCounter <= windowSize) {
    isrCounter += 10; // += 10 because one tick = 10ms
  }
}


/***********************************
 * LOOP()
 ***********************************/
void loop() {

  ArduinoOTA.handle();  // For OTA
  // Disable interrupt it OTA is starting, otherwise it will not work
  ArduinoOTA.onStart([](){
    timer1_disable();
    digitalWrite(pinRelayHeater, LOW); //Stop heating
    DEBUG_print("OTA update initiated\n");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
    ERROR_print("OTA update error\n");
  });
  // Enable interrupts if OTA is finished
  ArduinoOTA.onEnd([](){
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  });
 
  if (WiFi.status() == WL_CONNECTED){
    Blynk.run(); //Do Blynk magic stuff
    wifiReconnects = 0;

    //Check mqtt connection
    if (mqtt_enable && !mqtt_client.connected()) {
      unsigned long now = millis();
      if (now - mqtt_lastReconnectAttemptTime > (mqtt_reconnect_incremental_backoff * (mqtt_reconnectAttempts+1)) ) {
        mqtt_lastReconnectAttemptTime = now;
        // Attempt to reconnect
        if (mqtt_reconnect()) {
          mqtt_lastReconnectAttemptTime = 0;
          mqtt_reconnectAttempts = 0;
        } else if (mqtt_reconnectAttempts < mqtt_max_incremental_backoff) {
          mqtt_reconnectAttempts++;
        }
      }
    } else if (mqtt_enable) {
      // mqtt client connected, do mqtt housekeeping
      mqtt_client.loop();
      unsigned long now = millis();
      if (now - lastMQTTStatusReportTime >= lastMQTTStatusReportInterval) {
        lastMQTTStatusReportTime = now;
        mqtt_publish("temperature", number2string(Input));
        mqtt_publish("temperatureAboveTarget", number2string((Input - setPoint)));
        mqtt_publish("heaterUtilization", number2string(convertOutputToUtilisation(Output)));
        mqtt_publish("kp", number2string(bPID.GetKp()));
        mqtt_publish("ki", number2string(bPID.GetKi()));
        mqtt_publish("kd", number2string(bPID.GetKd()));
        mqtt_publish("outputP", number2string(convertOutputToUtilisation(bPID.GetOutputP())));
        mqtt_publish("outputI", number2string(convertOutputToUtilisation(bPID.GetOutputI())));
        mqtt_publish("outputD", number2string(convertOutputToUtilisation(bPID.GetOutputD())));
        mqtt_publish("pastTemperatureChange", number2string(pastTemperatureChange(10)));
        mqtt_publish("brewReady", bool2string(brewReady));
       }
    }
  } else {
    checkWifi();
  }
  unsigned long startT;
  unsigned long stopT;

  refreshTemp();   //read new temperature values
  testEmergencyStop();  // test if Temp is to high
  brew();   //start brewing if button pressed
  brewReady = checkBrewReady(setPoint, marginOfFluctuation, 40);
  refreshBrewReadyHardwareLed(brewReady);

  //check if PID should run or not. If not, set to manuel and force output to zero
  if (pidON == 0 && pidMode == 1) {
    pidMode = 0;
    bPID.SetMode(pidMode);
    Output = 0 ;
    DEBUG_print("Set PID=off\n");
  } else if (pidON == 1 && pidMode == 0) {
    Output = 0; // safety: be 100% sure that PID.compute() starts fresh.
    pidMode = 1;
    bPID.SetMode(pidMode);
    if ( millis() - output_timestamp > 21000) {
      DEBUG_print("Set PID=on\n");
      output_timestamp = millis();
    }
  }
  if (burstShot == 1 && pidMode == 1) {
    burstShot = 0;
    bPID.SetBurst(burstPower);
    snprintf(debugline, sizeof(debugline), "BURST Output=%0.2f", convertOutputToUtilisation(Output));
    DEBUG_println(debugline);
    mqtt_publish("events", debugline);
  }

  //Sicherheitsabfrage
  if (!sensorError && !emergencyStop && Input > 0) {
    updateState();

    /* state 1: Water is very cold, set heater to full power */
    if (activeState == 1) {
      Output = windowSize;  //fix mqtt to show correct values

    /* state 2: ColdstartTemp reached. Now stabilizing temperature after coldstart */
    } else if (activeState == 2) {
      Output = convertUtilisationToOutput(steadyPower);  //fix mqtt to show correct values

    /* state 4: Brew detected. Increase heater power */
    } else if (activeState == 4) {
      Output = windowSize;
      if (OnlyPID == 1 && timerBrewDetection == 1){
        bezugsZeit = millis() - timeBrewDetection;
      }

    /* state 5: Outer Zone reached. More power than in inner zone */
    } else if (activeState == 5) {
      if (aggoTn != 0) {
        aggoKi = aggoKp / aggoTn ;
      } else {
        aggoKi = 0;
      }
      aggoKd = aggoTv * aggoKp ;
      if (pidMode == 1) bPID.SetMode(AUTOMATIC);
      bPID.SetTunings(aggoKp, aggoKi, aggoKd);

    /* state 3: Inner zone reached = "normal" low power mode */
    } else {
      if (pidMode == 1) bPID.SetMode(AUTOMATIC);
      if (aggTn != 0) {
        aggKi = aggKp / aggTn ;
      } else {
        aggKi = 0 ;
      }
      aggKd = aggTv * aggKp ;
      bPID.SetTunings(aggKp, aggKi, aggKd);
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
      if ( millis() - output_timestamp > 15000) {
        ERROR_print("sensorError detected. Shutdown PID and heater\n");
        output_timestamp = millis();
      }
    }
    digitalWrite(pinRelayHeater, LOW); //Stop heating

    //DISPLAY AUSGABE
    snprintf(debugline, sizeof(debugline), "Temp: %0.2f", getCurrentTemperature());
    displaymessage("rancilio", "Check Temp. Sensor!", debugline, "");
    
  } else if (emergencyStop) {

    //Deactivate PID
    if (pidMode == 1) {
      pidMode = 0;
      bPID.SetMode(pidMode);
      Output = 0 ;
      if ( millis() - output_timestamp > 10000) {
         ERROR_print("emergencyStop detected. Shutdown PID and heater (temp=%0.2f)\n", getCurrentTemperature());
         output_timestamp = millis();
      }
    }

    digitalWrite(pinRelayHeater, LOW); //Stop heating

    //DISPLAY AUSGABE
    char line2[17];
    //char line3[17];
    snprintf(line2, sizeof(line2), "Temp: %0.2f", getCurrentTemperature());
    //snprintf(line3, sizeof(line3), "Temp > %u", emergency_temperature);
    displaymessage(EMERGENCY_ICON, EMERGENCY_TEXT, line2, "");

  } else {
    if ( millis() - output_timestamp > 15000) {
       ERROR_print("unknown error\n");
       output_timestamp = millis();
    }
  }
  Debug.handle();
}


/***********************************
 * SETUP()
 ***********************************/
void setup() {
  DEBUGSTART(115200);
  Debug.begin(hostname, Debug.DEBUG);
  Debug.setResetCmdEnabled(true); // Enable the reset command
  Debug.showProfiler(true); // Profiler (Good to measure times, to optimize codes)
  Debug.showColors(true); // Colors
  Debug.setSerialEnabled(true); // log to Serial also
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
  #ifdef BREW_READY_LED
  pinMode(pinLed, OUTPUT);
  digitalWrite(pinLed, LOW);
  #endif

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
  displaymessage("rancilio", sysVersion, "", "");
  delay(2000);

  /********************************************************
     BLYNK & Fallback offline
  ******************************************************/
  //TODO OFFLINE MODUS is totally broken in master and here
  if (Offlinemodus == 0) {

    WiFi.hostname(hostname);
    if (fallback == 0) {
      displaymessage("rancilio", "Connect to Blynk", "no Fallback", "");
      Blynk.begin(auth, ssid, pass, blynkaddress, blynkport);
    }

    if (fallback == 1) {
      unsigned long started = millis();
      displaymessage("rancilio", "1: Connect Wifi to:", ssid, "");

      /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
        would try to act as both a client and an access-point and could cause
        network-issues with your other WiFi-devices on your WiFi-network. */
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, pass);
      DEBUG_print("Connecting to %s ...\n", ssid);

      // wait 10 seconds for connection:
      while ((WiFi.status() != WL_CONNECTED) && (millis() - started < 20000))
      {
        yield();    //Prevent Watchdog trigger
      }

      if (WiFi.status() == WL_CONNECTED) {
        DEBUG_println("WiFi connected\n");
        DEBUG_print("IP address: %s\n", WiFi.localIP().toString().c_str());

        displaymessage("rancilio", "2: Wifi connected, ", "try Blynk   ", "");
        DEBUG_print("Wifi works, now try Blynk connection\n");
        delay(2000);
        Blynk.config(auth, blynkaddress, blynkport) ;
        Blynk.connect(30000);

        // Blnky works:
        if (Blynk.connected() == true) {
          displaymessage("rancilio", "3: Blynk connected", "sync all variables...", "");
          DEBUG_print("Blynk is online, new values to eeprom\n");
         // Blynk.run() ; 
          Blynk.syncVirtual(V4);
          Blynk.syncVirtual(V5);
          Blynk.syncVirtual(V6);
          Blynk.syncVirtual(V7);
          Blynk.syncVirtual(V8);
          Blynk.syncVirtual(V9);
          Blynk.syncVirtual(V10);
          Blynk.syncVirtual(V12);
          Blynk.syncVirtual(V13);
          Blynk.syncVirtual(V30);
          Blynk.syncVirtual(V31);
          Blynk.syncVirtual(V32);
          Blynk.syncVirtual(V34);
          Blynk.syncVirtual(V40);
          Blynk.syncVirtual(V41);
          Blynk.syncVirtual(V42);
          Blynk.syncVirtual(V43);
          Blynk.syncVirtual(V44);
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
          EEPROM.put(80, starttemp);
          EEPROM.put(90, aggoKp);
          EEPROM.put(100, aggoTn);
          EEPROM.put(110, aggoTv);
          EEPROM.put(130, brewDetectionSensitivity);
          EEPROM.put(140, steadyPower);
          EEPROM.put(150, steadyPowerOffset);
          EEPROM.put(160, steadyPowerOffset_Time);
          EEPROM.put(170, burstPower);
          // eeprom schließen
          EEPROM.commit();
        } else {
          ERROR_print("Not connected to Blynk\n");
        }
      }

      //TODO OFFLINE MODUS is totally broken in master and here.
      // add version info to Address 70 and use to validate structs
      if (WiFi.status() != WL_CONNECTED || Blynk.connected() != true) {
        displaymessage("rancilio", "Begin Fallback,", "No Blynk/Wifi", "");
        delay(2000);
        DEBUG_print("Start offline mode with eeprom values, no wifi or blynk :\n");
        Offlinemodus = 1 ;
        // eeprom öffnen
        EEPROM.begin(1024);
        // eeprom werte prüfen, ob numerisch
        double dummy;
        EEPROM.get(0, dummy);
        DEBUG_print("check eeprom 0x00 in dummy:\n");
        DEBUG_println(dummy);
        if (!isnan(dummy)) {
          EEPROM.get(0, aggKp);
          EEPROM.get(10, aggTn);
          EEPROM.get(20, aggTv);
          EEPROM.get(30, setPoint);
          EEPROM.get(40, brewtime);
          EEPROM.get(50, preinfusion);
          EEPROM.get(60, preinfusionpause);
          EEPROM.get(80, starttemp);
          EEPROM.get(90, aggoKp);
          EEPROM.get(100, aggoTn);
          EEPROM.get(110, aggoTv);
          EEPROM.get(130, brewDetectionSensitivity);
          EEPROM.get(140, steadyPower);
          EEPROM.get(150, steadyPowerOffset);
          EEPROM.get(160, steadyPowerOffset_Time);
          EEPROM.get(170, burstPower);
        }
        else
        {
          displaymessage("rancilio", "No eeprom,", "Value", "");
          ERROR_print("No working eeprom value, I am sorry, but use default offline value\n");
          delay(2000);
        }
        // eeeprom schließen
        EEPROM.commit();
      }

      // Connect to MQTT-Service (only if WIFI and Blynk are working)
      if (Offlinemodus == 0 && mqtt_enable) {
        snprintf(topic_will, sizeof(topic_will), "%s%s/%s", mqtt_topic_prefix, hostname, "will");
        snprintf(topic_set, sizeof(topic_set), "%s%s/%s", mqtt_topic_prefix, hostname, "set");
        mqtt_client.setServer(mqtt_server_ip, mqtt_server_port);
        mqtt_client.setCallback(mqtt_callback);
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
    movingaverage ini array
  ******************************************************/
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readingstemp[thisReading] = 0;
    readingstime[thisReading] = 0;
    //readingchangerate[thisReading] = 0;
  }

  /********************************************************
     TEMP SENSOR
  ******************************************************/
  if (TempSensor == 1) {
    sensors.begin();
    sensors.getAddress(sensorDeviceAddress, 0);
    sensors.setResolution(sensorDeviceAddress, 10) ;
    while (true) {
      sensors.requestTemperatures();
      previousInput = sensors.getTempCByIndex(0);
      delay(500);
      sensors.requestTemperatures();
      Input = sensors.getTempCByIndex(0);
      if (checkSensor(Input, previousInput)) {
        updateTemperatureHistory(Input);
        break;
      }
      delay(500);
    }
  }

  if (TempSensor == 2) {
    while (true) {
      temperature = 0;
      Sensor1.getTemperature(&temperature);
      previousInput = Sensor1.calc_Celsius(&temperature);
      delay(500);
      temperature = 0;
      Sensor1.getTemperature(&temperature);
      Input = Sensor1.calc_Celsius(&temperature);
      if (checkSensor(Input, previousInput)) {
        updateTemperatureHistory(Input);
        break;
      }
      delay(500);
    }
  }

  /********************************************************
     Ini PID
  ******************************************************/

  bPID.SetSampleTime(windowSize);
  bPID.SetOutputLimits(0, windowSize);
  bPID.SetMode(AUTOMATIC);

  /********************************************************
     REST INIT()
  ******************************************************/
  //Initialisation MUST be at the very end of the init(), otherwise the time comparison in loop() will have a big offset
  unsigned long currentTime = millis();
  previousMillistemp = currentTime;
  previousMillistemp2 = currentTime;;
  previousMillisDisplay = currentTime;
  previousMillisBlynk = currentTime;

  /********************************************************
    Timer1 ISR - Initialisierung
    TIM_DIV1 = 0,   //80MHz (80 ticks/us - 104857.588 us max)
    TIM_DIV16 = 1,  //5MHz (5 ticks/us - 1677721.4 us max)
    TIM_DIV256 = 3  //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
  ******************************************************/
  //delay(35);
  timer1_isr_init();
  timer1_attachInterrupt(onTimer1ISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(50000); // set interrupt time to 10ms
}


/********************************************************
  Displayausgabe
*****************************************************/
void displaymessage(String logo, String displaymessagetext, String displaymessagetext2, String displaymessagetext3) {
  if (Display == 2) {
    /********************************************************
       DISPLAY AUSGABE
    ******************************************************/
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.clearDisplay();
    display.setCursor(0, 47);
    display.println(displaymessagetext);
    if (displaymessagetext3 == "") {
      display.print(displaymessagetext2);
    } else {
      display.println(displaymessagetext2);
      display.print(displaymessagetext3);
    }
    //128x64
    if (logo == "rancilio") {
      display.drawBitmap(41,2, rancilio_logo_bits,rancilio_logo_width, rancilio_logo_height, WHITE);  //Rancilio startup logo
    } else if (logo == "steam") {
      display.drawBitmap(41,2, stream_logo_bits, stream_logo_width, stream_logo_height, WHITE);  //Rancilio startup logo
    }
    display.display();
  }
  if (Display == 1) {
    /********************************************************
       DISPLAY AUSGABE
    ******************************************************/
    u8x8.clear();
    u8x8.setFont(u8x8_font_chroma48medium8_r);  //Ausgabe vom aktuellen Wert im Display
    u8x8.print(displaymessagetext);
    u8x8.setCursor(0, 1);
    u8x8.print(displaymessagetext2);
  }
}

/********************************************************
    send data to display
******************************************************/
void printScreen() {
  if (Display == 1 && !sensorError) {
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
    if (bPID.GetKi() != 0){
    u8x8.print(bPID.GetKp() / bPID.GetKi());}
    else
    {u8x8.print("0");}
    u8x8.setCursor(11, 0);
    u8x8.print(",");
    u8x8.setCursor(12, 0);
    u8x8.print(bPID.GetKd() / bPID.GetKp());
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
    u8x8.print(convertOutputToUtilisation(Output));
  }
  
  if (Display == 2 && !sensorError) {
    display.clearDisplay();
    display.drawBitmap(2,2, logo_bits, logo_width, logo_height, WHITE);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(32, 15); 
    display.print("Ist :  ");
    display.print(Input, 1);
    display.print(" ");
    display.print((char)247);
    display.println("C");
    display.setCursor(32, 25); 
    display.print("Soll:  ");
    display.print(setPoint, 1);
    display.print(" ");
    display.print((char)247);
    display.println("C");

    //draw current temp in icon
    display.drawLine(11, 48, 11, 58 - (Input / 2), WHITE); 
    display.drawLine(12, 48, 12, 58 - (Input / 2), WHITE);  
    display.drawLine(13, 48, 13, 58 - (Input / 2), WHITE); 
    display.drawLine(14, 48, 14, 58 - (Input / 2), WHITE); 
    display.drawLine(15, 48, 15, 58 - (Input / 2), WHITE);
   
    //draw setPoint line
    display.drawLine(18, 58 - (setPoint / 2), 23, 58 - (setPoint / 2), WHITE);

    // Brew
    display.setCursor(32, 40); 
    display.print("Brew:  ");
    display.setTextSize(1);
    display.print(bezugsZeit / 1000);
    display.print("/");
    if (ONLYPID == 1){
      display.println(0, 0);             // deaktivieren wenn Preinfusion ( // voransetzen )
    }
    else 
    {
      display.println(totalbrewtime / 1000);            // aktivieren wenn Preinfusion
    }
    //draw box
    display.drawRoundRect(0, 0, 128, 64, 1, WHITE);    
    display.display();
  }
}
