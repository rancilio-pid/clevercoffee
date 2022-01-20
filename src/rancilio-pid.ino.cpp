# 1 "/var/folders/m6/d944mmrx3vxf_b6prf8xm3h40000gn/T/tmp8t_9uj2d"
#include <Arduino.h>
# 1 "/Users/Andreas/Documents/GitHub/ranciliopid/src/rancilio-pid.ino"





#define SYSVERSION_INO '3.0.0 ALPHA'
#define SYSVERSION_DISPLAY "Version 3.0.0 ALPHA"




#include <ArduinoOTA.h>
#include "userConfig.h"
#include <U8g2lib.h>
#include "PID_v1.h"
#include "languages.h"
#include <DallasTemperature.h>
#if defined(ESP8266)
  #include <BlynkSimpleEsp8266.h>
#endif
#if defined(ESP32)
  #include <BlynkSimpleEsp32.h>
  #include <os.h>
  hw_timer_t * timer = NULL;
#endif
#include "icon.h"
#include <ZACwire.h>
#include <PubSubClient.h>
#include "TSIC.h"
#include <Adafruit_VL53L0X.h>
#include "Storage.h"

#if (BREWMODE == 2 || ONLYPIDSCALE == 1)
#include <HX711_ADC.h>
#endif




#if !defined(SYSVERSION) || !defined(SYSVERSION_INO) || (SYSVERSION != SYSVERSION_INO)
  #error Version of userConfig file and rancilio-pid.ino need to match!
#endif




MACHINE machine = (enum MACHINE) MACHINEID;

#define HIGH_ACCURACY 

#include "PeriodicTrigger.h"
PeriodicTrigger writeDebugTrigger(5000);
PeriodicTrigger logbrew(500);





enum MachineState {
    kInit = 0,
    kColdStart = 10,
    kSetPointNegative = 19,
    kPidNormal = 20,
    kBrew = 30,
    kShotTimerAfterBrew = 31,
    kBrewDetectionTrailing = 35,
    kSteam = 40,
    kCoolDown = 45,
    kBackflush = 50,
    kEmergencyStop = 80,
    kPidOffline = 90,
    kSensorError = 100,
    keepromError = 110,
};
MachineState machinestate = kInit;
int machinestatecold = 0;
unsigned long machinestatecoldmillis = 0;
MachineState lastmachinestate = kInit;
int lastmachinestatepid = -1;




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

const unsigned long brewswitchDelay = BREWSWITCHDELAY;
int BrewMode = BREWMODE;


uint8_t oled_i2c = OLED_I2C;


Adafruit_VL53L0X lox = Adafruit_VL53L0X();
int calibration_mode = CALIBRATION_MODE;
uint8_t tof_i2c = TOF_I2C;
int water_full = WATER_FULL;
int water_empty = WATER_EMPTY;
unsigned long previousMillisTOF;
const unsigned long intervalTOF = 5000 ;
double distance;
double percentage;


const char* hostname = HOSTNAME;
const char* auth = AUTH;
const char* ssid = D_SSID;
const char* pass = PASS;
unsigned long lastWifiConnectionAttempt = millis();
unsigned int wifiReconnects = 0;

uint8_t softApEnabled = 0 ;
IPAddress localIp(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
const char* AP_WIFI_SSID = APWIFISSID ;
const char* AP_WIFI_KEY = APWIFIKEY ;
const unsigned long checkpowerofftime = 30*1000 ;
boolean checklastpoweroffEnabled = false;
boolean softApEnabledcheck = false ;
int softApstate = 0;



const char* OTAhost = OTAHOST;
const char* OTApass = OTAPASS;


const char* blynkaddress = BLYNKADDRESS;
const int blynkport = BLYNKPORT;
unsigned int blynkReCnctFlag;
unsigned int blynkReCnctCount = 0;
unsigned long lastBlynkConnectionAttempt = millis();


const unsigned long fillTime = FILLTIME;
const unsigned long flushTime = FLUSHTIME;
int maxflushCycles = MAXFLUSHCYCLES;


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
unsigned int MQTTReCnctFlag;
unsigned int MQTTReCnctCount = 0;


#include <InfluxDbClient.h>
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);
Point sensor("machinestate");
unsigned long previousMillisInflux;
const unsigned long intervalInflux = INTERVALINFLUX;


unsigned long previousMillisVoltagesensorreading = millis();
const unsigned long intervalVoltagesensor= 200 ;
int VoltageSensorON, VoltageSensorOFF;


const int maxBrewDurationForSteamModeQM_ON = 200;
const int minPVSOffTimedForSteamModeQM_OFF = 1500;
unsigned long timePVStoON = 0;
unsigned long lastTimePVSwasON = 0;
bool steamQM_active = false;
bool brewSteamDetectedQM = false;
bool coolingFlushDetectedQM = false;


#if (PRESSURESENSOR == 1)
int offset = OFFSET;
int fullScale = FULLSCALE;
int maxPressure = MAXPRESSURE;
float inputPressure = 0;
const unsigned long intervalPressure = 200;
unsigned long previousMillisPressure;
#endif





int pidON = 1 ;
int relayON, relayOFF;
boolean kaltstart = true;
boolean emergencyStop = false;
double EmergencyStopTemp = 120;
const char* sysVersion PROGMEM = "Version 3.0.0 ALPHA";
int inX = 0, inY = 0, inOld = 0, inSum = 0;
int bars = 0;
boolean brewDetected = 0;
boolean setupDone = false;
int backflushON = 0;
int flushCycles = 0;
int backflushState = 10;




const int numReadings = 15;
double readingstemp[numReadings];
unsigned long readingstime[numReadings];
double readingchangerate[numReadings];

int readIndex = 1;
double total = 0;
double heatrateaverage = 0;
double changerate = 0;
double heatrateaveragemin = 0 ;
unsigned long timeBrewdetection = 0 ;
int timerBrewdetection = 0 ;
int firstreading = 1 ;




double aggbKp = AGGBKP;
double aggbTn = AGGBTN;
double aggbTv = AGGBTV;
#if aggbTn == 0
double aggbKi = 0;
#else
double aggbKi = aggbKp / aggbTn;
#endif
double aggbKd = aggbTv * aggbKp ;
double brewtimersoftware = 45;
double brewboarder = BREWDETECTIONLIMIT;
const int PonE = PONE;





  #include "brewscaleini.h"




boolean sensorError = false;
int error = 0;
int maxErrorCounter = 10 ;




unsigned long previousMillistemp;
const unsigned long intervaltempmestsic = 400 ;
const unsigned long intervaltempmesds18b20 = 400 ;
int pidMode = 1;

const unsigned int windowSize = 1000;
unsigned int isrCounter = 0;
unsigned long windowStartTime;
double Input, Output;
double setPointTemp;
double previousInput = 0;

double BrewSetPoint = SETPOINT;
double setPoint = BrewSetPoint;
double SteamSetPoint = STEAMSETPOINT;
int SteamON = 0;
int SteamFirstON = 0;
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

PID bPID(&Input, &Output, &setPoint, aggKp, aggKi, aggKd, PonE, DIRECT) ;




OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress sensorDeviceAddress;




uint16_t temperature = 0;
float Temperature_C = 0;

#if (ONE_WIRE_BUS == 16 && TEMPSENSOR == 2 && defined(ESP8266))
TSIC Sensor1(ONE_WIRE_BUS);
#else
ZACwire Sensor2(ONE_WIRE_BUS, 306);
#endif




unsigned long previousMillisBlynk;
unsigned long previousMillisMQTT;
const unsigned long intervalBlynk = 1000;
const unsigned long intervalMQTT = 5000;
int blynksendcounter = 1;




#include "RancilioServer.h"


