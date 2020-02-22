/********************************************************
   Version 1.9.8h MASTER
  Key facts: major revision
  - Check the PIN Ports in the CODE!
  - Find your changerate of the machine, can be wrong, test it!
******************************************************/

#include "icon.h"
#include "RemoteDebug.h"  //https://github.com/JoaoLopesF/RemoteDebug


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

RemoteDebug Debug;

// Debug mode is active if #define DEBUGMODE is set
#define DEBUGMODE

#ifndef DEBUGMODE
#define DEBUG_println(a)
#define DEBUG_print(a)
#define DEBUGSTART(a)
#else
//#define DEBUG_println(a) rdebugDln("%s", a);
#define DEBUG_printFmt(fmt, ...) if (Debug.isActive(Debug.DEBUG))   Debug.printf("%0u " fmt, millis()/1000, ##__VA_ARGS__)
#define DEBUG_println(a) if (Debug.isActive(Debug.DEBUG))   Debug.printf("%0u %s\n", millis()/1000, a)
//#define DEBUG_println(a) Serial.println(a);
//#define DEBUG_printFmt(fmt, ...) if (Debug.isActive(Debug.DEBUG))   Debug.printf("%0u (%s)" fmt, millis()/1000, ##__VA_ARGS__)
#define DEBUG_print(a) if (Debug.isActive(Debug.DEBUG))   Debug.printf("%0u %s", millis()/1000, a)
//#define DEBUG_print(a) Serial.print(a);
#define DEBUGSTART(a) Serial.begin(a);
#endif

//#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG

//Define pins for outputs
#define pinRelayVentil    12
#define pinRelayPumpe     13
#define pinRelayHeater    14

const char* sysVersion PROGMEM  = "Version 1.9.8h Beta";

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
int pidON = 1 ;               // 1 = control loop in closed loop
int relayON, relayOFF;        // used for relay trigger type. Do not change!
int coldstartStep = 3;        // 1:= coldstart step1 detected (full heater power), 
                              // 2:= coldstart step2 (heater off, waiting for temperature to stabilize)
                              // 3:= coldstart is over, we are in regular PID mode (default)
boolean emergencyStop = false;// Notstop bei zu hoher Temperatur

/********************************************************
   moving average - Brüherkennung
*****************************************************/
const int numReadings = 75;             // number of values per Array
double readingstemp[numReadings];        // the readings from Temp
float readingstime[numReadings];        // the readings from time
float readingchangerate[numReadings];   // DiffTemp (based on readings) Per Second

int readIndex = 0;              // the index of the current reading
int totaltime = 0 ;             // the running time
double changerate = 0;          // local change rate of temprature
double heatrateaveragemin = 0 ;
unsigned long  timeBrewDetection = 0 ;
int timerBrewDetection = 0 ;
int i = 0;
#ifdef BREW_READY_DETECTION
float marginOfFluctuation = float(BREW_READY_DETECTION);
#else
float marginOfFluctuation = 0
#endif


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
double brewtimersoftware = 45;    // 20-5 for detection // after detecting a brew, wait at least this amount of seconds for detecting the next one.
double brewboarder = 2.0 ;        // border for the detection // if temperature decreased within the last 6 seconds below this threshold (in Celcius), then we detect a brew.
const int PonE = PONE;
// be carefull: to low: risk of wrong brew detection
// and rising temperature


/********************************************************
   Steady Power Temperature Controller
*****************************************************/
#include "VelocityControllerTypeC.h"

bool enableSteadyPowerTempController = true ; // TODO set default false + config + eeprom
double steadyPower = 49;  // TODO config + eeprom
double offsetPower = 0;
double burstPower = 500;
int burstShot = 0;

unsigned long previousMillistemp;  // initialisation at the end of init()
const long intervaltempmestsic = 400 ;
const long intervaltempmesds18b20 = 400  ;
int pidMode = 1; //1 = Automatic, 0 = Manual

const unsigned int windowSize = 1000;
unsigned int isrCounter = 0;  // counter for ISR
unsigned long windowStartTime;
double Input, Output, setPointTemp;  //
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

VelocityControllerTypeC bPID(&Input, &Output, &setPoint, aggKp, aggKi, aggKd, &Debug);

unsigned long outputSum_lastcorrection = 0;

/********************************************************
   Analog Schalter Read
******************************************************/
const int analogPin = 0; // will be use in case of hardware
int brewcounter = 0;
int brewswitch = 0;