std::vector<editable_t> editableVars = {
    {"PID_ON", "PID on?", kInteger, (void *)&pidON},
    {"PID_KP", "PID P", kDouble, (void *)&aggKp},
    {"PID_TN", "PID I", kDouble, (void *)&aggTn},
    {"PID_TV", "PID D", kDouble, (void *)&aggTv},
    {"TEMP", "Temperature", kDouble, (void *)&Input},
    {"BREW_SET_POINT", "Set point", kDouble, (void *)&BrewSetPoint},
    {"BREW_TIME", "Brew Time s", kDouble, (void *)&brewtime},
    {"BREW_PREINFUSION", "Preinfusion Time s", kDouble, (void *)&preinfusion},
    {"BREW_PREINFUSUINPAUSE", "Pause s", kDouble, (void *)&preinfusionpause},
    {"PID_BD_KP", "BD P", kDouble, (void *)&aggbKp},
    {"PID_BD_TN", "BD I", kDouble, (void *)&aggbTn},
    {"PID_BD_TV", "BD D", kDouble, (void *)&aggbTv},
    {"PID_BD_TIMER", "PID BD Time s", kDouble, (void *)&brewtimersoftware},
    {"PID_BD_BREWBOARDER", "PID BD Sensitivity", kDouble, (void *)&brewboarder},
    {"AP_WIFI_SSID", "AP WiFi Name", kCString, (void *)AP_WIFI_SSID},
    {"AP_WIFI_KEY", "AP WiFi Password", kCString, (void *)AP_WIFI_KEY},
    {"START_KP", "Start P", kDouble, (void *)&startKp},
    {"START_TN", "Start I", kDouble, (void *)&startTn},
    {"STEAM_MODE", "STEAM MODE", rInteger, (void *)&SteamON}
};

unsigned long lastTempEvent = 0;
unsigned long tempEventInterval = 1000;
void getSignalStrength();
void createSoftAp();
void stopSoftAp();
void checklastpoweroff();
void setchecklastpoweroff();
void checkPressure();
void testEmergencyStop();
void movAvg();
boolean checkSensor(float tempInput);
void refreshTemp();
void initOfflineMode();
void checkWifi();
void sendInflux();
void checkBlynk();
void checkMQTT();
char* number2string(double in);
char* number2string(float in);
char* number2string(int in);
char* number2string(unsigned int in);
bool mqtt_publish(const char *reading, char *payload);
void sendToBlynkMQTT();
void brewdetection();
int filter(int input);
void mqtt_callback(char* topic, byte* data, unsigned int length);
void ETriggervoid();
void checkSteamON();
void setEmergencyStopTemp();
void initSteamQM();
boolean checkSteamOffQM();
void machinestatevoid();
void debugVerboseOutput();
void ledtemp();
void setup();
void loop();
void loopcalibrate();
void looppid();
int readSysParamsFromStorage(void);
int setSteammode(void);
int writeSysParamsToStorage(void);
int writeSysParamsToBlynk(void);
#line 360 "/Users/Andreas/Documents/GitHub/ranciliopid/src/rancilio-pid.ino"
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
  } else if (rssi < -50 && rssi >= -65) {
    bars = 3;
  } else if (rssi < -65 && rssi >= -75) {
    bars = 2;
  } else if (rssi < -75 && rssi >= -80) {
    bars = 1;
  } else {
    bars = 0;
  }
}





 #include "ISR.h"







#if DISPLAY == 1
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
#endif
#if DISPLAY == 2
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
#endif

unsigned long previousMillisDisplay;
const unsigned long intervalDisplay = 500;


#if (DISPLAY == 1 || DISPLAY == 2)
  #if (DISPLAYTEMPLATE < 20)
    #include "display.h"
  #endif
  #if (DISPLAYTEMPLATE >= 20)
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
  #if (DISPLAYTEMPLATE == 4)
      #include "Displaytemplatescale.h"
  #endif
  #if (DISPLAYTEMPLATE == 20)
      #include "Displaytemplateupright.h"
  #endif
#endif





void createSoftAp()
{
 if (softApEnabledcheck == false)
 {
      WiFi.enableAP(true);
      WiFi.softAP(AP_WIFI_SSID, AP_WIFI_KEY);
      WiFi.softAPConfig(localIp, gateway, subnet);


      softApEnabledcheck = true;
      Serial.println("Set softApEnabled: 1, AP MODE\n");

      uint8_t eepromvalue = 0;
      storageSet(STO_ITEM_SOFT_AP_ENABLED_CHECK, eepromvalue, true) ;
      softApstate = 0;
      Serial.printf("AccessPoint created with SSID %s and KEY %s and OTA Flash via http://%i.%i.%i.%i/\r\n", AP_WIFI_SSID, AP_WIFI_KEY, WiFi.softAPIP()[0],WiFi.softAPIP()[1],WiFi.softAPIP()[2],WiFi.softAPIP()[3]);

      #if (DISPLAY != 0)
        displayMessage("AP-MODE: SSID:", String(AP_WIFI_SSID), "KEY:", String(AP_WIFI_KEY), "IP:","192.168.1.1");
      #endif
 }

 yield();
}

void stopSoftAp()
{
    Serial.println("Closing AccesPoint");



    WiFi.enableAP(false);
}

void checklastpoweroff()
{
  storageGet(STO_ITEM_SOFT_AP_ENABLED_CHECK, softApEnabled);

  Serial.printf("softApEnabled: %i\n",softApEnabled);


 if (softApEnabled != 1)
 {
  Serial.printf("Set softApEnabled: 1, was 0\n");
  uint8_t eepromvalue = 1;
  storageSet(STO_ITEM_SOFT_AP_ENABLED_CHECK, eepromvalue) ;
 }

 storageCommit();
}

void setchecklastpoweroff()
{
  if (millis() > checkpowerofftime && checklastpoweroffEnabled == false)
  {
    disableTimer1();
    Serial.printf("Set softApEnabled 0 after checkpowerofftime\n");
    uint8_t eepromvalue = 0;
    storageSet(STO_ITEM_SOFT_AP_ENABLED_CHECK, eepromvalue, true) ;
    checklastpoweroffEnabled = true;
    enableTimer1();
  }
}




BLYNK_CONNECTED() {
  if (Offlinemodus == 0 && BLYNK == 1) {
    Blynk.syncAll();

  }
}

BLYNK_WRITE(V4) {
  aggKp = param.asDouble();
}

BLYNK_WRITE(V5) {
  aggTn = param.asDouble();
}
BLYNK_WRITE(V6) {
  aggTv = param.asDouble();
}

BLYNK_WRITE(V7) {
  BrewSetPoint = param.asDouble();
  mqtt_publish("BrewSetPoint", number2string(BrewSetPoint));
}

BLYNK_WRITE(V8) {
  brewtime = param.asDouble();
  mqtt_publish("brewtime", number2string(brewtime));
}

BLYNK_WRITE(V9) {
  preinfusion = param.asDouble();
  mqtt_publish("preinfusion", number2string(preinfusion));
}

BLYNK_WRITE(V10) {
  preinfusionpause = param.asDouble();
  mqtt_publish("preinfusionpause", number2string(preinfusionpause));
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
#if (BREWMODE == 2)
BLYNK_WRITE(V18)
{
  weightSetpoint = param.asFloat();
}
#endif
BLYNK_WRITE(V25)
{
  calibration_mode = param.asInt();
}
BLYNK_WRITE(V26)
{
  water_empty = param.asInt();
}
BLYNK_WRITE(V27)
{
  water_full = param.asInt();
}

BLYNK_WRITE(V30)
{
  aggbKp = param.asDouble();
}

BLYNK_WRITE(V31) {
  aggbTn = param.asDouble();
}
BLYNK_WRITE(V32) {
  aggbTv = param.asDouble();
}
BLYNK_WRITE(V33) {
  brewtimersoftware = param.asDouble();
}
BLYNK_WRITE(V34) {
  brewboarder = param.asDouble();
}
BLYNK_WRITE(V40) {
  backflushON = param.asInt();
}

#if (COLDSTART_PID == 2)
  BLYNK_WRITE(V11)
    {
    startKp = param.asDouble();
    }
  BLYNK_WRITE(V14)
    {
      startTn = param.asDouble();
    }
 #endif


#if (PRESSURESENSOR == 1)





void checkPressure() {
  float inputPressureFilter = 0;
  unsigned long currentMillisPressure = millis();

  if (currentMillisPressure - previousMillisPressure >= intervalPressure)
  {
    previousMillisPressure = currentMillisPressure;

    inputPressure = ((analogRead(PINPRESSURESENSOR) - offset) * maxPressure * 0.0689476) / (fullScale - offset);
    inputPressureFilter = filter(inputPressure);

    Serial.printf("pressure raw: %f\n", inputPressure);
    Serial.printf("pressure filtered: %f\n", inputPressureFilter);
  }
}

#endif





unsigned long previousMillisETrigger ;
const unsigned long intervalETrigger = ETRIGGERTIME ;
int relayETriggerON, relayETriggerOFF;



void testEmergencyStop() {
  if (Input > EmergencyStopTemp && emergencyStop == false) {
    emergencyStop = true;
  } else if (Input < 100 && emergencyStop == true) {
    emergencyStop = false;
  }
}




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

    readIndex = 0;
  } else {
    readIndex++;
  }
}







boolean checkSensor(float tempInput) {
  boolean sensorOK = false;
  boolean badCondition = ( tempInput < 0 || tempInput > 150 || fabs(tempInput - previousInput) > 5);
  if ( badCondition && !sensorError) {
    error++;
    sensorOK = false;
    if (error >= 5)
    {
     Serial.printf("*** WARNING: temperature sensor reading: consec_errors = %i, temp_current = %.1f\n", error, tempInput);
    }
  } else if (badCondition == false && sensorOK == false) {
    error = 0;
    sensorOK = true;
  }
  if (error >= maxErrorCounter && !sensorError) {
    sensorError = true ;
    Serial.printf("*** ERROR: temperature sensor malfunction: temp_current = %.1f\n",tempInput);
  } else if (error == 0 && sensorError) {
    sensorError = false ;
  }
  return sensorOK;
}






void refreshTemp() {
  unsigned long currentMillistemp = millis();
  previousInput = Input ;
  if (TempSensor == 1)
  {
    if (currentMillistemp - previousMillistemp >= intervaltempmesds18b20)
    {
      previousMillistemp = currentMillistemp;
      sensors.requestTemperatures();
      if (!checkSensor(sensors.getTempCByIndex(0)) && firstreading == 0 ) return;
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



      temperature = 0;
       #if (ONE_WIRE_BUS == 16 && defined(ESP8266))
         Sensor1.getTemperature(&temperature);
         Temperature_C = Sensor1.calc_Celsius(&temperature);
         #endif
       #if ((ONE_WIRE_BUS != 16 && defined(ESP8266)) || defined(ESP32))
        Temperature_C = Sensor2.getTemp();
       #endif

      if (!checkSensor(Temperature_C) && firstreading == 0) return;
      Input = Temperature_C;
      if (Brewdetection != 0) {
        movAvg();
      } else if (firstreading != 0) {
        firstreading = 0;
      }
    }
  }
}





#include "brewvoid.h"
#include "scalevoid.h"





void initOfflineMode()
{
  #if DISPLAY != 0
    displayMessage("", "", "", "", "Begin Fallback,", "No Wifi");
  #endif
  Serial.println("Start offline mode with eeprom values, no wifi :(");
  Offlinemodus = 1 ;

  if (readSysParamsFromStorage() != 0)
  {
    #if DISPLAY != 0
    displayMessage("", "", "", "", "No eeprom,", "Values");
    #endif
    Serial.println("No working eeprom value, I am sorry, but use default offline value :)");
    delay(1000);
  }
}





void checkWifi() {
  if (Offlinemodus == 1 || brewcounter > 11) return;
  do {
    if ((millis() - lastWifiConnectionAttempt >= wifiConnectionDelay) && (wifiReconnects <= maxWifiReconnects)) {
      int statusTemp = WiFi.status();
      if (statusTemp != WL_CONNECTED) {
        lastWifiConnectionAttempt = millis();
        wifiReconnects++;
        Serial.printf("Attempting WIFI reconnection: %i\n",wifiReconnects);
        if (!setupDone) {
           #if DISPLAY != 0
            displayMessage("", "", "", "", langstring_wifirecon, String(wifiReconnects));
          #endif
        }
        WiFi.disconnect();
        WiFi.begin(ssid, pass);

        int count = 1;
        while (WiFi.status() != WL_CONNECTED && count <= 20) {
          delay(100);
          count++;
        }
      }
    }
    yield();
  } while ( !setupDone && wifiReconnects < maxWifiReconnects && WiFi.status() != WL_CONNECTED);

  if (wifiReconnects >= maxWifiReconnects && !setupDone) {
    initOfflineMode();
  }

}


void sendInflux(){
  unsigned long currentMillisInflux = millis();

  if (currentMillisInflux - previousMillisInflux >= intervalInflux){
    previousMillisInflux = currentMillisInflux;
    sensor.clearFields();
    sensor.addField("value", Input);
    sensor.addField("setPoint", setPoint);
    sensor.addField("HeaterPower", Output);
    sensor.addField("Kp", bPID.GetKp());
    sensor.addField("Ki", bPID.GetKi());
    sensor.addField("Kd", bPID.GetKd());
    sensor.addField("pidON", pidON);
    sensor.addField("brewtime", brewtime);
    sensor.addField("preinfusionpause", preinfusionpause);
    sensor.addField("preinfusion", preinfusion);
    sensor.addField("SteamON", SteamON);
    byte mac[6];
    WiFi.macAddress(mac);
    String macaddr0 = number2string(mac[0]);
    String macaddr1 = number2string(mac[1]);
    String macaddr2 = number2string(mac[2]);
    String macaddr3 = number2string(mac[3]);
    String macaddr4 = number2string(mac[4]);
    String macaddr5 = number2string(mac[5]);
    String completemac = macaddr0 + macaddr1 + macaddr2 + macaddr3 + macaddr4 + macaddr5;
    sensor.addField("mac", completemac);


    if (!client.writePoint(sensor)) {
      Serial.printf("InfluxDB write failed: %s\n", client.getLastErrorMessage().c_str());
    }
  }
}







void checkBlynk() {
  if (Offlinemodus == 1 ||BLYNK == 0 || brewcounter > 11) return;
  if ((millis() - lastBlynkConnectionAttempt >= wifiConnectionDelay) && (blynkReCnctCount <= maxWifiReconnects)) {
    int statusTemp = Blynk.connected();
    if (statusTemp != 1) {
      lastBlynkConnectionAttempt = millis();
      blynkReCnctCount++;
      Serial.printf("Attempting blynk reconnection: %i\n",blynkReCnctCount);
      Blynk.connect(3000);
    }
  }
}
# 904 "/Users/Andreas/Documents/GitHub/ranciliopid/src/rancilio-pid.ino"
void checkMQTT(){
  if (Offlinemodus == 1 || brewcounter > 11) return;
  if ((millis() - lastMQTTConnectionAttempt >= wifiConnectionDelay) && (MQTTReCnctCount <= maxWifiReconnects)) {
    int statusTemp = mqtt.connected();
    if (statusTemp != 1) {
      lastMQTTConnectionAttempt = millis();
      MQTTReCnctCount++;
      Serial.printf("Attempting MQTT reconnection: %i\n",MQTTReCnctCount);
      if (mqtt.connect(hostname, mqtt_username, mqtt_password,topic_will,0,0,"exit") == true);{
        mqtt.subscribe(topic_set);
        Serial.println("Subscribe to MQTT Topics");
      }
    }
  }
}





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