long brewtime = 25000;
long aktuelleZeit = 0;
long totalbrewtime = 0;
int preinfusion = 2000;
int preinfusionpause = 5000;
unsigned long bezugsZeit = 0;
unsigned long startZeit = 0;

/********************************************************
   Sensor check
******************************************************/
boolean sensorError = false;
int error = 0;
int maxErrorCounter = 10 ;  //define maximum number of consecutive polls (of intervaltempmes* duration) to have errors
char debugline[100];

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
/*
   NOT KNOWN WHAT THIS IS USED FOR
  #include <TimeLib.h>
  #include <WidgetRTC.h>
  BlynkTimer timer;
  WidgetRTC rtc;
  WidgetBridge bridge1(V1);
*/

//Update Intervall zur App
unsigned long previousMillisBlynk;  // initialisation at the end of init()
const long intervalBlynk = 1000;
int blynksendcounter = 1;

//Update für Display
unsigned long previousMillisDisplay;  // initialisation at the end of init()
const long intervalDisplay = 500;

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
  aggTn = param.asDouble();  // TODO Beschriftung ändern im widget oder hier: I=>Tn
}
BLYNK_WRITE(V6) {
  aggTv =  param.asDouble(); // TODO Beschriftung ändern im widget oder hier: D=>Tv
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
  aggbTn = param.asDouble(); //BUG: either use PID values or change the blink widget description
}
BLYNK_WRITE(V32) {
  aggbTv =  param.asDouble(); //BUG: either use PID values or change the blink widget description
}
BLYNK_WRITE(V33) {
  brewtimersoftware =  param.asDouble();
}
BLYNK_WRITE(V34) {
  brewboarder =  param.asDouble();
}

BLYNK_WRITE(V40) {
  burstShot =  param.asInt();
}
BLYNK_WRITE(V41) {
  steadyPower =  param.asDouble();
}
BLYNK_WRITE(V42) {
  offsetPower =  param.asDouble();
}
BLYNK_WRITE(V43) {
  burstPower =  param.asDouble();
}

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

//void float2Bytes(float float_variable, byte bytes_temp[4]){ 
//  memcpy(bytes_temp, (unsigned char*) (&float_variable), 4);
//}

//void double2Bytes(double double_variable, byte bytes_temp[sizeof(double)]){
//  memcpy(bytes_temp, (unsigned char*) (&double_variable), sizeof(double));
//}

char* mqtt_build_topic(char* reading) {
  char* topic = (char *) malloc(sizeof(char) * 256);
  snprintf(topic, sizeof(topic), "%s%s/%s", mqtt_topic_prefix, hostname, reading);
  return topic;
}

boolean mqtt_publish(char* reading, char* payload) {
  char topic[MQTT_MAX_PUBLISH_SIZE];
  if (!mqtt_client.connected()) return false;
  snprintf(topic, MQTT_MAX_PUBLISH_SIZE, "%s%s/%s", mqtt_topic_prefix, hostname, reading);
  if (strlen(topic) + strlen(payload) >= MQTT_MAX_PUBLISH_SIZE) { //TODO test this code block later
    snprintf(debugline, sizeof(debugline), "WARN: mqtt_publish() wants to send to much data (len=%u)", strlen(topic) + strlen(payload));
    DEBUG_println(debugline);
    return false;
  } else {
    unsigned long currentMillis = millis();
    if (currentMillis > mqtt_dontPublishUntilTime) {
      boolean ret = mqtt_client.publish(topic, payload, mqtt_flag_retained);
      if (ret == false) { //TODO test this code block later (faking an error, eg millis <30000?)
        mqtt_dontPublishUntilTime = millis() + mqtt_dontPublishBackoffTime;
        snprintf(debugline, sizeof(debugline), "WARN: mqtt_publish() error on publish. Dont publish the next %ul milliseconds", mqtt_dontPublishBackoffTime);
        DEBUG_println(debugline);
      }
      return ret;
    } else { //TODO test this code block later (faking an error)
      snprintf(debugline, sizeof(debugline), "WARN: mqtt_publish() wont publish data (still for the next %ul milliseconds)", mqtt_dontPublishUntilTime - currentMillis);
      DEBUG_println(debugline);
      return false;
    }
  }
}