bool mqtt_publish(const char *reading, char *payload)
{
#if MQTT
    char topic[120];
    snprintf(topic, 120, "%s%s/%s", mqtt_topic_prefix, hostname, reading);
    return mqtt.publish(topic, payload, true);
#else
    return false;
#endif
}





void sendToBlynkMQTT()
{
  if (Offlinemodus == 1) return;

  unsigned long currentMillisBlynk = millis();
  unsigned long currentMillisMQTT = millis();
  unsigned long currentMillistemp = 0;
  if ((currentMillisBlynk - previousMillisBlynk >= intervalBlynk) && (BLYNK == 1))
  {
    if (Blynk.connected())
    {
      if (blynksendcounter == 1) {
        Blynk.virtualWrite(V2, Input);
      }
      if (blynksendcounter == 2) {
        Blynk.virtualWrite(V23, Output);
      }
      if (blynksendcounter == 3) {
        Blynk.virtualWrite(V17, setPoint);
      }
      if (blynksendcounter == 4) {
        Blynk.virtualWrite(V35, heatrateaverage);
      }
      if (blynksendcounter == 5) {
        Blynk.virtualWrite(V36, heatrateaveragemin);
      }
      if (grafana == 1 && blynksendcounter >= 6) {

        Blynk.virtualWrite(V60, Input, Output, bPID.GetKp(), bPID.GetKi(), bPID.GetKd(), setPoint, heatrateaverage);
        blynksendcounter = 0;
      } else if (grafana == 0 && blynksendcounter >= 5) {
        blynksendcounter = 0;
      }
      blynksendcounter++;
    }
  }
  if ((currentMillisMQTT - previousMillisMQTT >= intervalMQTT) && (MQTT == 1))
  {
     previousMillisMQTT = currentMillisMQTT;
              checkMQTT();
              if (mqtt.connected() == 1)
              {
                mqtt_publish("temperature", number2string(Input));
                mqtt_publish("setPoint", number2string(setPoint));
                mqtt_publish("HeaterPower", number2string(Output));
                mqtt_publish("Kp", number2string(bPID.GetKp()));
                mqtt_publish("Ki", number2string(bPID.GetKi()));
                mqtt_publish("Kd", number2string(bPID.GetKd()));
                mqtt_publish("pidON", number2string(pidON));
                mqtt_publish("brewtime", number2string(brewtime));
                mqtt_publish("preinfusionpause", number2string(preinfusionpause));
                mqtt_publish("preinfusion", number2string(preinfusion));
                mqtt_publish("SteamON", number2string(SteamON));

              }
    }

}




void brewdetection()
{
  if (brewboarder == 0) return;



  if (Brewdetection == 1)
  {
     if (timerBrewdetection == 1)
    {
     bezugsZeit = millis() - timeBrewdetection ;
     }

    if (millis() - timeBrewdetection > brewtimersoftware * 1000 && timerBrewdetection == 1 )
    {
      timerBrewdetection = 0 ;
      if (machinestate != 30)
      {
        bezugsZeit = 0 ;
      }
     }
  } else if (Brewdetection == 2)
  {
    if (millis() - timeBrewdetection > brewtimersoftware * 1000 && timerBrewdetection == 1 )
    {
      timerBrewdetection = 0 ;
    }
  } else if (Brewdetection == 3)
  {

    if (( digitalRead(PINVOLTAGESENSOR) == VoltageSensorON) && brewDetected == 1)
       {
       bezugsZeit = millis() - startZeit ;
       lastbezugszeit = bezugsZeit ;
       }

    if
     ((digitalRead(PINVOLTAGESENSOR) == VoltageSensorOFF) && (brewDetected == 1 || coolingFlushDetectedQM == true) )
      {
        brewDetected = 0;
        timePVStoON = bezugsZeit;
        bezugsZeit = 0 ;
        startZeit = 0;
        coolingFlushDetectedQM = false;
        Serial.println("HW Brew - Voltage Sensor - End");

      }
    if (millis() - timeBrewdetection > brewtimersoftware * 1000 && timerBrewdetection == 1)
    {
      timerBrewdetection = 0 ;
    }
  }



  if ( Brewdetection == 1)
  {
    if (heatrateaverage <= -brewboarder && timerBrewdetection == 0 && (fabs(Input - BrewSetPoint) < 5))
    {
      Serial.println("SW Brew detected") ;
      timeBrewdetection = millis() ;
      timerBrewdetection = 1 ;
    }
  } else if (Brewdetection == 2)
  {
    if (brewcounter > 10 && brewDetected == 0 && brewboarder != 0)
    {
      Serial.println("HW Brew detected") ;
      timeBrewdetection = millis() ;
      timerBrewdetection = 1 ;
      brewDetected = 1;
    }
  } else if (Brewdetection == 3)
  {
    switch (machine) {

      case QuickMill:

      if (!coolingFlushDetectedQM)
      {
        int pvs = digitalRead(PINVOLTAGESENSOR);
        if (pvs == VoltageSensorON && brewDetected == 0 && brewSteamDetectedQM == 0 && !steamQM_active)
        {
          timeBrewdetection = millis();
          timePVStoON = millis();
          timerBrewdetection = 1;
          brewDetected = 0;
          lastbezugszeit = 0;
          brewSteamDetectedQM = 1;
          Serial.println("Quick Mill: setting brewSteamDetectedQM = 1");
          logbrew.reset();
        }

        if (brewSteamDetectedQM == 1)
        {
          if (pvs == VoltageSensorOFF)
          {
            brewSteamDetectedQM = 0;

            if (millis() - timePVStoON < maxBrewDurationForSteamModeQM_ON)
            {
              Serial.println("Quick Mill: steam-mode detected");
              initSteamQM();
            } else {
              Serial.printf("*** ERROR: QuickMill: neither brew nor steam\n");
            }
          }
          else if (millis() - timePVStoON > maxBrewDurationForSteamModeQM_ON)
          {
            if( Input < BrewSetPoint + 2) {
              Serial.println("Quick Mill: brew-mode detected");
              startZeit = timePVStoON;
              brewDetected = 1;
              brewSteamDetectedQM = 0;
            } else {
              Serial.println("Quick Mill: cooling-flush detected");
              coolingFlushDetectedQM = true;
              brewSteamDetectedQM = 0;
            }
          }
        }
      }
      break;

      default:
      previousMillisVoltagesensorreading = millis();
      if (digitalRead(PINVOLTAGESENSOR) == VoltageSensorON && brewDetected == 0 )
      {
        Serial.println("HW Brew - Voltage Sensor -  Start") ;
        timeBrewdetection = millis() ;
        startZeit = millis() ;
        timerBrewdetection = 1 ;
        brewDetected = 1;
        lastbezugszeit = 0 ;
      }
    }
  }
}






int filter(int input) {
  inX = input * 0.3;
  inY = inOld * 0.7;
  inSum = inX + inY;
  inOld = inSum;

  return inSum;
}






void mqtt_callback(char* topic, byte* data, unsigned int length) {
  char topic_str[256];
  os_memcpy(topic_str, topic, sizeof(topic_str));
  topic_str[255] = '\0';
  char data_str[length+1];
  os_memcpy(data_str, data, length);
  data_str[length] = '\0';
  char topic_pattern[255];
  char configVar[120];
  char cmd[64];
  double data_double;

  snprintf(topic_pattern, sizeof(topic_pattern), "%s%s/%%[^\\/]/%%[^\\/]", mqtt_topic_prefix, hostname);
  Serial.println(topic_pattern);
  if ( (sscanf( topic_str, topic_pattern , &configVar, &cmd) != 2) || (strcmp(cmd, "set") != 0) ) {
    Serial.println(topic_str);
    return;
  }
  Serial.println(topic_str);
  Serial.println(data_str);
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
    mqtt_publish("brewtime", number2string(brewtime));
    brewtime = data_double ;
    return;
  }
  if (strcmp(configVar, "preinfusion") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (Blynk.connected()) { Blynk.virtualWrite(V9, String(data_double));}
    mqtt_publish("preinfusion", number2string(preinfusion));
    preinfusion = data_double;
    return;
  }
  if (strcmp(configVar, "preinfusionpause") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (Blynk.connected()) { Blynk.virtualWrite(V10, String(data_double));}
    mqtt_publish("preinfusionpause", number2string(preinfusionpause));
    preinfusionpause = data_double ;
    return;
  }
    if (strcmp(configVar, "pidON") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (Blynk.connected()) { Blynk.virtualWrite(V13,String(data_double));}
    mqtt_publish("pidON", number2string(pidON));
    pidON = data_double ;
    return;
  }

}





void ETriggervoid()
{

  static int ETriggeractive = 0;
  unsigned long currentMillisETrigger = millis();
  if (ETRIGGER == 1)
  {

    if (currentMillisETrigger - previousMillisETrigger >= (1000*intervalETrigger))
    {
      ETriggeractive = 1 ;
      previousMillisETrigger = currentMillisETrigger;

      digitalWrite(PINETRIGGER, relayETriggerON);
    }

    else if (ETriggeractive == 1 && previousMillisETrigger+(10*1000) < (currentMillisETrigger))
    {
    digitalWrite(PINETRIGGER, relayETriggerOFF);
    ETriggeractive = 0;
    }
  }
}



void checkSteamON()
{

  if (digitalRead(STEAMONPIN) == HIGH)
  {
    SteamON = 1;
  }
  if (digitalRead(STEAMONPIN) == LOW && SteamFirstON == 0)
  {
    SteamON = 0;
  }

  if (machine == QuickMill )
  {
    if (steamQM_active == true)
    {
      if( checkSteamOffQM() == true )
      {
        SteamON = 0;
        steamQM_active = false;
        lastTimePVSwasON = 0;
      }
      else
      {
        SteamON = 1;
      }
    }
  }
  if (SteamON == 1)
  {
    setPoint = SteamSetPoint ;
  }
   if (SteamON == 0)
  {
    setPoint = BrewSetPoint ;
  }
}

void setEmergencyStopTemp()
{
  if (machinestate == kSteam || machinestate == kCoolDown)
  {
    if (EmergencyStopTemp != 145)
    EmergencyStopTemp = 145;
  }
  else
  {
    if (EmergencyStopTemp != 120)
    EmergencyStopTemp = 120;
  }
}


void initSteamQM()
{



  lastTimePVSwasON = millis();
  steamQM_active = true;
  timePVStoON = 0;
  SteamON = 1;
}

boolean checkSteamOffQM()
{





  if( digitalRead(PINVOLTAGESENSOR) == VoltageSensorON ) {
    lastTimePVSwasON = millis();
  }

  if( (millis() - lastTimePVSwasON) > minPVSOffTimedForSteamModeQM_OFF ) {
    lastTimePVSwasON = 0;
    return true;
  }

  return false;
}





void machinestatevoid()
{
  switch (machinestate)
  {

    case kInit:
      if (Input < (BrewSetPoint-1) || Input < 150 )
      {
        machinestate = kColdStart;
        Serial.println(Input);
        Serial.println(machinestate);
      }

      if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
     if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kColdStart:
      switch (machinestatecold)




      {
        case 0:
          if (Input >= (BrewSetPoint-1) && Input < 150 )
          {
            machinestatecoldmillis = millis();
            machinestatecold = 10 ;
            Serial.println("Input >= (BrewSetPoint-1), wait 10 sec before machinestate 19");

          }
          break;
        case 10:
          if (Input < (BrewSetPoint-1))
          {
            machinestatecold = 0 ;
            Serial.println("Reset timer for machinestate 19: Input < (BrewSetPoint-1)");
          }
          if (machinestatecoldmillis+10*1000 < millis() )
          {
            machinestate = kSetPointNegative ;
            Serial.println("10 sec Input >= (BrewSetPoint-1) finished, switch to state 19");
          }
          break;
      }
      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

      if
      (
       (bezugsZeit > 0 && ONLYPID == 1) ||
       (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)
      )

      {
        machinestate = kBrew;
      }

      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

      if (backflushON || backflushState > 10)
      {
        machinestate = kBackflush;
      }

     if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
     if(sensorError)
      {
        machinestate = kSensorError;
      }
      break;

      case kSetPointNegative:
      brewdetection();
      if (Input >= (BrewSetPoint))
      {
        machinestate = kPidNormal;
      }
      if
      (
       (bezugsZeit > 0 && ONLYPID == 1) ||
       (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)
      )
      {
        machinestate = kBrew;
      }
      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

      if (backflushON || backflushState > 10)
      {
        machinestate = kBackflush;
      }

      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

     if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
     if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kPidNormal:
      brewdetection();
      if
      (
       (bezugsZeit > 0 && ONLYPID == 1) ||
       (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)
      )
      {
        machinestate = kBrew;
      }
      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

      if (backflushON || backflushState > 10)
      {
        machinestate = kBackflush;
      }
      if (emergencyStop)
      {
        machinestate = kEmergencyStop;
      }
     if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
     if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kBrew:
      brewdetection();

      if (logbrew.check())
          Serial.printf("(tB,T,hra) --> %5.2f %6.2f %8.2f\n",(double)(millis() - startZeit)/1000,Input,heatrateaverage);
      if
      (
       (bezugsZeit > 35*1000 && Brewdetection == 1 && ONLYPID == 1 ) ||
       (bezugsZeit == 0 && Brewdetection == 3 && ONLYPID == 1 ) ||
       ((brewcounter == 10 || brewcounter == 43) && ONLYPID == 0 )
      )
      {
       if ((ONLYPID == 1 && Brewdetection == 3) || ONLYPID == 0 )
       {
         machinestate = kShotTimerAfterBrew ;
         lastbezugszeitMillis = millis() ;

       }
       if (ONLYPID == 1 && Brewdetection == 1 && timerBrewdetection == 1)
       {
         machinestate = kBrewDetectionTrailing ;
       }
      }
      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

      if (emergencyStop)
      {
        machinestate = kEmergencyStop;
      }
     if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
     if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kShotTimerAfterBrew:
    brewdetection();
      if ( millis()-lastbezugszeitMillis > BREWSWITCHDELAY )
      {
       Serial.printf("Bezugsdauer: %4.1f s\n",lastbezugszeit/1000);
       machinestate = kBrewDetectionTrailing ;
       lastbezugszeit = 0 ;
      }

      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

     if (backflushON || backflushState > 10)
      {
        machinestate = kBackflush;
      }

      if (emergencyStop)
      {
        machinestate = kEmergencyStop;
      }

     if (pidON == 0)
      {
        machinestate = kPidOffline;
      }

     if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kBrewDetectionTrailing:
      brewdetection();
      if (timerBrewdetection == 0)
      {
        machinestate = kPidNormal;
      }
      if
      (
       (bezugsZeit > 0 && ONLYPID == 1 && Brewdetection == 3) ||
       (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)
      )
      {
        machinestate = kBrew;
      }

      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

      if (backflushON || backflushState > 10)
      {
        machinestate = kBackflush;
      }

      if (emergencyStop)
      {
        machinestate = kEmergencyStop;
      }
      if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
     if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kSteam:
      if (SteamON == 0)
      {
        machinestate = kCoolDown;
      }

       if (emergencyStop)
      {
        machinestate = kEmergencyStop;
      }

      if (backflushON || backflushState > 10)
      {
        machinestate = kBackflush;
      }

      if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
      if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kCoolDown:
    if (Brewdetection == 2 || Brewdetection == 3)
      {




        brewdetection();
      }
      if (Brewdetection == 1 && ONLYPID == 1)
      {

         if (heatrateaverage > 0 && Input < BrewSetPoint + 2)
         {
            machinestate = kPidNormal;
         }
      }
      if ((Brewdetection == 3 || Brewdetection == 2) && Input < BrewSetPoint + 2)
      {
        machinestate = kPidNormal;
      }

      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

      if (backflushON || backflushState > 10)
      {
        machinestate = kBackflush;
      }

      if (emergencyStop)
      {
        machinestate = kEmergencyStop;
      }
      if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
      if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kBackflush:
      if (backflushON == 0)
       {
         machinestate = kPidNormal;
       }

      if (emergencyStop)
       {
         machinestate = kEmergencyStop;
       }
      if (pidON == 0)
       {
         machinestate = kPidOffline;
       }
      if(sensorError)
       {
         machinestate = kSensorError;
       }
    break;

    case kEmergencyStop:
      if (!emergencyStop)
      {
        machinestate = kPidNormal;
      }
      if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
      if(sensorError)
      {
        machinestate = kSensorError ;
      }
    break;

    case kPidOffline:
      if (pidON == 1)
      {
        if(kaltstart)
        {
          machinestate = kColdStart;
        }
        else if(!kaltstart && (Input > (BrewSetPoint-10) ))
        {
          machinestate = kPidNormal;
        }
        else if (Input <= (BrewSetPoint-10) )
        {
          machinestate = kColdStart;
          kaltstart = true;
        }
      }

      if(sensorError)
      {
        machinestate = kSensorError ;
      }
    break;

    case kSensorError:
      machinestate = kSensorError ;
    break;

    case keepromError:
      machinestate = keepromError ;

    break;

  }

  if (machinestate != lastmachinestate) {
    Serial.printf("new machinestate: %i -> %i\n", lastmachinestate, machinestate);
    lastmachinestate = machinestate;
  }
}