boolean mqtt_reconnect() {
  espClient.setTimeout(2000); // set timeout for mqtt connect()/write() to 2 seconds (default 5 seconds).
  if (mqtt_client.connect(hostname, mqtt_username, mqtt_password, topic_will, 0, 0, "unexpected exit")) {
    DEBUG_println("INFO: Connected to mqtt server");
    mqtt_publish("events", "INFO: Connected to mqtt server");
    mqtt_client.subscribe(topic_set);
  } else {
    snprintf(debugline, sizeof(debugline), "WARN: Cannot connect to mqtt server (consecutive failures=#%u)", mqtt_reconnectAttempts);
    DEBUG_println(debugline);
  }
  return mqtt_client.connected();
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  DEBUG_print("Message arrived [");
  DEBUG_print(topic);
  DEBUG_print("] ");
  for (int i = 0; i < length; i++) {
    DEBUG_print((char)payload[i]);
  }
  DEBUG_println(" ");
  // OPTIONAL TODO: business logic to activate rancilio functions from external (eg brewing, PidOn, startup, PID parameters,..)
}

/********************************************************
  Notstop wenn Temp zu hoch
*****************************************************/
void testEmergencyStop(){
  if (Input > 120){
    if (emergencyStop != true) {
      snprintf(debugline, sizeof(debugline), "ERROR: EmergencyStop because temperature>120 (temperature=%0.2f)", Input);
      DEBUG_println(debugline);
      mqtt_publish("events", debugline);
    }
    emergencyStop = true;
  } else if (Input < 100) {
    if (emergencyStop == true) {
      snprintf(debugline, sizeof(debugline), "ERROR: EmergencyStop ended because temperature<100 (temperature=%0.2f)", Input);
      DEBUG_println(debugline);
      mqtt_publish("events", debugline);
    }
    emergencyStop = false;
  }
}


/********************************************************
  Displayausgabe
*****************************************************/