void debugVerboseOutput()
{
  static PeriodicTrigger trigger(10000);
  if(trigger.check())
  {
    Serial.printf("Tsoll=%5.1f  Tist=%5.1f Machinestate=%2i KP=%4.2f KI=%4.2f KD=%4.2f\n",BrewSetPoint,Input,machinestate,bPID.GetKp(),bPID.GetKi(),bPID.GetKd());
  }
}

void ledtemp()
{
  if (USELED == 1)
  {
    pinMode(LEDPIN, OUTPUT);
    digitalWrite(LEDPIN, LOW);
    if ((machinestate == kPidNormal && (fabs(Input - setPoint) < 0.5)) ||
        (Input > 115 && fabs(Input - BrewSetPoint) < 5))
    {
    digitalWrite(LEDPIN, HIGH);
    }
  }
}



void setup()
{
  Serial.begin(115200);

  initTimer1();

  storageSetup();


  checklastpoweroff();

  if (softApEnabled == 1)
  {



    #if DISPLAY != 0
      u8g2.setI2CAddress(oled_i2c * 2);
      u8g2.begin();
      u8g2_prepare();
      displayLogo(sysVersion, "");
      delay(2000);
    #endif
    disableTimer1();
    createSoftAp();

  } else if(softApEnabled == 0)
  {
    if (MQTT == 1) {

      snprintf(topic_will, sizeof(topic_will), "%s%s/%s", mqtt_topic_prefix, hostname, "will");
      snprintf(topic_set, sizeof(topic_set), "%s%s/+/%s", mqtt_topic_prefix, hostname, "set");
      mqtt.setServer(mqtt_server_ip, mqtt_server_port);
      mqtt.setCallback(mqtt_callback);
      checkMQTT();
    }




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
      relayETriggerOFF = LOW;
    } else {
      relayETriggerON = LOW;
      relayETriggerOFF = HIGH;
    }
    if (VOLTAGESENSORTYPE)
    {
      VoltageSensorON = HIGH;
      VoltageSensorOFF = LOW;
    } else {
      VoltageSensorON = LOW;
      VoltageSensorOFF = HIGH;
    }




    pinMode(pinRelayVentil, OUTPUT);
    pinMode(pinRelayPumpe, OUTPUT);
    pinMode(pinRelayHeater, OUTPUT);
    pinMode(STEAMONPIN, INPUT);
    digitalWrite(pinRelayVentil, relayOFF);
    digitalWrite(pinRelayPumpe, relayOFF);
    digitalWrite(pinRelayHeater, LOW);
    if (ETRIGGER == 1)
    {
      pinMode(PINETRIGGER, OUTPUT);
      digitalWrite(PINETRIGGER, relayETriggerOFF);
    }
    if (BREWDETECTION == 3)
    {
      pinMode(PINVOLTAGESENSOR, PINMODEVOLTAGESENSOR);
    }
    if (PINBREWSWITCH > 0)
    {
      #if (defined(ESP8266) && PINBREWSWITCH == 16)
        pinMode(PINBREWSWITCH, INPUT_PULLDOWN_16);
      #endif
      #if (defined(ESP8266) && PINBREWSWITCH == 15)
        pinMode(PINBREWSWITCH, INPUT);
      #endif
      #if defined(ESP32)
        pinMode(PINBREWSWITCH, INPUT_PULLDOWN);;
      #endif
    }
      #if (defined(ESP8266) && STEAMONPIN == 16)
        pinMode(STEAMONPIN, INPUT_PULLDOWN_16);
      #endif
        #if (defined(ESP8266) && STEAMONPIN == 15)
      pinMode(STEAMONPIN, INPUT);
      #endif
      #if defined(ESP32)
        pinMode(STEAMONPIN, INPUT_PULLDOWN);
      #endif



    #if DISPLAY != 0
      u8g2.setI2CAddress(oled_i2c * 2);
      u8g2.begin();
      u8g2_prepare();
      displayLogo(sysVersion, "");
      delay(2000);
    #endif



    #if (BREWMODE == 2 || ONLYPIDSCALE == 1)
      initScale() ;
    #endif





    if (TOF != 0) {
    lox.begin(tof_i2c);
    lox.setMeasurementTimingBudgetMicroSeconds(2000000);
    }




    if (Offlinemodus == 0)
    {
      #if defined(ESP8266)
        WiFi.hostname(hostname);
      #endif
      unsigned long started = millis();
      #if DISPLAY != 0
        displayLogo(langstring_connectwifi1, ssid);
      #endif



      WiFi.mode(WIFI_STA);
      WiFi.persistent(false);
      WiFi.begin(ssid, pass);
      #if defined(ESP32)
      WiFi.setHostname(hostname);
      #endif
      Serial.printf("Connecting to %s ...\n",ssid);


      while ((WiFi.status() != WL_CONNECTED) && (millis() - started < 20000))
      {
        yield();
      }

      checkWifi();

      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.printf("WiFi connected - IP = %i.%i.%i.%i\n",WiFi.localIP()[0],WiFi.localIP()[1],WiFi.localIP()[2],WiFi.localIP()[3]);

        if ( BLYNK == 1)
        {
          Serial.println("Wifi works, now try Blynk (timeout 30s)");
        }

        if (fallback == 0) {
          #if DISPLAY != 0
            displayLogo(langstring_connectblynk1[0], langstring_connectblynk1[1]);
          #endif
        } else if (fallback == 1) {
          #if DISPLAY != 0
            displayLogo(langstring_connectwifi2[0], langstring_connectwifi2[1]);
          #endif
        }





        if (LOCALHOST == 1)
        {
            setEepromWriteFcn(writeSysParamsToStorage);
            setBlynkWriteFcn(writeSysParamsToBlynk);
            setSteammodeFcn(setSteammode);
            if (readSysParamsFromStorage() != 0)
              {
                #if DISPLAY != 0
                displayLogo("3:", "use eeprom values..");
                #endif
              }
            else{
                #if DISPLAY != 0
                displayLogo("3:", "config defaults..");
                #endif
            }
            serverSetup();
        }


        delay(1000);


        if ( BLYNK == 1)
        {
          Blynk.config(auth, blynkaddress, blynkport) ;
          Blynk.connect(30000);

          if (Blynk.connected() == true)
          {
            #if DISPLAY != 0
              displayLogo(langstring_connectblynk2[0], langstring_connectblynk2[1]);
            #endif
            Serial.println("Blynk is online");
            if (fallback == 1)
            {
              Serial.println("sync all variables and write new values to eeprom");

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



              writeSysParamsToStorage();
            }
          } else
          {
            Serial.println("No connection to Blynk");
            if (readSysParamsFromStorage() == 0)
            {
              #if DISPLAY != 0
              displayLogo("3: Blynk not connected", "use eeprom values..");
              #endif
            }
          }
        }
      }
      else
      {
        #if DISPLAY != 0
          displayLogo(langstring_nowifi[0], langstring_nowifi[1]);
        #endif
        Serial.println("No WIFI");
        WiFi.disconnect(true);
        delay(1000);
      }
    }





    if (ota && Offlinemodus == 0 && WiFi.status() == WL_CONNECTED) {
      ArduinoOTA.setHostname(OTAhost);
      ArduinoOTA.setPassword(OTApass);
      ArduinoOTA.begin();
    }







    bPID.SetSampleTime(windowSize);
    bPID.SetOutputLimits(0, windowSize);
    bPID.SetMode(AUTOMATIC);





    if (TempSensor == 1)
    {
      sensors.begin();
      sensors.getAddress(sensorDeviceAddress, 0);
      sensors.setResolution(sensorDeviceAddress, 10) ;
      sensors.requestTemperatures();
      Input = sensors.getTempCByIndex(0);
    }
    if (TempSensor == 2)
    {
      temperature = 0;
      #if (ONE_WIRE_BUS == 16 && defined(ESP8266))
          Sensor1.getTemperature(&temperature);
          Input = Sensor1.calc_Celsius(&temperature);
      #endif
      #if ((ONE_WIRE_BUS != 16 && defined(ESP8266)) || defined(ESP32))
          Input = Sensor2.getTemp();
      #endif
    }







    if (Brewdetection == 1) {
      for (int thisReading = 0; thisReading < numReadings; thisReading++) {
        readingstemp[thisReading] = 0;
        readingstime[thisReading] = 0;
        readingchangerate[thisReading] = 0;
      }
    }