void displaymessage(String displaymessagetext, String displaymessagetext2) {
  if (Display == 2) {
    /********************************************************
       DISPLAY AUSGABE
    ******************************************************/
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

//calculate the temperature difference between NOW and a datapoint in history. Returns temperature in celcius.
double pastTemperatureChange(int lookback) {
  double temperatureDiff;
  if (lookback > numReadings) lookback=numReadings -1;
  int offset = lookback % numReadings;
  int historicIndex = (readIndex - offset);
  if ( historicIndex < 0 ) {
    historicIndex += numReadings;
  }

  //ignore not yet initialized values
  if (readingstime[readIndex] == 0 || readingstime[historicIndex] == 0) return 0;

  if (brewboarder <= 30) {
    temperatureDiff = (readingstemp[readIndex] - readingstemp[historicIndex]);
  } else { // use previous factor on brewboarder threshold (compatibility to old brewboarder values using a factor of 100)
    temperatureDiff = (readingstemp[readIndex] - readingstemp[historicIndex]) * 100;
  }
  //compatibel code (can heatrateaveragemin be removed completely?)
  if (heatrateaveragemin > temperatureDiff) {
    heatrateaveragemin = temperatureDiff;
  }
  return temperatureDiff;
}

bool brewReady(double setPoint, float marginOfFluctuation, int lookback) {
  if (almostEqual(marginOfFluctuation, 0)) return false;
  if (lookback > numReadings) lookback=numReadings;
  for (int offset = 0; offset < lookback; offset++) {
    int thisReading = readIndex - offset;
    if (thisReading < 0) thisReading = numReadings + thisReading;
    if (readingstime[thisReading] == 0) return false;
    //snprintf(debugline, sizeof(debugline), "INFO: brewReady(): (cur_index=%d) index=%d: fluctuation=%0.2f (marginOfFluctuation=%0.2f)", readIndex, thisReading, fabs(setPoint - readingstemp[thisReading]), marginOfFluctuation);
    //DEBUG_println(debugline);
    if (fabs(setPoint - readingstemp[thisReading]) > (marginOfFluctuation + FLT_EPSILON)) return false;
  }
  return true;
}

/********************************************************
  check sensor value. If < 0 or difference between old and new >10, then increase error.
  If error is equal to maxErrorCounter, then set sensorError
*****************************************************/
boolean checkSensor(float tempInput) {
  boolean sensorOK = false;
  /********************************************************
    sensor error
  ******************************************************/
  if ( ( tempInput < 0 || tempInput > 150 || fabs(tempInput - previousInput) > 5) && !sensorError) {
    error++;
    sensorOK = false;
    snprintf(debugline, sizeof(debugline), "WARN: temperature sensor reading: consec_errors=%d, temp_current=%0.2f, temp_prev=%0.2f", error, tempInput, previousInput);
    DEBUG_println(debugline);
  } else if (tempInput > 0) {
    error = 0;
    sensorOK = true;
  }
  if (error >= maxErrorCounter && !sensorError) {
    sensorError = true ;
    snprintf(debugline, sizeof(debugline), "ERROR: temperature sensor malfunction: temp_current=%0.2f, temp_prev=%0.2f", tempInput, previousInput);
    DEBUG_println(debugline);
    mqtt_publish("events", debugline);
  } else if (error == 0 && TempSensorRecovery == 1) { //Safe-guard: prefer to stop heating forever if sensor is flapping!
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
  unsigned long millis_elapsed = currentMillistemp - previousMillistemp ;
  if (TempSensor == 1)
  {
    if ( floor(millis_elapsed / intervaltempmesds18b20) >= 2) {
      snprintf(debugline, sizeof(debugline), "WARN: Temporary main loop() hang. Number of temp polls missed=%g, millis_elapsed=%lu", floor(millis_elapsed / intervaltempmesds18b20) -1, millis_elapsed);
      DEBUG_println(debugline);
      mqtt_publish("events", debugline);
    }
    if (millis_elapsed >= intervaltempmesds18b20)
    {
      previousMillistemp = currentMillistemp;
      sensors.requestTemperatures();
      if (!checkSensor(sensors.getTempCByIndex(0))) return;  //if sensor data is not valid, abort function
      Input = sensors.getTempCByIndex(0);
      updateTemperatureHistory(Input);
    }
  }
  if (TempSensor == 2)
  {
    if ( floor(millis_elapsed / intervaltempmestsic) >= 2) {
      snprintf(debugline, sizeof(debugline), "WARN: Temporary main loop() hang. Number of temp polls missed=%g, millis_elapsed=%lu", floor(millis_elapsed / intervaltempmestsic) -1, millis_elapsed);
      DEBUG_println(debugline);
      mqtt_publish("events", debugline);
    }
    if (millis_elapsed >= intervaltempmestsic)
    {
      previousMillistemp = currentMillistemp;
      /*  variable "temperature" must be set to zero, before reading new data
            getTemperature only updates if data is valid, otherwise "temperature" will still hold old values
      */
      temperature = 0;
      Sensor1.getTemperature(&temperature);
      // TODO simulate what happens when temp >500 . It looks like Input is written despite if-ranges?!?! (pointer issue?)
      // temperature must be between 0x000 and 0x7FF(=DEC2047)
      Temperatur_C = Sensor1.calc_Celsius(&temperature);
      // Temperature_C must be -50C < Temperature_C <= 150C
      if (!checkSensor(Temperatur_C)) return;  //if sensor data is not valid, abort function
      Input = Temperatur_C;
      // Input = random(50,70) ;// test value
      updateTemperatureHistory(Input);
    }
  }
}

/********************************************************
    PreInfusion, Brew , if not Only PID
******************************************************/
void brew() {
  if (OnlyPID == 0) {
    brewswitch = analogRead(analogPin);
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
    u8x8.print(Output);
  }
  if (Display == 2 && !sensorError) {
    display.clearDisplay();
    display.drawBitmap(0,0, logo_bits,logo_width, logo_height, WHITE);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(32, 10); 
    display.print("Ist :  ");
    display.print(Input, 1);
    display.print(" ");
    display.print((char)247);
    display.println("C");
    display.setCursor(32, 20); 
    display.print("Soll:  ");
    display.print(setPoint, 1);
    display.print(" ");
    display.print((char)247);
    display.println("C");
   // display.print("Heizen: ");
    
   // display.println(" %");
   
// Draw heat bar
   display.drawLine(15, 58, 117, 58, WHITE);
   display.drawLine(15, 58, 15, 61, WHITE); 
   display.drawLine(117, 58, 117, 61, WHITE);

   display.drawLine(16, 59, (Output / 10) + 16, 59, WHITE);
   display.drawLine(16, 60, (Output / 10) + 16, 60, WHITE);
   display.drawLine(15, 61, 117, 61, WHITE);
   
//draw current temp in icon
   display.drawLine(9, 48, 9, 58 - (Input / 2), WHITE); 
   display.drawLine(10, 48, 10, 58 - (Input / 2), WHITE);  
   display.drawLine(11, 48, 11, 58 - (Input / 2), WHITE); 
   display.drawLine(12, 48, 12, 58 - (Input / 2), WHITE); 
   display.drawLine(13, 48, 13, 58 - (Input / 2), WHITE);
   
//draw setPoint line
   display.drawLine(18, 58 - (setPoint / 2), 23, 58 - (setPoint / 2), WHITE); 
 
// PID Werte ueber heatbar
    display.setCursor(40, 50);  

    display.print(bPID.GetKp(), 0); // P 
    display.print("|");
    if (bPID.GetKi() != 0){      
      display.print(bPID.GetKp() / bPID.GetKi(), 0);; // I
    }
    else
    { 
      display.print("0");
    }
    display.print("|");
    display.println(bPID.GetKd() / bPID.GetKp(), 0); // D
    display.setCursor(98,50);
    display.print(Output / 10, 0);
    display.print("%");

// Brew
    display.setCursor(32, 31); 
    display.print("Brew:  ");
    display.setTextSize(1);
    display.print(bezugsZeit / 1000);
    display.print("/");
    if (ONLYPID == 1){
      display.println(brewtimersoftware, 0);             // deaktivieren wenn Preinfusion ( // voransetzen )
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

/********************************************************
  send data to Blynk server
*****************************************************/

void sendToBlynk() {
  if (Offlinemodus != 0) return;
  unsigned long currentMillisBlynk = millis();
  if (currentMillisBlynk - previousMillisBlynk >= intervalBlynk) {
    previousMillisBlynk += intervalBlynk;  // FIX race
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
        Blynk.virtualWrite(V35, pastTemperatureChange(30));
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
    coldstart Step Detection
******************************************************/
void coldstartStepDetection() {
  const int coldstartStep1_minTempToActivate = 5;
  switch (coldstartStep) {
    case 1: // coldstart step 1 running, that means full heater power. Check if target temp is reached
    {
      if (Input >= starttemp) {
        snprintf(debugline, sizeof(debugline), "INFO: Coldstart transition to step 2 (heater off)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        coldstartStep = 2;
      }
      break;
    }
    case 2: // coldstart step 1 running, that means heater is off and we are waiting to temperature to stabilize.
    {
      float tempChange = pastTemperatureChange(30); // 12 secs
      if ( (Input - setPoint >= 0) || (Input - setPoint <= -20) || (Input - setPoint <= 0  && tempChange <= 0)) {
        snprintf(debugline, sizeof(debugline), "INFO: Coldstart transition to step 3 (regular PID controlled)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        
        //double outputSum = 49; // set to steady value . TODO remove hard-coded
        //bPID.SetOutputLimits((outputSum), outputSum+1); //hack to set bPid.outputSum
        //bPID.SetOutputLimits(outputSum, windowSize);
        coldstartStep = 3;
      }
      break;
    }
    case 3: // regular mode, check if we need to go into coldstart step 1
    default:
    {
      if (Input < starttemp - coldstartStep1_minTempToActivate) {
        snprintf(debugline, sizeof(debugline), "INFO: Coldstart transition to step 1 (heater full power)");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
        coldstartStep = 1;
      }
      break;
    }
  }
}
/********************************************************
    brewDetection
******************************************************/
void brewRunDetection() {
  if (brewboarder == 0) return; //abort brewDetection if deactivated

  // Brew detecion == 1 software solution , == 2 hardware
  if (brewDetection == 1 || brewDetection == 2) {
    //disable brew-detection after brewtimersoftware seconds
    if (millis() - timeBrewDetection >= brewtimersoftware * 1000) {
      if (timerBrewDetection == 1) {
        snprintf(debugline, sizeof(debugline), "INFO: Brew detection is over. Reverting to regular pid values.");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
      }
      timerBrewDetection = 0 ;
        if (OnlyPID == 1) {
          bezugsZeit = 0 ;
        }
    }
  }

  if (brewDetection == 1) {
    //enable bew-detection if not already running and diff temp is > brewboarder
    if (pastTemperatureChange(15) <= -brewboarder && timerBrewDetection == 0 ) {
      timeBrewDetection = millis() ;
      timerBrewDetection = 1 ;
      mqtt_publish("brewDetected", "1");
      snprintf(debugline, sizeof(debugline), "INFO: Brew detected. Setting brew pid values for %0.0f seconds", brewtimersoftware);
      DEBUG_println(debugline);
      mqtt_publish("events", debugline);
    }
  }
}


void ICACHE_RAM_ATTR onTimer1ISR() {
  timer1_write(50000); // set interrupt time to 10ms

  //run PID calculation
  if ( bPID.Compute() ) {
    isrCounter = 0;  // Attention: heater might not shutdown if bPid.SetSampleTime(), windowSize, timer1_write() and are not set correctly!
    //if (burstShot == 1) {
    //  burstShot = 0;
    //  Output = burstPower;
    //  snprintf(debugline, sizeof(debugline), "INFO: bPID.Compute(): BURST Output=%0.2f, InputTemp=%0.2f, DiffTemp=%0.2f, isrCounter=%u", Output, Input, (setPoint - Input), isrCounter);
    //  DEBUG_println(debugline);
    //} else {
    snprintf(debugline, sizeof(debugline), "INFO: bPID.Compute(): Output=%0.2f, InputTemp=%0.2f, DiffTemp=%0.2f, isrCounter=%u", Output, Input, (setPoint - Input), isrCounter);
    DEBUG_println(debugline);
    //}
  }

  if ( true ) {  // TODO TOBIAS READD
  if (Output <= isrCounter) {
    digitalWrite(pinRelayHeater, LOW);
    //DEBUG_println("Power off!");
  } else {
    digitalWrite(pinRelayHeater, HIGH);
    //DEBUG_println("Power on!");
  }
  }
  
  //increase counter until fail-safe is reached
  if (isrCounter <= windowSize) {
    isrCounter += 10; // += 10 because one tick = 10ms
  }
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
  displaymessage(sysVersion, "");
  delay(2000);

  /********************************************************
     BLYNK & Fallback offline
  ******************************************************/
  if (Offlinemodus == 0) {

    WiFi.hostname(hostname);
    if (fallback == 0) {

      displaymessage("Connect to Blynk", "no Fallback");
      Blynk.begin(auth, ssid, pass, blynkaddress, blynkport);
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
        DEBUG_println(WiFi.localIP().toString().c_str());
        Debug.begin(hostname, Debug.DEBUG);
        Debug.setResetCmdEnabled(true); // Enable the reset command
        Debug.showProfiler(true); // Profiler (Good to measure times, to optimize codes)
        Debug.showColors(true); // Colors
        // Debug.setSerialEnabled(true); // log to Serial also

        displaymessage("2: Wifi connected, ", "try Blynk   ");
        DEBUG_println("Wifi works, now try Blynk connection");
        delay(2000);
        Blynk.config(auth, blynkaddress, blynkport) ;
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
          Blynk.syncVirtual(V40);
          Blynk.syncVirtual(V41);
          Blynk.syncVirtual(V42);
          Blynk.syncVirtual(V43);
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
        //TODO: Show this state somehow. eg blinking LED?
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
    do {
      delay(100);
      sensors.requestTemperatures();
      Input = sensors.getTempCByIndex(0);
    } while (!checkSensor(Input));
  }

  if (TempSensor == 2) {
    temperature = 0;
    Sensor1.getTemperature(&temperature);
    Input = Sensor1.calc_Celsius(&temperature);
    //do {
    //  delay(100);
    //  temperature = 0;
    //  Sensor1.getTemperature(&temperature);
    //  Input = Sensor1.calc_Celsius(&temperature);
    //} while (!checkSensor(Input));
  }   

  /********************************************************
    movingaverage ini array
  ******************************************************/
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readingstemp[thisReading] = 0;
    readingstime[thisReading] = 0;
    readingchangerate[thisReading] = 0;
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

  ArduinoOTA.handle();  // For OTA
  // Disable interrupt it OTA is starting, otherwise it will not work
  ArduinoOTA.onStart([](){
    timer1_disable();
    digitalWrite(pinRelayHeater, LOW); //Stop heating
    snprintf(debugline, sizeof(debugline), "INFO: OTA update initiated");
    DEBUG_println(debugline);
    mqtt_publish("events", debugline);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
    snprintf(debugline, sizeof(debugline), "INFO: OTA update error");
    DEBUG_println(debugline);
    mqtt_publish("events", debugline);
  });
  // Enable interrupts if OTA is finished
  ArduinoOTA.onEnd([](){
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  });
 
  if (WiFi.status() == WL_CONNECTED){
 
    Blynk.run(); //Do Blynk magic stuff
    wifiReconnects = 0;
    //Check mqtt connection
    if (!mqtt_client.connected()) {
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
    } else {
      // mqtt client connected, do mqtt housekeeping
      mqtt_client.loop();
      unsigned long now = millis();
      if (now - lastMQTTStatusReportTime >= lastMQTTStatusReportInterval) {
        lastMQTTStatusReportTime = now;
        mqtt_publish("temperature", number2string(Input));
        mqtt_publish("temperatureAboveTarget", number2string((Input - setPoint)));
        mqtt_publish("heaterUtilization", number2string(100*Output/windowSize));
        mqtt_publish("kp", number2string(bPID.GetKp()));
        mqtt_publish("ki", number2string(bPID.GetKi()));
        mqtt_publish("kd", number2string(bPID.GetKd()));
        mqtt_publish("pastTemperatureChange", number2string(pastTemperatureChange(30)));
        mqtt_publish("brewReady", bool2string(brewReady(setPoint, marginOfFluctuation, numReadings)));
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

  //check if PID should run or not. If not, set to manuel and force output to zero
  if (pidON == 0 && pidMode == 1) {
    pidMode = 0;
    bPID.SetMode(pidMode);
    Output = 0 ;
  } else if (pidON == 1 && pidMode == 0) {
    Output = 0; // safety: be 100% sure that PID.compute() starts fresh.
    pidMode = 1;
    bPID.SetMode(pidMode);
  }
  if (burstShot == 1 && pidMode == 1) {
    burstShot = 0;
    Output = burstPower ;
    snprintf(debugline, sizeof(debugline), "INFO: BURST Output=%0.2f", Output);
    DEBUG_println(debugline);
    mqtt_publish("events", debugline);
  }

  //Sicherheitsabfrage
  if (!sensorError && Input > 0 && !emergencyStop) {

    brewRunDetection();
    coldstartStepDetection();

    //coldstartStep 1: Water is very cold, set heater to full power
    if (coldstartStep == 1) {
      if ( enableSteadyPowerTempController ) {
        Output = 1000;
      } else {
        if (startTn != 0) {
          startKi = startKp / startTn;
        } else {
          startKi = 0;
        }
        bPID.SetTunings(startKp, startKi, 0); // TODO BUG: setTunings() greift erst wenn startTn != 0 ist?? float comparison < 0 failed.
      }

    //coldstartStep 2: ColdstartTemp reached. Now steadily increasing temperature.
    } else if (coldstartStep == 2) {
        // bPID.SetTunings(0, 0, 0);
        double outputSum = steadyPower; // set to steady value . TODO remove hard-coded
        bPID.SetOutputLimits(steadyPower*1, steadyPower*1+1);
        bPID.SetOutputLimits(steadyPower*1, windowSize);
        

    //if brew is detected, set brew specific PID values
    } else if ( coldstartStep == 3 && timerBrewDetection == 1 && (millis() - timeBrewDetection  < brewtimersoftware * 1000)) {
      // calc ki, kd
      if (aggbTn != 0) {
        aggbKi = aggbKp / aggbTn ;
      } else {
        aggbKi = 0;
      }
      aggbKd = aggbTv * aggbKp ;
      if ( not almostEqual(bPID.GetKp(), aggbKp) || not almostEqual(bPID.GetKi(), aggbKi) || not almostEqual(bPID.GetKd(), aggbKd) ) {
        snprintf(debugline, sizeof(debugline), "INFO: Brew detected. Setting brew pid values.");
        DEBUG_println(debugline);
        mqtt_publish("events", debugline);
      }
      bPID.SetTunings(aggbKp, aggbKi, aggbKd) ;
      if (OnlyPID == 1){
        bezugsZeit = millis() - timeBrewDetection ;
      }

    //coldstartStep 3: set regular pid values in "normal" mode
    } else {
      // calc ki, kd
      if (aggTn != 0) {
        aggKi = aggKp / aggTn ;
      } else {
        aggKi = 0 ;
      }
      aggKd = aggTv * aggKp ;

      // TOBIAS TODO "Implement more phases": Add minimum output to 400 when near-below setpoint. Dont set when too high or in another phase??
      // Idea against high peaks: Check every 10sec the Diff-Temp(20sec): If Diff-Temp is >=0.5, do
      // 1) if setPoint is breached in 40secs time, then reduce Output by 50% (but not not below stable).
      // 2) if setPoint is breached in 20sec time, then reduce Output to "stable".
      // Idea: Remember past outputs and Temperatures:
      // 1) If Temp decreases and past output 
      //
      // Idea: Compensate for "colder" brewhead by having setpoint increased?! test water temp.
      // Push temp 3C above settemp. Coldstartphase2 ==> autotune phase (previous saved stablepower (eeprom) as reference).
      // autotune phase: If temp is decreasing in the last 40sec, then increase stablepower by 10%. Else decrease 10% and repeat. I stable then safe value.
      // let temp slowly drop to setpoint by -5% and set back to steadypower when reached.

      //TODO IMPORTANT: mqtt_publish must transfer all temp values with 3 precision to reduce rounding errors!
      if ( false &&  millis() - outputSum_lastcorrection > 20 * 1000 ) {

        double outputSum = 49; //TODO remove hard-coded . Wenn es kalt ist dann brauche ich ~90
        
        if ( Input - setPoint >= 2 ) {
          outputSum_lastcorrection = millis();
          snprintf(debugline, sizeof(debugline), "INFO: Force outputSum to low value (too high above setpoint)");
          DEBUG_println(debugline);
          mqtt_publish("events", debugline);
          bPID.SetOutputLimits((outputSum*0.5), outputSum*0.5+1); //hack to set bPid.outputSum
          bPID.SetOutputLimits(0, windowSize);
        }
        
        //manipulate PID controller to reduce oscillating temperature (Tobias: this safes around 60 seconds on 1 oscillation)
        //set outputSum to a minimum "stable" value when we are traversing downwards past setpoint
        if ( (Output < outputSum * 1.1) && (pastTemperatureChange(30) <= -0.1 ) && (Input - setPoint >= 0.2 && Input - setPoint < 0.4 ) ) {
          outputSum_lastcorrection = millis();
          snprintf(debugline, sizeof(debugline), "INFO: Force outputSum to stable value (on traversing downwards past setpoint)");
          DEBUG_println(debugline);
          mqtt_publish("events", debugline);
          bPID.SetOutputLimits((outputSum*1.1), outputSum*1.1+1); //hack to set bPid.outputSum
          bPID.SetOutputLimits(outputSum*1.1, windowSize);  //same value as in coldstartstep transition from 2->3.
        }

        if ( true ) {
          //manipulate PID controller to reduce oscillating temperature (Tobias: this safes around 25 seconds on 1 oscillation)
          //reduce too high output on traversing upwards past setpoint
          //double outputSum = 49; //TODO remove hard-coded

          if ( (Output > outputSum * 1.5) && (pastTemperatureChange(30) > 0.2 ) && (Input - setPoint <= -0.5 && Input - setPoint >= -4.0) ) {
            outputSum_lastcorrection = millis();
            snprintf(debugline, sizeof(debugline), "INFO: Force outputSum to stable value (on traversing upwards past setpoint - rough)");
            DEBUG_println(debugline);
            mqtt_publish("events", debugline);
            bPID.SetOutputLimits((outputSum*0.6), outputSum*0.6+1); //hack to set bPid.outputSum
            bPID.SetOutputLimits(outputSum*0.6, windowSize);
          }
          
          if ( (Output > outputSum * 1) && (pastTemperatureChange(30) > 0.1 ) && (Input - setPoint <= 0 && Input - setPoint > -0.3) ) {
            outputSum_lastcorrection = millis();
            snprintf(debugline, sizeof(debugline), "INFO: Force outputSum to stable value (on traversing upwards past setpoint - fine)");
            DEBUG_println(debugline);
            mqtt_publish("events", debugline);
            bPID.SetOutputLimits((outputSum*0.9), outputSum*0.9+1); //hack to set bPid.outputSum
            bPID.SetOutputLimits(outputSum*0.9, windowSize);
          }

          if ( (Output < outputSum * 1) && (pastTemperatureChange(30) <= 0.1 && pastTemperatureChange(30) >= -0.1) && (Input - setPoint <= 0.1 && Input - setPoint >= -0.1) ) {
            outputSum_lastcorrection = millis();
            snprintf(debugline, sizeof(debugline), "INFO: Force outputSum to stable value (stable point)");
            DEBUG_println(debugline);
            mqtt_publish("events", debugline);
            //dont do this: bPID.SetOutputLimits((outputSum*1.1), outputSum*1.1+1); //hack to set bPid.outputSum
            bPID.SetOutputLimits(outputSum*1.0, windowSize);
          }
        }
      }
      
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
    if (Display == 1) {
      u8x8.setFont(u8x8_font_chroma48medium8_r);  //Ausgabe vom aktuellen Wert im Display
      u8x8.setCursor(0, 0);
      u8x8.print("               ");
      u8x8.setCursor(0, 1);
      u8x8.print("               ");
      u8x8.setCursor(0, 2);
      u8x8.print("               ");
      u8x8.setCursor(0, 1);
      u8x8.print("Error: Temp = ");
      u8x8.setCursor(0, 2);
      u8x8.print(Input);
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
    if (Display == 1) {
      u8x8.setFont(u8x8_font_chroma48medium8_r);  //Ausgabe vom aktuellen Wert im Display
      u8x8.setCursor(0, 0);
      u8x8.print("               ");
      u8x8.setCursor(0, 1);
      u8x8.print("               ");
      u8x8.setCursor(0, 2);
      u8x8.print("               ");
      u8x8.setCursor(0, 1);
      u8x8.print("Emergency Stop! T>120");
      u8x8.setCursor(0, 2);
      u8x8.print(Input);
    }
  }

  Debug.handle();
}