# 2145 "/Users/Andreas/Documents/GitHub/ranciliopid/src/rancilio-pid.ino"
    unsigned long currentTime = millis();
    previousMillistemp = currentTime;
    windowStartTime = currentTime;
    previousMillisDisplay = currentTime;
    previousMillisBlynk = currentTime;
    previousMillisMQTT = currentTime;
    previousMillisInflux = currentTime;
    previousMillisETrigger = currentTime;
    previousMillisVoltagesensorreading = currentTime;
    #if (BREWMODE == 2)
    previousMillisScale = currentTime;
    #endif
    #if (PRESSURESENSOR == 1)
    previousMillisPressure = currentTime;
    #endif
    setupDone = true;

    byte mac[6];
    WiFi.macAddress(mac);
    String macaddr0 = number2string(mac[0]);
    String macaddr1 = number2string(mac[1]);
    String macaddr2 = number2string(mac[2]);
    String macaddr3 = number2string(mac[3]);
    String macaddr4 = number2string(mac[4]);
    String macaddr5 = number2string(mac[5]);
    String completemac = macaddr0 + macaddr1 + macaddr2 + macaddr3 + macaddr4 + macaddr5;
    Serial.printf("MAC-ADRESSE: %s\n", completemac.c_str());

  enableTimer1();
  }
}

void loop() {
  if (calibration_mode == 1 && TOF == 1)
  {
      loopcalibrate();
  } else if (softApEnabled == 0)
  {
      looppid();
  } else if (softApEnabled == 1)
  {
    switch (softApstate)
    {
      case 0:
        if(WiFi.softAPgetStationNum() > 0)
        {
          ArduinoOTA.setHostname(OTAhost);
          ArduinoOTA.setPassword(OTApass);
          ArduinoOTA.begin();
          softApstate = 10;
        }
      break;
      case 10:
      ArduinoOTA.handle();

      ArduinoOTA.onStart([]()
      {

        #if defined(ESP8266)
        timer1_disable();
        #endif
        #if defined(ESP32)
        timerAlarmDisable(timer);
        #endif
        digitalWrite(pinRelayHeater, LOW);
      });
      ArduinoOTA.onError([](ota_error_t error)
      {
        #if defined(ESP8266)
        timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
        #endif
        #if defined(ESP32)
        timerAlarmEnable(timer);
        #endif
      });

      ArduinoOTA.onEnd([]()
      {
        #if defined(ESP8266)
        timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
        #endif
        #if defined(ESP32)
          timerAlarmEnable(timer);
        #endif
      });
      break;
    }
  }

}


void loopcalibrate()
{

  if (pidMode == 1)
  {
    pidMode = 0;
    bPID.SetMode(pidMode);
    Output = 0;
  }
  if (Blynk.connected() && BLYNK == 1)
  {
      Blynk.run();
      blynkReCnctCount = 0;
  } else
  {
    checkBlynk();
  }
    digitalWrite(pinRelayHeater, LOW);

  unsigned long currentMillisTOF = millis();
  if (currentMillisTOF - previousMillisTOF >= intervalTOF)
  {
    previousMillisTOF = millis() ;
    VL53L0X_RangingMeasurementData_t measure;
    lox.rangingTest(&measure, false);
    distance = measure.RangeMilliMeter;
    Serial.println(distance);
    Serial.println("mm");
    #if DISPLAY !=0
        displayDistance(distance);
    #endif
  }
}


void looppid()
{

  if (WiFi.status() == WL_CONNECTED && Offlinemodus == 0)
  {

    if (MQTT == 1)
    {
      checkMQTT();
      if (mqtt.connected() == 1)
      {
        mqtt.loop();
      }
    }
    ArduinoOTA.handle();

    ArduinoOTA.onStart([]()
    {
      disableTimer1();
      digitalWrite(pinRelayHeater, LOW);
    });
    ArduinoOTA.onError([](ota_error_t error)
    {
      enableTimer1();
    });

    ArduinoOTA.onEnd([]()
    {
      enableTimer1();
    });

    if (Blynk.connected() && BLYNK == 1)
    {
      Blynk.run();
      blynkReCnctCount = 0;
    } else
    {
      checkBlynk();
    }
    wifiReconnects = 0;
  } else
  {
    checkWifi();
  }
  if (TOF != 0)
  {
        unsigned long currentMillisTOF = millis();
      if (currentMillisTOF - previousMillisTOF >= intervalTOF)
      {
        previousMillisTOF = millis() ;
        VL53L0X_RangingMeasurementData_t measure;
        lox.rangingTest(&measure, false);
        distance = measure.RangeMilliMeter;
        if (distance <= 1000)
        {
          percentage = (100.00 / (water_empty - water_full)) * (water_empty - distance);
          Serial.println(percentage);
        }
      }
  }

  refreshTemp();
  testEmergencyStop();
  bPID.Compute();

  if ((millis() - lastTempEvent) > tempEventInterval) {
    sendTempEvent(Input, BrewSetPoint);
    lastTempEvent = millis();
  }

  #if (BREWMODE == 2 || ONLYPIDSCALE == 1 )
    checkWeight() ;
  #endif
    #if (PRESSURESENSOR == 1)
    checkPressure();
    #endif
  brew();
  checkSteamON();
  setEmergencyStopTemp();
  sendToBlynkMQTT();
  machinestatevoid() ;
  setchecklastpoweroff();
  ledtemp();

  if (INFLUXDB == 1){
    sendInflux();
  }

  if (ETRIGGER == 1)
  {
    ETriggervoid();
  }
  #if (ONLYPIDSCALE == 1)
      shottimerscale() ;
  #endif





  #if DISPLAY != 0
      unsigned long currentMillisDisplay = millis();
      if (currentMillisDisplay - previousMillisDisplay >= 100)
      {
        displayShottimer() ;
      }
      if (currentMillisDisplay - previousMillisDisplay >= intervalDisplay)
      {
        previousMillisDisplay = currentMillisDisplay;
        #if DISPLAYTEMPLATE < 20
          Displaymachinestate() ;
        #endif
        printScreen();
      }
  #endif
  if (machinestate == kPidOffline || machinestate == kSensorError || machinestate == kEmergencyStop || machinestate == keepromError)
  {
    if (pidMode == 1)
    {

      pidMode = 0;
      bPID.SetMode(pidMode);
      Output = 0 ;
      digitalWrite(pinRelayHeater, LOW);
    }
  }
  else
  {
    if (pidMode == 0)
    {
    pidMode = 1;
    bPID.SetMode(pidMode);
    }
  }


  if (machinestate == kInit || machinestate == kColdStart || machinestate == kSetPointNegative)
  {
    if (startTn != 0) {
      startKi = startKp / startTn;
    } else {
      startKi = 0 ;
    }
    if (lastmachinestatepid != machinestate)
    {
      Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n",startKp,startKi,0);
      lastmachinestatepid = machinestate;
    }
    bPID.SetTunings(startKp, startKi, 0, P_ON_M);

  }
  if (machinestate == kPidNormal )
  {

    if (aggTn != 0) {
      aggKi = aggKp / aggTn ;
    } else {
      aggKi = 0 ;
    }
    aggKd = aggTv * aggKp ;
    if (lastmachinestatepid != machinestate)
    {
      Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n",aggKp,aggKi,aggKd);
      lastmachinestatepid = machinestate;
    }
    bPID.SetTunings(aggKp, aggKi, aggKd, PonE);
    kaltstart = false;
  }

  if (machinestate >= 30 && machinestate <= 35)
  {

    if (aggbTn != 0) {
      aggbKi = aggbKp / aggbTn ;
    } else {
      aggbKi = 0 ;
    }
    aggbKd = aggbTv * aggbKp ;
    if (lastmachinestatepid != machinestate)
    {
      Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n",aggbKp,aggbKi,aggbKd);
      lastmachinestatepid = machinestate;
    }
    bPID.SetTunings(aggbKp, aggbKi, aggbKd, PonE) ;
  }

  if (machinestate == kSteam)
  {







    if (lastmachinestatepid != machinestate)
    {
      Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n",150,0,0);
      lastmachinestatepid = machinestate;
    }
    bPID.SetTunings(150, 0, 0, PonE);
  }

  if (machinestate == kCoolDown)
  {
    switch (machine) {

      case QuickMill:
        aggbKp = 150;
        aggbKi = 0;
        aggbKd = 0;
      break;

      default:

        if (aggbTn != 0) {
          aggbKi = aggbKp / aggbTn;
        } else {
          aggbKi = 0;
        }
        aggbKd = aggbTv * aggbKp;
    }

    if (lastmachinestatepid != machinestate)
    {
      Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n",aggbKp,aggbKi,aggbKd);
      lastmachinestatepid = machinestate;
    }
    bPID.SetTunings(aggbKp, aggbKi, aggbKd, PonE) ;
  }

}
# 2512 "/Users/Andreas/Documents/GitHub/ranciliopid/src/rancilio-pid.ino"
int readSysParamsFromStorage(void)
{
  storageGet(STO_ITEM_PID_KP_REGULAR, aggKp);
  storageGet(STO_ITEM_PID_TN_REGULAR, aggTn);
  storageGet(STO_ITEM_PID_TV_REGULAR, aggTv);
  storageGet(STO_ITEM_BREW_SETPOINT, BrewSetPoint);
  storageGet(STO_ITEM_BREW_TIME, brewtime);
  storageGet(STO_ITEM_PRE_INFUSION_TIME, preinfusion);
  storageGet(STO_ITEM_PRE_INFUSION_PAUSE, preinfusionpause);
  storageGet(STO_ITEM_PID_KP_BD, aggbKp);
  storageGet(STO_ITEM_PID_TN_BD, aggbTn);
  storageGet(STO_ITEM_PID_TV_BD, aggbTv);
  storageGet(STO_ITEM_BREW_SW_TIMER, brewtimersoftware);
  storageGet(STO_ITEM_BD_THRESHOLD, brewboarder);
  storageGet(STO_ITEM_PID_KP_START, startKp);
  storageGet(STO_ITEM_PID_TN_START, startTn);

  return 0;
}
# 2539 "/Users/Andreas/Documents/GitHub/ranciliopid/src/rancilio-pid.ino"
int setSteammode(void)
{
  if(SteamON == 0)
  {
    SteamON = 1;
    Serial.printf("Steammode was 0, is 1 now\n");
    return 1;
  }
  if(SteamON == 1)
  {
    SteamON = 0;
    Serial.printf("Steammode was 1, is 0 now\n");
    return 1;
  }
   if ( BLYNK == 1 && Blynk.connected())
   {
      Blynk.virtualWrite(V15, SteamON);
   }

}

int writeSysParamsToStorage(void)
{
  storageSet(STO_ITEM_PID_KP_REGULAR, aggKp);
  storageSet(STO_ITEM_PID_TN_REGULAR, aggTn);
  storageSet(STO_ITEM_PID_TV_REGULAR, aggTv);
  storageSet(STO_ITEM_BREW_SETPOINT, BrewSetPoint);
  storageSet(STO_ITEM_BREW_TIME, brewtime);
  storageSet(STO_ITEM_PRE_INFUSION_TIME, preinfusion);
  storageSet(STO_ITEM_PRE_INFUSION_PAUSE, preinfusionpause);
  storageSet(STO_ITEM_PID_KP_BD, aggbKp);
  storageSet(STO_ITEM_PID_TN_BD, aggbTn);
  storageSet(STO_ITEM_PID_TV_BD, aggbTv);
  storageSet(STO_ITEM_BREW_SW_TIMER, brewtimersoftware);
  storageSet(STO_ITEM_BD_THRESHOLD, brewboarder);
  storageSet(STO_ITEM_PID_KP_START, startKp);
  storageSet(STO_ITEM_PID_TN_START, startTn);

  return storageCommit();
}


int writeSysParamsToBlynk(void)
{
 if ( BLYNK == 1 && Blynk.connected())
 {
  Blynk.virtualWrite(V2, Input);
  Blynk.virtualWrite(V4, aggKp);
  Blynk.virtualWrite(V5, aggTn);
  Blynk.virtualWrite(V6, aggTv);
  Blynk.virtualWrite(V7, BrewSetPoint);
  Blynk.virtualWrite(V8, brewtime);
  Blynk.virtualWrite(V9, preinfusion);
  Blynk.virtualWrite(V10, preinfusionpause);
  Blynk.virtualWrite(V13, pidON);
  Blynk.virtualWrite(V15, SteamON);
  Blynk.virtualWrite(V16, SteamSetPoint);
  Blynk.virtualWrite(V17, setPoint);

  #if (BREWMODE == 2)
    Blynk.virtualWrite(V18, weightSetpoint;
  #endif

  #if (COLDSTART_PID == 2)
    Blynk.virtualWrite(V11, startKp);
    Blynk.virtualWrite(V14, startTn);
  #endif
  }
  return 1;
}