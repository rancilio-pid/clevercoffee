/**
 * @file rancilio-pid.cpp
 *
 * @brief Main sketch
 *
 * @version 3.0.1 Alpha
 */


// Firmware version
#define FW_VERSION    3
#define FW_SUBVERSION 0
#define FW_HOTFIX     1
#define FW_BRANCH     "ALPHA"

// Includes
#include <ArduinoOTA.h>
#include "rancilio-pid.h"
#include "Storage.h"
#include "SysPara.h"
#include "icon.h"        // user icons for display
#include "languages.h"   // for language translation
#include "userConfig.h"  // needs to be configured by the user

// Libraries
#include <Adafruit_VL53L0X.h>   // for ToF Sensor
#include <DallasTemperature.h>  // Library for dallas temp sensor
#include <InfluxDbClient.h>
#include <PubSubClient.h>
#include <U8g2lib.h>    // i2c display
#include <ZACwire.h>    // new TSIC bus library
#include "PID_v1.h"     // for PID calculation
#include "TSIC.h"       // library for TSIC temp sensor

#if defined(ESP8266)
    #include <BlynkSimpleEsp8266.h>
#endif

#if defined(ESP32)
    #include <BlynkSimpleEsp32.h>
    #include <os.h>
    hw_timer_t *timer = NULL;
#endif

#if (BREWMODE == 2 || ONLYPIDSCALE == 1)
    #include <HX711_ADC.h>
#endif

// Version of userConfig need to match, checked by preprocessor
#if (FW_VERSION != USR_FW_VERSION) || \
    (FW_SUBVERSION != USR_FW_SUBVERSION) || \
    (FW_HOTFIX != USR_FW_HOTFIX)
    #error Version of userConfig file and rancilio-pid.cpp need to match!
#endif


MACHINE machine = (enum MACHINE)MACHINEID;

#define HIGH_ACCURACY

#include "PeriodicTrigger.h"
PeriodicTrigger writeDebugTrigger(5000);  // returns true every 5000 ms
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


// Definitions below must be changed in the userConfig.h file
int connectmode = CONNECTMODE;

int Offlinemodus = 0;
const int OnlyPID = ONLYPID;
const int TempSensor = TEMPSENSOR;
const int Brewdetection = BREWDETECTION;
const int triggerType = TRIGGERTYPE;
const int VoltageSensorType = VOLTAGESENSORTYPE;
const boolean ota = OTA;
const int grafana = GRAFANA;
const unsigned long wifiConnectionDelay = WIFICINNECTIONDELAY;
const unsigned int maxWifiReconnects = MAXWIFIRECONNECTS;
const unsigned long brewswitchDelay = BREWSWITCHDELAY;
int BrewMode = BREWMODE;

// Display
uint8_t oled_i2c = OLED_I2C;

// ToF Sensor
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
int calibration_mode = CALIBRATION_MODE;
uint8_t tof_i2c = TOF_I2C;
int water_full = WATER_FULL;
int water_empty = WATER_EMPTY;
unsigned long previousMillisTOF;         // initialisation at the end of init()
const unsigned long intervalTOF = 5000;  // ms
double distance;
double percentage;

// WiFi
const char *hostname = HOSTNAME;
const char *auth = AUTH;
const char *ssid = D_SSID;
const char *pass = PASS;
unsigned long lastWifiConnectionAttempt = millis();
unsigned int wifiReconnects = 0;  // actual number of reconnects

uint8_t softApEnabled = 0;
IPAddress localIp(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
const char *AP_WIFI_SSID = APWIFISSID;
const char *AP_WIFI_KEY = APWIFIKEY;
const unsigned long checkpowerofftime = 30 * 1000;
boolean checklastpoweroffEnabled = false;
boolean softApEnabledcheck = false;
int softApstate = 0;

// OTA
const char *OTAhost = OTAHOST;
const char *OTApass = OTAPASS;

// Blynk
const char *blynkaddress = BLYNKADDRESS;
const int blynkport = BLYNKPORT;
unsigned int blynkReCnctFlag;       // Blynk Reconnection Flag
unsigned int blynkReCnctCount = 0;  // Blynk Reconnection counter
unsigned long lastBlynkConnectionAttempt = millis();

// Backflush values
const unsigned long fillTime = FILLTIME;
const unsigned long flushTime = FLUSHTIME;
int maxflushCycles = MAXFLUSHCYCLES;

// InfluxDB Client
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);
Point sensor("machinestate");
const unsigned long intervalInflux = INTERVALINFLUX;
unsigned long previousMillisInflux;  // initialisation at the end of init()

// Voltage Sensor
unsigned long previousMillisVoltagesensorreading = millis();
const unsigned long intervalVoltagesensor = 200;
int VoltageSensorON, VoltageSensorOFF;

// QuickMill thermoblock steam-mode (only for BREWDETECTION = 3)
const int maxBrewDurationForSteamModeQM_ON = 200;   // if brewtime is shorter steam-mode starts
const int minPVSOffTimedForSteamModeQM_OFF = 1500;  // if PVS-off-time is longer steam-mode ends
unsigned long timePVStoON = 0;      // time pinvoltagesensor switched to ON
unsigned long lastTimePVSwasON = 0; // last time pinvoltagesensor was ON
bool steamQM_active = false;        // steam-mode is active
bool brewSteamDetectedQM = false;   // brew/steam detected, not sure yet what it is
bool coolingFlushDetectedQM = false;

// Pressure sensor
#if (PRESSURESENSOR == 1)   // Pressure sensor connected
    int offset = OFFSET;
    int fullScale = FULLSCALE;
    int maxPressure = MAXPRESSURE;
    float inputPressure = 0;
    const unsigned long intervalPressure = 200;
    unsigned long previousMillisPressure;  // initialisation at the end of init()
#endif

// Method forward declarations
bool mqtt_publish(const char *reading, char *payload);
void setSteamMode(int steamMode);
void setPidStatus(int pidStatus);
void setBackflush(int backflush);
void loopcalibrate();
void looppid();
void initSteamQM();
boolean checkSteamOffQM();
void writeSysParamsToBlynk(void);
void writeSysParamsToMQTT(void);
char *number2string(double in);
char *number2string(float in);
char *number2string(int in);
char *number2string(unsigned int in);
int filter(int input);

// Variable declarations
uint8_t pidON = 1;               // 1 = control loop in closed loop
int relayON, relayOFF;           // used for relay trigger type. Do not change!
boolean kaltstart = true;        // true = Rancilio started for first time
boolean emergencyStop = false;   // Notstop bei zu hoher Temperatur
double EmergencyStopTemp = 120;  // Temp EmergencyStopTemp
int inX = 0, inY = 0, inOld = 0, inSum = 0; // used for filter()
int bars = 0;  // used for getSignalStrength()
boolean brewDetected = 0;
boolean setupDone = false;
int backflushON = 0;      // 1 = activate backflush
int flushCycles = 0;      // number of active flush cycles
int backflushState = 10;  // counter for state machine

// Moving average - brewdetection
const int numReadings = 15;               // number of values per Array
double readingstemp[numReadings];         // the readings from Temp
unsigned long readingstime[numReadings];  // the readings from time
double readingchangerate[numReadings];

int readIndex = 1;           // the index of the current reading
double total = 0;            // total sum of readingchangerate[]
double heatrateaverage = 0;  // the average over the numReadings
double changerate = 0;       // local change rate of temprature
double heatrateaveragemin = 0;
unsigned long timeBrewdetection = 0;
int timerBrewdetection = 0;  // flag is set if brew was detected
int firstreading = 1;        // Ini of the field, also used for sensor check

// PID - values for offline brewdetection
double aggbKp = AGGBKP;
double aggbTn = AGGBTN;
double aggbTv = AGGBTV;

#if aggbTn == 0
    double aggbKi = 0;
#else
    double aggbKi = aggbKp / aggbTn;
#endif

double aggbKd = aggbTv * aggbKp;
double brewtimersoftware = BREW_SW_TIMER; // 20-5 for detection
double brewboarder = BREWDETECTIONLIMIT;  // brew detection limit
const int PonE = PONE;

// Brewing, 1 = Normale Prefinfusion , 2 = Scale & Shottimer = 2
#include "brewscaleini.h"

// Sensor check
boolean sensorError = false;
int error = 0;
int maxErrorCounter = 10;  // depends on intervaltempmes* , define max seconds for invalid data

// PID controller
unsigned long previousMillistemp;  // initialisation at the end of init()
const unsigned long intervaltempmestsic = 400;
const unsigned long intervaltempmesds18b20 = 400;
int pidMode = 1;  // 1 = Automatic, 0 = Manual

const unsigned int windowSize = 1000;
unsigned int isrCounter = 0;  // counter for ISR
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

double aggKd = aggTv * aggKp;

// Timer - ISR for PID calculation and heat realay output
#include "ISR.h"

PID bPID(&Input, &Output, &setPoint, aggKp, aggKi, aggKd, PonE, DIRECT);

// Dallas temp sensor
OneWire oneWire(ONE_WIRE_BUS);  // Setup a oneWire instance to communicate with any OneWire
                                // devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire);    // Pass our oneWire reference to Dallas Temperature.
DeviceAddress sensorDeviceAddress;      // arrays to hold device address

// TSIC 306 temp sensor
uint16_t temperature = 0;   // internal variable used to read temeprature
float Temperature_C = 0;    // internal variable that holds the converted temperature in °C

#if (ONE_WIRE_BUS == 16 && TEMPSENSOR == 2 && defined(ESP8266))
    TSIC Sensor1(ONE_WIRE_BUS);  // only Signalpin, VCCpin unused by default
#else
    ZACwire Sensor2(ONE_WIRE_BUS, 306);  // set OneWire pin to receive signal from the TSic "306"
#endif

// Blynk update Interval
unsigned long previousMillisBlynk;  // initialisation at the end of init()
unsigned long previousMillisMQTT;   // initialisation at the end of init()
const unsigned long intervalBlynk = 1000;
const unsigned long intervalMQTT = 5000;
int blynksendcounter = 1;

// MQTT
WiFiClient net;
PubSubClient mqtt(net);
const char *mqtt_server_ip = MQTT_SERVER_IP;
const int mqtt_server_port = MQTT_SERVER_PORT;
const char *mqtt_username = MQTT_USERNAME;
const char *mqtt_password = MQTT_PASSWORD;
const char *mqtt_topic_prefix = MQTT_TOPIC_PREFIX;
char topic_will[256];
char topic_set[256];
unsigned long lastMQTTConnectionAttempt = millis();
unsigned int MQTTReCnctFlag;       // Blynk Reconnection Flag
unsigned int MQTTReCnctCount = 0;  // Blynk Reconnection counter


// system parameters (current value as pointer to variable, minimum, maximum, optional storage ID)
SysParaClass<double> sysParaPidKpStart(&startKp, 0, 100, STO_ITEM_PID_KP_START);
SysParaClass<double> sysParaPidTnStart(&startTn, 0, 999, STO_ITEM_PID_TN_START);
SysParaClass<double> sysParaPidKpReg(&aggKp, 0, 100, STO_ITEM_PID_KP_REGULAR);
SysParaClass<double> sysParaPidTnReg(&aggTn, 0, 999, STO_ITEM_PID_TN_REGULAR);
SysParaClass<double> sysParaPidTvReg(&aggTv, 0, 999, STO_ITEM_PID_TV_REGULAR);
SysParaClass<double> sysParaPidKpBd(&aggbKp, 0, 100, STO_ITEM_PID_KP_BD);
SysParaClass<double> sysParaPidTnBd(&aggbTn, 0, 999, STO_ITEM_PID_TN_BD);
SysParaClass<double> sysParaPidTvBd(&aggbTv, 0, 999, STO_ITEM_PID_TV_BD);
SysParaClass<double> sysParaBrewSetPoint(&BrewSetPoint, 89, 105, STO_ITEM_BREW_SETPOINT);
SysParaClass<double> sysParaBrewTime(&brewtime, 0, 60, STO_ITEM_BREW_TIME);
SysParaClass<double> sysParaBrewSwTimer(&brewtimersoftware, 0, 999, STO_ITEM_BREW_SW_TIMER);
SysParaClass<double> sysParaBrewThresh(&brewboarder, 0, 999, STO_ITEM_BD_THRESHOLD);
SysParaClass<double> sysParaPreInfTime(&preinfusion, 0, 10, STO_ITEM_PRE_INFUSION_TIME);
SysParaClass<double> sysParaPreInfPause(&preinfusionpause, 0, 20, STO_ITEM_PRE_INFUSION_PAUSE);
SysParaClass<double> sysParaWeightSetPoint(&weightSetpoint, 0, 500, STO_ITEM_WEIGHTSETPOINT);
SysParaClass<uint8_t> sysParaPidOn(&pidON, 0, 1, STO_ITEM_PID_ON);


enum MQTTSettableType {
    tUInt8,
    tDouble,
};


struct mqttVars_t {
    String mqttParamName;
    MQTTSettableType type;
    void *mqttVarPtr;
};

std::vector<mqttVars_t> mqttVars = {
    {"BrewSetPoint", tDouble, (void *)&BrewSetPoint},
    {"brewtime", tDouble, (void *)&brewtime},
    {"preinfusion", tDouble, (void *)&preinfusion},
    {"preinfusionpause", tDouble, (void *)&preinfusionpause},
    {"pidON", tUInt8, (void *)&pidON},
    {"backflushON", tUInt8, (void *)&backflushON},
};

// Embedded HTTP Server
#include "RancilioServer.h"

std::vector<editable_t> editableVars = {
    {"PID_ON", "PID on?", kUInt8, (void *)&pidON},  // ummm, why isn't pidON a boolean?
    {"PID_KP", "PID P", kDouble, (void *)&aggKp},
    {"PID_TN", "PID I", kDouble, (void *)&aggTn},
    {"PID_TV", "PID D", kDouble, (void *)&aggTv},
    {"TEMP", "Temperature", kDouble, (void *)&Input},
    {"BREW_SET_POINT", "Set point (°C)", kDouble, (void *)&BrewSetPoint},
    {"BREW_TIME", "Brew Time (s)", kDouble, (void *)&brewtime},
    {"BREW_PREINFUSION", "Preinfusion Time (s)", kDouble, (void *)&preinfusion},
    {"BREW_PREINFUSUINPAUSE", "Pause (s)", kDouble, (void *)&preinfusionpause},
    {"PID_BD_KP", "BD P", kDouble, (void *)&aggbKp},
    {"PID_BD_TN", "BD I", kDouble, (void *)&aggbTn},
    {"PID_BD_TV", "BD D", kDouble, (void *)&aggbTv},
    {"PID_BD_TIMER", "PID BD Time (s)", kDouble, (void *)&brewtimersoftware},
    {"PID_BD_BREWBOARDER", "PID BD Sensitivity", kDouble, (void *)&brewboarder},
    {"AP_WIFI_SSID", "AP WiFi Name", kCString, (void *)AP_WIFI_SSID},
    {"AP_WIFI_KEY", "AP WiFi Password", kCString, (void *)AP_WIFI_KEY},
    {"START_KP", "Start P", kDouble, (void *)&startKp},
    {"START_TN", "Start I", kDouble, (void *)&startTn},
    {"STEAM_MODE", "Steam Mode", rInteger, (void *)&SteamON},
    {"BACKFLUSH_ON", "Backflush", rInteger, (void *)&backflushON},
    {"WEIGHTSETPOINT", "Brew weight setpoint (g)",kDouble, (void *)&weightSetpoint},
};

unsigned long lastTempEvent = 0;
unsigned long tempEventInterval = 1000;

/**
 * @brief Get Wifi signal strength and set bars for display
 */
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

// Display define & template
#if OLED_DISPLAY == 1
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);  // e.g. 1.3"
#endif
#if OLED_DISPLAY == 2
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);  // e.g. 0.96"
#endif

// Update for Display
unsigned long previousMillisDisplay;  // initialisation at the end of init()
const unsigned long intervalDisplay = 500;

// Horizontal or vertical display
#if (OLED_DISPLAY == 1 || OLED_DISPLAY == 2)
    #if (DISPLAYTEMPLATE < 20)  // horizontal templates
        #include "display.h"
    #endif

    #if (DISPLAYTEMPLATE >= 20)  // vertical templates
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

/**
 * @brief Create software WiFi AP
 */
void createSoftAp() {
    if (softApEnabledcheck == false) {
        WiFi.enableAP(true);
        WiFi.softAP(AP_WIFI_SSID, AP_WIFI_KEY);
        WiFi.softAPConfig(localIp, gateway, subnet);

        softApEnabledcheck = true;
        Serial.println("Set softApEnabled: 1, Setup AP MODE\n");

        uint8_t eepromvalue = 0;
        storageSet(STO_ITEM_SOFT_AP_ENABLED_CHECK, eepromvalue, true);
        softApstate = 0;
        Serial.printf(
            "AccessPoint created with SSID %s and KEY %s and OTA Flash via "
            "http://%i.%i.%i.%i/\r\n",
            AP_WIFI_SSID, AP_WIFI_KEY, WiFi.softAPIP()[0], WiFi.softAPIP()[1],
            WiFi.softAPIP()[2], WiFi.softAPIP()[3]);

        #if (OLED_DISPLAY != 0)
            displayMessage("Setup-MODE: SSID:", String(AP_WIFI_SSID), "KEY:", String(AP_WIFI_KEY), "IP:", "192.168.1.1");
        #endif
    }

    yield();
}

void stopSoftAp() {
    Serial.println("Closing AccesPoint");
    WiFi.enableAP(false);
}

void checklastpoweroff() {
    storageGet(STO_ITEM_SOFT_AP_ENABLED_CHECK, softApEnabled);

    Serial.printf("softApEnabled: %i\n", softApEnabled);

    if (softApEnabled != 1) {
        Serial.printf("Set softApEnabled: 1, was 0\n");
        uint8_t eepromvalue = 1;
        storageSet(STO_ITEM_SOFT_AP_ENABLED_CHECK, eepromvalue);
    }

    storageCommit();
}

void setchecklastpoweroff() {
    if (millis() > checkpowerofftime && checklastpoweroffEnabled == false) {
        Serial.printf("Set softApEnabled 0 after checkpowerofftime\n");
        uint8_t eepromvalue = 0;
        storageSet(STO_ITEM_SOFT_AP_ENABLED_CHECK, eepromvalue, true);
        checklastpoweroffEnabled = true;
        storageCommit();
    }
}

// Blynk define pins and read values
BLYNK_CONNECTED() {
    if (Offlinemodus == 0 && BLYNK == 1) {
        Blynk.syncAll();
    }
}

BLYNK_WRITE(V4) { aggKp = param.asDouble(); }

BLYNK_WRITE(V5) { aggTn = param.asDouble(); }

BLYNK_WRITE(V6) { aggTv = param.asDouble(); }

BLYNK_WRITE(V7) { BrewSetPoint = param.asDouble(); }

BLYNK_WRITE(V8) { brewtime = param.asDouble(); }

BLYNK_WRITE(V9) { preinfusion = param.asDouble(); }

BLYNK_WRITE(V10) { preinfusionpause = param.asDouble(); }

BLYNK_WRITE(V13) { pidON = param.asInt(); }

BLYNK_WRITE(V15) {
    SteamON = param.asInt();

    if (SteamON == 1) {
        SteamFirstON = 1;
    }

    if (SteamON == 0) {
        SteamFirstON = 0;
    }
}

BLYNK_WRITE(V16) { SteamSetPoint = param.asDouble(); }

#if (BREWMODE == 2)
    BLYNK_WRITE(V18) { weightSetpoint = param.asFloat(); }
#endif

BLYNK_WRITE(V25) { calibration_mode = param.asInt(); }

BLYNK_WRITE(V26) { water_empty = param.asInt(); }

BLYNK_WRITE(V27) { water_full = param.asInt(); }

BLYNK_WRITE(V30) { aggbKp = param.asDouble(); }

BLYNK_WRITE(V31) { aggbTn = param.asDouble(); }

BLYNK_WRITE(V32) { aggbTv = param.asDouble(); }

BLYNK_WRITE(V33) { brewtimersoftware = param.asDouble(); }

BLYNK_WRITE(V34) { brewboarder = param.asDouble(); }

BLYNK_WRITE(V40) { backflushON = param.asInt(); }

#if (COLDSTART_PID == 2)  // Blynk values, else default starttemp from config
    BLYNK_WRITE(V11) { startKp = param.asDouble(); }

    BLYNK_WRITE(V14) { startTn = param.asDouble(); }
#endif

#if (PRESSURESENSOR == 1)  // Pressure sensor connected
    /**
     * Pressure sensor
     * Verify before installation: meassured analog input value (should be 3,300 V
     * for 3,3 V supply) and respective ADC value (3,30 V = 1023)
     */
    void checkPressure() {
        float inputPressureFilter = 0;
        unsigned long currentMillisPressure = millis();

        if (currentMillisPressure - previousMillisPressure >= intervalPressure) {
            previousMillisPressure = currentMillisPressure;

            inputPressure =
                ((analogRead(PINPRESSURESENSOR) - offset) * maxPressure * 0.0689476) /
                (fullScale - offset);   // pressure conversion and unit
                                        // conversion [psi] -> [bar]
            inputPressureFilter = filter(inputPressure);

            Serial.printf("pressure raw: %f\n", inputPressure);
            Serial.printf("pressure filtered: %f\n", inputPressureFilter);
        }
    }
#endif

// Trigger for Rancilio E Machine
unsigned long previousMillisETrigger;  // initialisation at the end of init()
const unsigned long intervalETrigger = ETRIGGERTIME;  // in Seconds
int relayETriggerON, relayETriggerOFF;

// Emergency stop if temp is too high
void testEmergencyStop() {
    if (Input > EmergencyStopTemp && emergencyStop == false) {
        emergencyStop = true;
    } else if (Input < 100 && emergencyStop == true) {
        emergencyStop = false;
    }
}

/**
 * @brief Moving average - brewdetection (SW)
 */
void movAvg() {
    if (firstreading == 1) {
        for (int thisReading = 0; thisReading < numReadings; thisReading++) {
            readingstemp[thisReading] = Input;
            readingstime[thisReading] = 0;
            readingchangerate[thisReading] = 0;
        }

        firstreading = 0;
    }

    readingstime[readIndex] = millis();
    readingstemp[readIndex] = Input;

    if (readIndex == numReadings - 1) {
        changerate = (readingstemp[numReadings - 1] - readingstemp[0]) /
                        (readingstime[numReadings - 1] - readingstime[0]) * 10000;
    } else {
        changerate = (readingstemp[readIndex] - readingstemp[readIndex + 1]) /
                        (readingstime[readIndex] - readingstime[readIndex + 1]) *
                        10000;
    }

    readingchangerate[readIndex] = changerate;
    total = 0;

    for (int i = 0; i < numReadings; i++) {
        total += readingchangerate[i];
    }

    heatrateaverage = total / numReadings * 100;

    if (heatrateaveragemin > heatrateaverage) {
        heatrateaveragemin = heatrateaverage;
    }

    if (readIndex >= numReadings - 1) {
        // ...wrap around to the beginning:
        readIndex = 0;
    } else {
        readIndex++;
    }
}

/**
 * @brief check sensor value.
 * @return If < 0 or difference between old and new >25, then increase error.
 *      If error is equal to maxErrorCounter, then set sensorError
 */
boolean checkSensor(float tempInput) {
    boolean sensorOK = false;
    boolean badCondition = (tempInput < 0 || tempInput > 150 || fabs(tempInput - previousInput) > 5);

    if (badCondition && !sensorError) {
        error++;
        sensorOK = false;

        if (error >= 5) {  // warning after 5 times error
            Serial.printf(
                "*** WARNING: temperature sensor reading: consec_errors = %i, "
                "temp_current = %.1f\n",
                error, tempInput);
        }
    } else if (badCondition == false && sensorOK == false) {
        error = 0;
        sensorOK = true;
    }

    if (error >= maxErrorCounter && !sensorError) {
        sensorError = true;
        Serial.printf(
            "*** ERROR: temperature sensor malfunction: temp_current = %.1f\n",
            tempInput);
    } else if (error == 0 && sensorError) {
        sensorError = false;
    }

    return sensorOK;
}

/**
 * @brief Refresh temperature.
 *      Each time checkSensor() is called to verify the value.
 *      If the value is not valid, new data is not stored.
 */
void refreshTemp() {
  unsigned long currentMillistemp = millis();
  previousInput = Input;

    if (TempSensor == 1) {
        if (currentMillistemp - previousMillistemp >= intervaltempmesds18b20) {
            previousMillistemp = currentMillistemp;
            sensors.requestTemperatures();

            if (!checkSensor(sensors.getTempCByIndex(0)) && firstreading == 0)
                return; // if sensor data is not valid, abort function; Sensor must
                        // be read at least one time at system startup

            Input = sensors.getTempCByIndex(0);

            if (Brewdetection != 0) {
                movAvg();
            } else if (firstreading != 0) {
                firstreading = 0;
            }
        }
    }

    if (TempSensor == 2) {
        if (currentMillistemp - previousMillistemp >= intervaltempmestsic) {
            previousMillistemp = currentMillistemp;

            /* variable "temperature" must be set to zero, before reading new
            * data getTemperature only updates if data is valid, otherwise
            * "temperature" will still hold old values
            */
            temperature = 0;

        #if (ONE_WIRE_BUS == 16 && defined(ESP8266))
            Sensor1.getTemperature(&temperature);
            Temperature_C = Sensor1.calc_Celsius(&temperature);
        #endif

        #if ((ONE_WIRE_BUS != 16 && defined(ESP8266)) || defined(ESP32))
            Temperature_C = Sensor2.getTemp();
        #endif

        if (!checkSensor(Temperature_C) && firstreading == 0)
            return; // if sensor data is not valid, abort function; Sensor must
                    // be read at least one time at system startup

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

/**
 * @brief Switch to offline mode if maxWifiReconnects were exceeded during boot
 */
void initOfflineMode() {
    #if OLED_DISPLAY != 0
        displayMessage("", "", "", "", "Begin Fallback,", "No Wifi");
    #endif

    Serial.println("Start offline mode with eeprom values, no wifi :(");
    Offlinemodus = 1;

    if (readSysParamsFromStorage() != 0) {
        #if OLED_DISPLAY != 0
            displayMessage("", "", "", "", "No eeprom,", "Values");
        #endif

        Serial.println(
            "No working eeprom value, I am sorry, but use default offline value "
            ":)");

        delay(1000);
    }
}

/**
 * @brief Check if Wifi is connected, if not reconnect abort function if offline, or brew is running
 */
void checkWifi() {
    if (Offlinemodus == 1 || brewcounter > 11) return;

    /* if kaltstart ist still true when checkWifi() is called, then there was no WIFI connection
     * at boot -> connect or offlinemode
     */
    do {
        if ((millis() - lastWifiConnectionAttempt >= wifiConnectionDelay) && (wifiReconnects <= maxWifiReconnects)) {
            int statusTemp = WiFi.status();

            if (statusTemp != WL_CONNECTED) {  // check WiFi connection status
                lastWifiConnectionAttempt = millis();
                wifiReconnects++;
                Serial.printf("Attempting WIFI reconnection: %i\n", wifiReconnects);

                if (!setupDone) {
                    #if OLED_DISPLAY != 0
                        displayMessage("", "", "", "", langstring_wifirecon, String(wifiReconnects));
                    #endif
                }

                WiFi.disconnect();
                WiFi.begin(ssid, pass);  // attempt to connect to Wifi network

                int count = 1;

                while (WiFi.status() != WL_CONNECTED && count <= 20) {
                    delay(100); // give WIFI some time to connect
                    count++;    // reconnect counter, maximum waiting time for
                                // reconnect = 20*100ms
                }
            }
        }

        yield();  // Prevent WDT trigger
    } while (!setupDone && wifiReconnects < maxWifiReconnects && WiFi.status() != WL_CONNECTED);


    if (wifiReconnects >= maxWifiReconnects && !setupDone) {  // no wifi connection after boot, initiate offline mode
        // (only directly after boot)
        initOfflineMode();
    }
}

void sendInflux() {
    unsigned long currentMillisInflux = millis();

    if (currentMillisInflux - previousMillisInflux >= intervalInflux) {
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

        // Write point
        if (!client.writePoint(sensor)) {
            Serial.printf("InfluxDB write failed: %s\n", client.getLastErrorMessage().c_str());
        }
    }
}

/**
 * @brief Check if Blynk is connected, if not reconnect abort function if offline, or brew is running
 *      blynk is also using maxWifiReconnects!
 */
void checkBlynk() {
    if (Offlinemodus == 1 || BLYNK == 0 || brewcounter > 11) return;

    if ((millis() - lastBlynkConnectionAttempt >= wifiConnectionDelay) && (blynkReCnctCount <= maxWifiReconnects)) {
        int statusTemp = Blynk.connected();

        if (statusTemp != 1) {
            lastBlynkConnectionAttempt = millis();  // Reconnection Timer Function
            blynkReCnctCount++;                     // Increment reconnection Counter
            Serial.printf("Attempting blynk reconnection: %i\n", blynkReCnctCount);
            Blynk.connect(3000);    // Try to reconnect to the server; connect() is
                                    // a blocking function, watch the timeout!
        }
    }
}

/**
 * @brief Check if MQTT is connected, if not reconnect abort function if offline, or brew is running
 *      MQTT is also using maxWifiReconnects!
 */
void checkMQTT() {
    if (Offlinemodus == 1 || brewcounter > 11) return;

    if ((millis() - lastMQTTConnectionAttempt >= wifiConnectionDelay) && (MQTTReCnctCount <= maxWifiReconnects)) {
        int statusTemp = mqtt.connected();

        if (statusTemp != 1) {
            lastMQTTConnectionAttempt = millis();  // Reconnection Timer Function
            MQTTReCnctCount++;                     // Increment reconnection Counter
            Serial.printf("Attempting MQTT reconnection: %i\n", MQTTReCnctCount);

            if (mqtt.connect(hostname, mqtt_username, mqtt_password, topic_will, 0, 0,"exit") == true) {
                mqtt.subscribe(topic_set);
                Serial.println("Subscribe to MQTT Topics");
            }   // Try to reconnect to the server; connect() is a blocking
                // function, watch the timeout!
        }
    }
}

char number2string_double[22];

char *number2string(double in) {
    snprintf(number2string_double, sizeof(number2string_double), "%0.2f", in);
    return number2string_double;
}

char number2string_float[22];

char *number2string(float in) {
    snprintf(number2string_float, sizeof(number2string_float), "%0.2f", in);
    return number2string_float;
}

char number2string_int[22];

char *number2string(int in) {
    snprintf(number2string_int, sizeof(number2string_int), "%d", in);
    return number2string_int;
}

char number2string_uint[22];

char *number2string(unsigned int in) {
  snprintf(number2string_uint, sizeof(number2string_uint), "%u", in);
  return number2string_uint;
}

/**
 * @brief Publish Data to MQTT
 */
bool mqtt_publish(const char *reading, char *payload) {
    #if MQTT
        char topic[120];
        snprintf(topic, 120, "%s%s/%s", mqtt_topic_prefix, hostname, reading);
        return mqtt.publish(topic, payload, true);
    #else
        return false;
    #endif
}

/**
 * @brief Send data to Blynk server
 */
void sendToBlynkMQTT() {
    if (Offlinemodus == 1) return;

    unsigned long currentMillisBlynk = millis();

    if ((currentMillisBlynk - previousMillisBlynk >= intervalBlynk) && (BLYNK == 1)) {
        previousMillisBlynk = currentMillisBlynk;

        if (Blynk.connected()) {
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
        } else if (blynksendcounter >= 6) {
            blynksendcounter = 0;
        }

        blynksendcounter++;
        }
    }
}

/**
 * @brief Brewdetection
 */
void brewdetection() {
    if (brewboarder == 0) return;  // abort brewdetection if deactivated

    // Brew detecion: 1 = software solution, 2 = hardware, 3 = voltage sensor
    if (Brewdetection == 1) {
        if (timerBrewdetection == 1) {
            brewTime = millis() - timeBrewdetection;
        }

        // deactivate brewtimer after end of brewdetection pid
        if (millis() - timeBrewdetection > brewtimersoftware * 1000 && timerBrewdetection == 1) {
            timerBrewdetection = 0;  // rearm brewdetection

            if (machinestate != 30) { // if Onlypid = 1, brewTime > 0, no reset of brewTime in case of brewing.
                brewTime = 0;
            }
        }
    } else if (Brewdetection == 2) {
        if (millis() - timeBrewdetection > brewtimersoftware * 1000 && timerBrewdetection == 1) {
            timerBrewdetection = 0;  // rearm brewdetection
        }
    } else if (Brewdetection == 3) {
        // brewTime counter
        if ((digitalRead(PINVOLTAGESENSOR) == VoltageSensorON) && brewDetected == 1) {
            brewTime = millis() - startingTime;
            lastbrewTime = brewTime;
        }

        //  OFF: reset brew
        if ((digitalRead(PINVOLTAGESENSOR) == VoltageSensorOFF) &&(brewDetected == 1 || coolingFlushDetectedQM == true)) {
            brewDetected = 0;
            timePVStoON = brewTime;  // for QuickMill
            brewTime = 0;
            startingTime = 0;
            coolingFlushDetectedQM = false;
            Serial.println("HW Brew - Voltage Sensor - End");
        }

        if (millis() - timeBrewdetection > brewtimersoftware * 1000 && timerBrewdetection == 1) {  // reset PID Brew
            timerBrewdetection = 0; // rearm brewdetection
        }
    }

    // Activate brew detection
    if (Brewdetection == 1) {  // SW BD
        // BD PID only +/- 4 Grad Celsius, no detection if HW was active
        if (heatrateaverage <= -brewboarder && timerBrewdetection == 0 && (fabs(Input - BrewSetPoint) < 5)) {
            Serial.println("SW Brew detected");
            timeBrewdetection = millis();
            timerBrewdetection = 1;
        }
    } else if (Brewdetection == 2) {  // HW BD
        if (brewcounter > 10 && brewDetected == 0 && brewboarder != 0) {
            Serial.println("HW Brew detected");
            timeBrewdetection = millis();
            timerBrewdetection = 1;
            brewDetected = 1;
        }
    } else if (Brewdetection == 3) {  // voltage sensor
        switch (machine) {
            case QuickMill:

                if (!coolingFlushDetectedQM) {
                    int pvs = digitalRead(PINVOLTAGESENSOR);

                    if (pvs == VoltageSensorON && brewDetected == 0 &&
                        brewSteamDetectedQM == 0 && !steamQM_active) {
                        timeBrewdetection = millis();
                        timePVStoON = millis();
                        timerBrewdetection = 1;
                        brewDetected = 0;
                        lastbrewTime = 0;
                        brewSteamDetectedQM = 1;
                        Serial.println("Quick Mill: setting brewSteamDetectedQM = 1");
                        logbrew.reset();
                    }

                    if (brewSteamDetectedQM == 1) {
                        if (pvs == VoltageSensorOFF) {
                            brewSteamDetectedQM = 0;

                            if (millis() - timePVStoON < maxBrewDurationForSteamModeQM_ON) {
                                Serial.println("Quick Mill: steam-mode detected");
                                initSteamQM();
                            } else {
                                Serial.printf("*** ERROR: QuickMill: neither brew nor steam\n");
                            }
                        } else if (millis() - timePVStoON > maxBrewDurationForSteamModeQM_ON) {
                            if (Input < BrewSetPoint + 2) {
                                Serial.println("Quick Mill: brew-mode detected");
                                startingTime = timePVStoON;
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

            // no Quickmill:
            default:
                previousMillisVoltagesensorreading = millis();

                if (digitalRead(PINVOLTAGESENSOR) == VoltageSensorON && brewDetected == 0) {
                    Serial.println("HW Brew - Voltage Sensor -  Start");
                    timeBrewdetection = millis();
                    startingTime = millis();
                    timerBrewdetection = 1;
                    brewDetected = 1;
                    lastbrewTime = 0;
                }
        }
    }
}

/**
 * @brief after ~28 cycles the input is set to 99,66% if the real input value sum of inX and inY
 *      multiplier must be 1 increase inX multiplier to make the filter faster
 */
int filter(int input) {
    inX = input * 0.3;
    inY = inOld * 0.7;
    inSum = inX + inY;
    inOld = inSum;

    return inSum;
}

/**
 * @brief Assign the value of the mqtt parameter to the associated variable
 *
 * @param param MQTT parameter name
 * @param value MQTT value
 */
void assignMQTTParam(char *param, double value) {
    String key = String(param);
    boolean paramExists = false;

    for (mqttVars_t m : mqttVars) {
        if (m.mqttParamName.equals(key)) {
            switch (m.type) {
                case tDouble:
                    *(double *)m.mqttVarPtr = value;
                    break;
                case tUInt8:
                    *(uint8_t *)m.mqttVarPtr = value;
                    break;
            }

            paramExists = true;
            break;
        }
    }

    if (paramExists) {
        mqtt_publish(param, number2string(value));
        writeSysParamsToBlynk();
    }
    else {
        Serial.printf("%s is not a valid MQTT parameter.", param);
    }
}

/**
 * @brief MQTT Callback Function: set Parameters through MQTT
 */
void mqtt_callback(char *topic, byte *data, unsigned int length) {
    char topic_str[256];
    os_memcpy(topic_str, topic, sizeof(topic_str));
    topic_str[255] = '\0';
    char data_str[length + 1];
    os_memcpy(data_str, data, length);
    data_str[length] = '\0';
    char topic_pattern[255];
    char configVar[120];
    char cmd[64];
    double data_double;

    snprintf(topic_pattern, sizeof(topic_pattern), "%s%s/%%[^\\/]/%%[^\\/]", mqtt_topic_prefix, hostname);
    Serial.println(topic_pattern);

    if ((sscanf(topic_str, topic_pattern, &configVar, &cmd) != 2) || (strcmp(cmd, "set") != 0)) {
        Serial.println(topic_str);
        return;
    }

    Serial.println(topic_str);
    Serial.println(data_str);

    sscanf(data_str, "%lf", &data_double);

    assignMQTTParam(configVar, data_double);
}

/**
 * @brief E-Trigger for Silvia E
 */
void handleETrigger() {
    // Static variable only one time is 0
    static int ETriggeractive = 0;
    unsigned long currentMillisETrigger = millis();

    if (ETRIGGER == 1) {  // E Trigger is active from userconfig
        if (currentMillisETrigger - previousMillisETrigger >= (1000 * intervalETrigger))  { // s to ms * 1000
            ETriggeractive = 1;
            previousMillisETrigger = currentMillisETrigger;

            digitalWrite(PINETRIGGER, relayETriggerON);
        }

        // 10 Seconds later
        else if (ETriggeractive == 1 && previousMillisETrigger + (10 * 1000) < (currentMillisETrigger)) {
            digitalWrite(PINETRIGGER, relayETriggerOFF);
            ETriggeractive = 0;
        }
    }
}

/**
 * @brief SteamON & Quickmill
 */
void checkSteamON() {
    // check digital GIPO
    if (digitalRead(PINSTEAMSWITCH) == HIGH) {
        SteamON = 1;
    }

    // if via blynk on, then SteamFirstON == 1, prevent override
    if (digitalRead(PINSTEAMSWITCH) == LOW && SteamFirstON == 0) {
        SteamON = 0;
    }

    // monitor QuickMill thermoblock steam-mode
    if (machine == QuickMill) {
        if (steamQM_active == true) {
            if (checkSteamOffQM() == true) {  // if true: steam-mode can be turned off
                SteamON = 0;
                steamQM_active = false;
                lastTimePVSwasON = 0;
            } else {
                SteamON = 1;
            }
        }
    }

    if (SteamON == 1) {
        setPoint = SteamSetPoint;
    }

    if (SteamON == 0) {
        setPoint = BrewSetPoint;
    }
}

void setEmergencyStopTemp() {
    if (machinestate == kSteam || machinestate == kCoolDown) {
        if (EmergencyStopTemp != 145) EmergencyStopTemp = 145;
    } else {
        if (EmergencyStopTemp != 120) EmergencyStopTemp = 120;
    }
}

void initSteamQM() {
    // Initialize monitoring for steam switch off for QuickMill thermoblock
    lastTimePVSwasON = millis();  // time when pinvoltagesensor changes from ON to OFF
    steamQM_active = true;
    timePVStoON = 0;
    SteamON = 1;
}

boolean checkSteamOffQM() {
    /* Monitor pinvoltagesensor during active steam mode of QuickMill
     * thermoblock. Once the pinvolagesenor remains OFF for longer than a
     * pump-pulse time peride the switch is turned off and steam mode finished.
     */
    if (digitalRead(PINVOLTAGESENSOR) == VoltageSensorON) {
        lastTimePVSwasON = millis();
    }

    if ((millis() - lastTimePVSwasON) > minPVSOffTimedForSteamModeQM_OFF) {
        lastTimePVSwasON = 0;
        return true;
    }

    return false;
}

/**
 * @brief State machine
 */
void machinestatevoid() {
    switch (machinestate) {
        case kInit:
            // Prevent coldstart leave by Input 222
            if (Input < (BrewSetPoint - 1) || Input < 150) {
                machinestate = kColdStart;
                Serial.println(Input);
                Serial.println(machinestate);

                // some users have 100 % Output in kInit / Koldstart, reset PID
                pidMode = 0;
                bPID.SetMode(pidMode);
                Output = 0;
                digitalWrite(PINHEATER, LOW);  // Stop heating

                // start PID
                pidMode = 1;
                bPID.SetMode(pidMode);
            }

            if (pidON == 0) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kColdStart:
            /* One high Input let the state jump to 19.
            * switch (machinestatecold) prevent it, we wait 10 sec with new state.
            * during the 10 sec the Input has to be Input >= (BrewSetPoint-1),
            * If not, reset machinestatecold
            */
            switch (machinestatecold) {
                case 0:
                    if (Input >= (BrewSetPoint - 1) && Input < 150) {
                        machinestatecoldmillis = millis();  // get millis for interval calc
                        machinestatecold = 10;              // new state
                        Serial.println(
                            "Input >= (BrewSetPoint-1), wait 10 sec before machinestate "
                            "19");
                    }
                    break;

                case 10:
                    if (Input < (BrewSetPoint - 1)) {
                        machinestatecold = 0;  //  Input was only one time above
                                            //  BrewSetPoint, reset machinestatecold
                        Serial.println(
                            "Reset timer for machinestate 19: Input < (BrewSetPoint-1)");
                    }

                    if (machinestatecoldmillis + 10 * 1000 <
                        millis())  // 10 sec Input above BrewSetPoint, no set new state
                    {
                        machinestate = kSetPointNegative;
                        Serial.println(
                            "10 sec Input >= (BrewSetPoint-1) finished, switch to state "
                            "19");
                    }
                    break;
            }

            if (SteamON == 1) {
                machinestate = kSteam;
            }

            if ((brewTime > 0 && ONLYPID == 1) ||  // brewTime with Only PID
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)) {
                machinestate = kBrew;
            }

            if (SteamON == 1) {
                machinestate = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machinestate = kBackflush;
            }

            if (pidON == 0) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        // Setpoint -1 Celsius
        case kSetPointNegative:
            brewdetection();  // if brew detected, set PID values

            if (Input >= (BrewSetPoint)) {
                machinestate = kPidNormal;
            }

            if ((brewTime > 0 && ONLYPID == 1) ||  // brewTime with Only PID
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)) {
                machinestate = kBrew;
            }

            if (SteamON == 1) {
                machinestate = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machinestate = kBackflush;
            }

            if (SteamON == 1) {
                machinestate = kSteam;
            }

            if (pidON == 0) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kPidNormal:
            brewdetection();  // if brew detected, set PID values

            if ((brewTime > 0 && ONLYPID == 1) ||  // brewTime with Only PID
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)) {
                machinestate = kBrew;
            }

            if (SteamON == 1) {
                machinestate = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machinestate = kBackflush;
            }

            if (emergencyStop) {
                machinestate = kEmergencyStop;
            }

            if (pidON == 0) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kBrew:
            brewdetection();
            // Ausgabe waehrend des Bezugs von Bruehzeit, Temp und heatrateaverage
            if (logbrew.check())
                Serial.printf("(tB,T,hra) --> %5.2f %6.2f %8.2f\n",
                            (double)(millis() - startingTime) / 1000, Input,
                            heatrateaverage);

            if ((brewTime > 35 * 1000 && Brewdetection == 1 &&
                ONLYPID == 1) ||  // 35 sec later and BD PID active SW Solution
                (brewTime == 0 && Brewdetection == 3 &&
                ONLYPID == 1) ||  // Voltagesensor reset brewTime == 0
                ((brewcounter == 10 || brewcounter == 43) && ONLYPID == 0)) {
                if ((ONLYPID == 1 && Brewdetection == 3) ||
                    ONLYPID ==
                        0) {  // only delay of shotimer for voltagesensor or brewcounter
                    machinestate = kShotTimerAfterBrew;
                    lastbrewTimeMillis = millis();  // for delay
                }

                if (ONLYPID == 1 && Brewdetection == 1 && timerBrewdetection == 1) {  // direct to PID BD
                    machinestate = kBrewDetectionTrailing;
                }
            }

            if (SteamON == 1) {
                machinestate = kSteam;
            }

            if (emergencyStop) {
                machinestate = kEmergencyStop;
            }

            if (pidON == 0) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kShotTimerAfterBrew:
            brewdetection();

            if (millis() - lastbrewTimeMillis > BREWSWITCHDELAY) {
                Serial.printf("Bezugsdauer: %4.1f s\n", lastbrewTime / 1000);
                machinestate = kBrewDetectionTrailing;
                lastbrewTime = 0;
            }

            if (SteamON == 1) {
                machinestate = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machinestate = kBackflush;
            }

            if (emergencyStop) {
                machinestate = kEmergencyStop;
            }

            if (pidON == 0) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kBrewDetectionTrailing:
            brewdetection();

            if (timerBrewdetection == 0) {
                machinestate = kPidNormal;
            }

            if ((brewTime > 0 && ONLYPID == 1 && Brewdetection == 3) ||  // New Brew inner BD only by Only PID AND Voltage Sensor
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)) {
                machinestate = kBrew;
            }

            if (SteamON == 1) {
                machinestate = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machinestate = kBackflush;
            }

            if (emergencyStop) {
                machinestate = kEmergencyStop;
            }

            if (pidON == 0) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kSteam:
            if (SteamON == 0) {
                machinestate = kCoolDown;
            }

            if (emergencyStop) {
                machinestate = kEmergencyStop;
            }

            if (backflushON || backflushState > 10) {
                machinestate = kBackflush;
            }

            if (pidON == 0) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kCoolDown:
            if (Brewdetection == 2 || Brewdetection == 3) {
                /* For quickmill:  steam detection only via switch, calling
                 * brewdetection() detects new steam request
                 */
                brewdetection();
            }

            if (Brewdetection == 1 && ONLYPID == 1) {
                // if local max reached enable state 20, then heat to settemp.
                if (heatrateaverage > 0 && Input < BrewSetPoint + 2) {
                    machinestate = kPidNormal;
                }
            }

            if ((Brewdetection == 3 || Brewdetection == 2) &&
                Input < BrewSetPoint + 2) {
                machinestate = kPidNormal;
            }

            if (SteamON == 1) {
                machinestate = kSteam;
            }

            if (backflushON || backflushState > 10) {
                machinestate = kBackflush;
            }

            if (emergencyStop) {
                machinestate = kEmergencyStop;
            }

            if (pidON == 0) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kBackflush:
            if (backflushON == 0) {
                machinestate = kPidNormal;
            }

            if (emergencyStop) {
                machinestate = kEmergencyStop;
            }

            if (pidON == 0) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kEmergencyStop:
            if (!emergencyStop) {
                machinestate = kPidNormal;
            }

            if (pidON == 0) {
                machinestate = kPidOffline;
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kPidOffline:
            if (pidON == 1) {
                if (kaltstart) {
                    machinestate = kColdStart;
                } else if (!kaltstart && (Input > (BrewSetPoint - 10))) {  // Input higher BrewSetPoint-10, normal PID
                    machinestate = kPidNormal;
                } else if (Input <= (BrewSetPoint - 10)) {
                    machinestate =
                        kColdStart;  // Input 10C below set point, enter cold start
                    kaltstart = true;
                }
            }

            if (sensorError) {
                machinestate = kSensorError;
            }
            break;

        case kSensorError:
            machinestate = kSensorError;
            break;

        case keepromError:
            machinestate = keepromError;
            break;
    }

    if (machinestate != lastmachinestate) {
        Serial.printf("new machinestate: %i -> %i\n", lastmachinestate, machinestate);
        lastmachinestate = machinestate;
    }
}

void debugVerboseOutput() {
    static PeriodicTrigger trigger(10000);

    if (trigger.check()) {
        Serial.printf(
            "Tsoll=%5.1f  Tist=%5.1f Machinestate=%2i KP=%4.2f "
            "KI=%4.2f KD=%4.2f\n",
            BrewSetPoint, Input, machinestate, bPID.GetKp(), bPID.GetKi(),
            bPID.GetKd());
    }
}

/**
 * @brief TODO
 */
void tempLed() {
    if (TEMPLED == 1) {
        pinMode(LEDPIN, OUTPUT);
        digitalWrite(LEDPIN, LOW);

        // inner Tempregion
        if ((machinestate == kPidNormal && (fabs(Input - setPoint) < 0.5)) || (Input > 115 && fabs(Input - BrewSetPoint) < 5))  {
            digitalWrite(LEDPIN, HIGH);
        }
    }
}

/**
 * @brief Set up internal WiFi hardware
 */
void wiFiSetup() {
    unsigned long started = millis();

    #if defined(ESP8266)
        WiFi.hostname(hostname);
    #endif

    #if OLED_DISPLAY != 0
        displayLogo(langstring_connectwifi1, ssid);
    #endif

    /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     * would try to act as both a client and an access-point and could cause
     * network-issues with your other WiFi-devices on your WiFi-network.
     */
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);  // needed, otherwise exceptions are triggered \o.O/
    WiFi.begin(ssid, pass);

    #if defined(ESP32)
        WiFi.setHostname(hostname);
    #endif

    Serial.printf("Connecting to %s ...\n", ssid);

    // wait up to 20 seconds for connection:
    while ((WiFi.status() != WL_CONNECTED) && (millis() - started < 20000)) {
        yield();  // Prevent Watchdog trigger
    }

    checkWifi();  // try to reconnect

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("WiFi connected - IP = %i.%i.%i.%i\n", WiFi.localIP()[0],
                    WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);

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
    } else {  // No WiFi
        #if OLED_DISPLAY != 0
            displayLogo(langstring_nowifi[0], langstring_nowifi[1]);
        #endif

        Serial.println("No WIFI");
        WiFi.disconnect(true);
        delay(1000);
    }
}

/**
 * @brief Blynk Setup
 */
void BlynkSetup() {
    if (BLYNK == 1) {
        Serial.println("Wifi works, now try Blynk (timeout 30s)");
        Blynk.config(auth, blynkaddress, blynkport);
        Blynk.connect(30000);

        if (Blynk.connected() == true) {
            #if OLED_DISPLAY != 0
                displayLogo(langstring_connectblynk2[0], langstring_connectblynk2[1]);
            #endif

            Serial.println("Blynk is online");
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
        } else {
            Serial.println("No connection to Blynk");

            if (readSysParamsFromStorage() == 0) {
                #if OLED_DISPLAY != 0
                        displayLogo("3: Blynk not connected", "use eeprom values..");
                #endif
            }
        }
    }
}

/**
 * @brief Set up embedded Website
 */
void websiteSetup() {
    setEepromWriteFcn(writeSysParamsToStorage);
    //setBlynkWriteFcn(writeSysParamsToBlynk);
    //setMQTTWriteFcn(writeSysParamsToMQTT);

    if (readSysParamsFromStorage() != 0) {
        #if OLED_DISPLAY != 0
            displayLogo("3:", "use eeprom values..");
        #endif
    } else {
        #if OLED_DISPLAY != 0
            displayLogo("3:", "config defaults..");
        #endif
    }

  serverSetup();
}

void setup() {
    const String sysVersion = "Version " + String(getFwVersion()) + " " + FW_BRANCH;

    Serial.begin(115200);

    initTimer1();

    storageSetup();

    // Check AP Mode
    checklastpoweroff();

    if (softApEnabled == 1) {
        #if OLED_DISPLAY != 0
            u8g2.setI2CAddress(oled_i2c * 2);
            u8g2.begin();
            u8g2_prepare();
            displayLogo(sysVersion, "");
            delay(2000);
        #endif

        disableTimer1();
        createSoftAp();
    } else if (softApEnabled == 0) {
        // Define trigger type
        if (triggerType) {
            relayON = HIGH;
            relayOFF = LOW;
        } else {
            relayON = LOW;
            relayOFF = HIGH;
        }

        if (TRIGGERRELAYTYPE) {
            relayETriggerON = HIGH;
            relayETriggerOFF = LOW;
        } else {
            relayETriggerON = LOW;
            relayETriggerOFF = HIGH;
        }
        if (VOLTAGESENSORTYPE) {
            VoltageSensorON = HIGH;
            VoltageSensorOFF = LOW;
        } else {
            VoltageSensorON = LOW;
            VoltageSensorOFF = HIGH;
        }

        // Initialize Pins
        pinMode(PINVALVE, OUTPUT);
        pinMode(PINPUMP, OUTPUT);
        pinMode(PINHEATER, OUTPUT);
        pinMode(PINSTEAMSWITCH, INPUT);
        digitalWrite(PINVALVE, relayOFF);
        digitalWrite(PINPUMP, relayOFF);
        digitalWrite(PINHEATER, LOW);

        // IF Etrigger selected
        if (ETRIGGER == 1) {
            pinMode(PINETRIGGER, OUTPUT);
            digitalWrite(PINETRIGGER, relayETriggerOFF);  // Set the E-Trigger OFF its,
                                                          // important for LOW Trigger Relais
        }

        // IF Voltage sensor selected
        if (BREWDETECTION == 3) {
        pinMode(PINVOLTAGESENSOR, PINMODEVOLTAGESENSOR);
        }

        // IF PINBREWSWITCH & Steam selected
        if (PINBREWSWITCH > 0) {
            #if (defined(ESP8266) && PINBREWSWITCH == 16)
                pinMode(PINBREWSWITCH, INPUT_PULLDOWN_16);
            #endif

            #if (defined(ESP8266) && PINBREWSWITCH == 15)
                pinMode(PINBREWSWITCH, INPUT);
            #endif

            #if defined(ESP32)
                pinMode(PINBREWSWITCH, INPUT_PULLDOWN);
                ;
            #endif
        }

        #if (defined(ESP8266) && PINSTEAMSWITCH == 16)
            pinMode(PINSTEAMSWITCH, INPUT_PULLDOWN_16);
        #endif

        #if (defined(ESP8266) && PINSTEAMSWITCH == 15)
            pinMode(PINSTEAMSWITCH, INPUT);
        #endif

        #if defined(ESP32)
            pinMode(PINSTEAMSWITCH, INPUT_PULLDOWN);
        #endif

        #if OLED_DISPLAY != 0
            u8g2.setI2CAddress(oled_i2c * 2);
            u8g2.begin();
            u8g2_prepare();
            displayLogo(sysVersion, "");
            delay(2000);
        #endif

        // Init Scale by BREWMODE 2 or SHOTTIMER 2
        #if (BREWMODE == 2 || ONLYPIDSCALE == 1)
            initScale();
        #endif

        // VL530L0x TOF sensor
        if (TOF != 0) {
            lox.begin(tof_i2c);  // initialize TOF sensor at I2C address
            lox.setMeasurementTimingBudgetMicroSeconds(2000000);
        }

        // BLYNK & Fallback offline
        if (connectmode == 1) {  // WiFi Mode
            wiFiSetup();
            websiteSetup();

            BlynkSetup();

            // OTA Updates
            if (ota && WiFi.status() == WL_CONNECTED) {
                ArduinoOTA.setHostname(OTAhost);  //  Device name for OTA
                ArduinoOTA.setPassword(OTApass);  //  Password for OTA
                ArduinoOTA.begin();
            }

            if (MQTT == 1) {
                snprintf(topic_will, sizeof(topic_will), "%s%s/%s", mqtt_topic_prefix, hostname, "will");
                snprintf(topic_set, sizeof(topic_set), "%s%s/+/%s", mqtt_topic_prefix, hostname, "set");
                mqtt.setServer(mqtt_server_ip, mqtt_server_port);
                mqtt.setCallback(mqtt_callback);
                checkMQTT();
            }
        }

        // Initialize PID controller
        bPID.SetSampleTime(windowSize);
        bPID.SetOutputLimits(0, windowSize);
        bPID.SetMode(AUTOMATIC);

        // Temp sensor
        if (TempSensor == 1) {
            sensors.begin();
            sensors.getAddress(sensorDeviceAddress, 0);
            sensors.setResolution(sensorDeviceAddress, 10);
            sensors.requestTemperatures();
            Input = sensors.getTempCByIndex(0);
        }

        if (TempSensor == 2) {
            temperature = 0;

            #if (ONE_WIRE_BUS == 16 && defined(ESP8266))
                Sensor1.getTemperature(&temperature);
                Input = Sensor1.calc_Celsius(&temperature);
            #endif

            #if ((ONE_WIRE_BUS != 16 && defined(ESP8266)) || defined(ESP32))
                Input = Sensor2.getTemp();
            #endif
        }

        // moving average ini array
        if (Brewdetection == 1) {
            for (int thisReading = 0; thisReading < numReadings; thisReading++) {
                readingstemp[thisReading] = 0;
                readingstime[thisReading] = 0;
                readingchangerate[thisReading] = 0;
            }
        }

        // Initialisation MUST be at the very end of the init(), otherwise the
        // time comparision in loop() will have a big offset
        unsigned long currentTime = millis();
        previousMillistemp = currentTime;
        windowStartTime = currentTime;
        previousMillisDisplay = currentTime;
        previousMillisBlynk = currentTime;
        previousMillisMQTT = currentTime;
        previousMillisInflux = currentTime;
        previousMillisETrigger = currentTime;
        previousMillisVoltagesensorreading = currentTime;
        lastMQTTConnectionAttempt = currentTime;

        #if (BREWMODE == 2)
            previousMillisScale = currentTime;
        #endif
        #if (PRESSURESENSOR == 1)
            previousMillisPressure = currentTime;
        #endif

        setupDone = true;

        enableTimer1();

    }  // else softenable == 1
}

void loop() {
    if (calibration_mode == 1 && TOF == 1) {
        loopcalibrate();
    } else if (softApEnabled == 0) {
        looppid();
    } else if (softApEnabled == 1) {
        switch (softApstate) {
            case 0:
                if (WiFi.softAPgetStationNum() > 0) {
                    ArduinoOTA.setHostname(OTAhost);  //  Device name for OTA
                    ArduinoOTA.setPassword(OTApass);  //  Password for OTA
                    ArduinoOTA.begin();
                    softApstate = 10;
                }
                break;
            case 10:
                ArduinoOTA.handle();  // For OTA

                // Disable interrupt it OTA is starting, otherwise it will not work
                ArduinoOTA.onStart([]() {
                    #if defined(ESP8266)
                            timer1_disable();
                    #endif

                    #if defined(ESP32)
                            timerAlarmDisable(timer);
                    #endif

                    digitalWrite(PINHEATER, LOW);  // Stop heating
                });

                ArduinoOTA.onError([](ota_error_t error) {
                    #if defined(ESP8266)
                            timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
                    #endif

                    #if defined(ESP32)
                            timerAlarmEnable(timer);
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
                break;
        }
    }
}

/**
 * @brief ToF sensor calibration mode
 */
void loopcalibrate() {
    // Deactivate PID
    if (pidMode == 1) {
        pidMode = 0;
        bPID.SetMode(pidMode);
        Output = 0;
    }

    if (Blynk.connected() && BLYNK == 1) {  // If connected run as normal
        Blynk.run();
        blynkReCnctCount = 0;  // reset blynk reconnects if connected
    } else {
        checkBlynk();
    }

    digitalWrite(PINHEATER, LOW);   // Stop heating to be on the safe side ...

    unsigned long currentMillisTOF = millis();

    if (currentMillisTOF - previousMillisTOF >= intervalTOF) {
        previousMillisTOF = millis();
        VL53L0X_RangingMeasurementData_t measure;   // TOF Sensor measurement
        lox.rangingTest(&measure, false);           // pass in 'true' to get debug data printout!
        distance = measure.RangeMilliMeter;         // write new distence value to 'distance'
        Serial.println(distance);
        Serial.println("mm");

        #if OLED_DISPLAY != 0
            displayDistance(distance);
        #endif
    }
}

void looppid() {
    // Only do Wifi stuff, if Wifi is connected
    if (WiFi.status() == WL_CONNECTED && Offlinemodus == 0) {
        if (MQTT == 1) {
            checkMQTT();
            writeSysParamsToMQTT();

            if (mqtt.connected() == 1) {
                mqtt.loop();
            }
        }

        ArduinoOTA.handle();  // For OTA

        // Disable interrupt it OTA is starting, otherwise it will not work
        ArduinoOTA.onStart([]() {
            disableTimer1();
            digitalWrite(PINHEATER, LOW);  // Stop heating
        });

        ArduinoOTA.onError([](ota_error_t error) { enableTimer1(); });

        // Enable interrupts if OTA is finished
        ArduinoOTA.onEnd([]() { enableTimer1(); });

        if (Blynk.connected() && BLYNK == 1) {  // If connected run as normal
            Blynk.run();
            blynkReCnctCount = 0;  // reset blynk reconnects if connected
        } else {
            checkBlynk();
        }

        wifiReconnects = 0;  // reset wifi reconnects if connected
    } else {
        checkWifi();
    }

    if (TOF != 0) {
        unsigned long currentMillisTOF = millis();

        if (currentMillisTOF - previousMillisTOF >= intervalTOF) {
            previousMillisTOF = millis();
            VL53L0X_RangingMeasurementData_t measure;   // TOF Sensor measurement
            lox.rangingTest(&measure, false);           // pass in 'true' to get debug data printout!
            distance = measure.RangeMilliMeter;         // write new distence value to 'distance'

            if (distance <= 1000) {
                percentage = (100.00 / (water_empty - water_full)) * (water_empty - distance);  // calculate percentage of waterlevel
                Serial.println(percentage);
            }
        }
    }

    refreshTemp();        // update temperature values
    testEmergencyStop();  // test if temp is too high
    bPID.Compute();

    if ((millis() - lastTempEvent) > tempEventInterval) {
        sendTempEvent(Input, BrewSetPoint);
        lastTempEvent = millis();
    }

    #if (BREWMODE == 2 || ONLYPIDSCALE == 1)
        checkWeight();  // Check Weight Scale in the loop
    #endif

    #if (PRESSURESENSOR == 1)
        checkPressure();
    #endif

    brew();          // start brewing if button pressed
    checkSteamON();  // check for steam
    setEmergencyStopTemp();
    sendToBlynkMQTT();
    machinestatevoid();      // calc machinestate
    setchecklastpoweroff();  // FOR AP MODE
    tempLed();

    if (INFLUXDB == 1) {
        sendInflux();
    }

    if (ETRIGGER == 1) {  // E-Trigger active then void Etrigger()
        handleETrigger();
    }

    #if (ONLYPIDSCALE == 1)  // only by shottimer 2, scale
        shottimerscale();
    #endif

    // Check if PID should run or not. If not, set to manuel and force output to zero
    #if OLED_DISPLAY != 0
    unsigned long currentMillisDisplay = millis();
    if (currentMillisDisplay - previousMillisDisplay >= 100) {
        displayShottimer();
    }
    if (currentMillisDisplay - previousMillisDisplay >= intervalDisplay) {
        previousMillisDisplay = currentMillisDisplay;
    #if DISPLAYTEMPLATE < 20  // not in vertikal template
        Displaymachinestate();
    #endif
        printScreen();  // refresh display
    }
    #endif

    if (machinestate == kPidOffline || machinestate == kSensorError || machinestate == kEmergencyStop || machinestate == keepromError) {
        if (pidMode == 1) {
            // Force PID shutdown
            pidMode = 0;
            bPID.SetMode(pidMode);
            Output = 0;
            digitalWrite(PINHEATER, LOW);  // Stop heating
        }
    } else {  // no sensorerror, no pid off or no Emergency Stop
        if (pidMode == 0) {
            pidMode = 1;
            bPID.SetMode(pidMode);
        }
    }

    // Set PID if first start of machine detected, and no SteamON
    if (machinestate == kInit || machinestate == kColdStart || machinestate == kSetPointNegative) {
        if (startTn != 0) {
            startKi = startKp / startTn;
        } else {
            startKi = 0;
        }

        if (lastmachinestatepid != machinestate) {
            Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", startKp, startKi, 0.0);
            lastmachinestatepid = machinestate;
        }

        bPID.SetTunings(startKp, startKi, 0, P_ON_M);
        // normal PID
    }

    if (machinestate == kPidNormal) {
        // Prevent overwriting of brewdetection values
        // calc ki, kd
        if (aggTn != 0) {
            aggKi = aggKp / aggTn;
        } else {
            aggKi = 0;
        }

        aggKd = aggTv * aggKp;

        if (lastmachinestatepid != machinestate) {
            Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", aggKp, aggKi, aggKd);
            lastmachinestatepid = machinestate;
        }

        bPID.SetTunings(aggKp, aggKi, aggKd, PonE);
        kaltstart = false;
    }

    // BD PID
    if (machinestate >= 30 && machinestate <= 35) {
        // calc ki, kd
        if (aggbTn != 0) {
            aggbKi = aggbKp / aggbTn;
        } else {
            aggbKi = 0;
        }

        aggbKd = aggbTv * aggbKp;

        if (lastmachinestatepid != machinestate) {
            Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", aggbKp, aggbKi, aggbKd);
            lastmachinestatepid = machinestate;
        }

        bPID.SetTunings(aggbKp, aggbKi, aggbKd, PonE);
    }

    // Steam on
    if (machinestate == kSteam) {
        if (lastmachinestatepid != machinestate) {
            Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", 150.0, 0.0, 0.0);
            lastmachinestatepid = machinestate;
        }

        bPID.SetTunings(150, 0, 0, PonE);
    }

    // chill-mode after steam
    if (machinestate == kCoolDown) {
        switch (machine) {
            case QuickMill:
                aggbKp = 150;
                aggbKi = 0;
                aggbKd = 0;
                break;

            default:
                // calc ki, kd
                if (aggbTn != 0) {
                    aggbKi = aggbKp / aggbTn;
                } else {
                    aggbKi = 0;
                }

                aggbKd = aggbTv * aggbKp;
        }

        if (lastmachinestatepid != machinestate) {
            Serial.printf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", aggbKp, aggbKi, aggbKd);
            lastmachinestatepid = machinestate;
        }

        bPID.SetTunings(aggbKp, aggbKi, aggbKd, PonE);
    }
    // sensor error OR Emergency Stop
}

void setBackflush(int backflush) {
    backflushON = backflush;
    writeSysParamsToBlynk();
}


void setSteamMode(int steamMode) {
    SteamON = steamMode;

    if (SteamON == 1) {
        SteamFirstON = 1;
    }

    if (SteamON == 0) {
        SteamFirstON = 0;
    }
    writeSysParamsToBlynk();
}

void setPidStatus(int pidStatus) {
    pidON = pidStatus;
     writeSysParamsToBlynk();
}

/**
 * @brief Reads all system parameter values from non-volatile storage
 *
 * @return 0 = success, < 0 = failure
 */
int readSysParamsFromStorage(void) {
    if (sysParaPidKpStart.getStorage() != 0) return -1;
    if (sysParaPidTnStart.getStorage() != 0) return -1;
    if (sysParaPidKpReg.getStorage() != 0) return -1;
    if (sysParaPidTnReg.getStorage() != 0) return -1;
    if (sysParaPidTvReg.getStorage() != 0) return -1;
    if (sysParaPidKpBd.getStorage() != 0) return -1;
    if (sysParaPidTnBd.getStorage() != 0) return -1;
    if (sysParaPidTvBd.getStorage() != 0) return -1;
    if (sysParaBrewSetPoint.getStorage() != 0) return -1;
    if (sysParaBrewTime.getStorage() != 0) return -1;
    if (sysParaBrewSwTimer.getStorage() != 0) return -1;
    if (sysParaBrewThresh.getStorage() != 0) return -1;
    if (sysParaPreInfTime.getStorage() != 0) return -1;
    if (sysParaPreInfPause.getStorage() != 0) return -1;
    if (sysParaWeightSetPoint.getStorage() != 0) return -1;
    if (sysParaPidOn.getStorage() != 0) return -1;

    return 0;
}

/**
 * @brief Writes all current system parameter values to non-volatile storage
 *
 * @return 0 = success, < 0 = failure
 */
int writeSysParamsToStorage(void) {
    if (sysParaPidKpStart.setStorage() != 0) return -1;
    if (sysParaPidTnStart.setStorage() != 0) return -1;
    if (sysParaPidKpReg.setStorage() != 0) return -1;
    if (sysParaPidTnReg.setStorage() != 0) return -1;
    if (sysParaPidTvReg.setStorage() != 0) return -1;
    if (sysParaPidKpBd.setStorage() != 0) return -1;
    if (sysParaPidTnBd.setStorage() != 0) return -1;
    if (sysParaPidTvBd.setStorage() != 0) return -1;
    if (sysParaBrewSetPoint.setStorage() != 0) return -1;
    if (sysParaBrewTime.setStorage() != 0) return -1;
    if (sysParaBrewSwTimer.setStorage() != 0) return -1;
    if (sysParaBrewThresh.setStorage() != 0) return -1;
    if (sysParaPreInfTime.setStorage() != 0) return -1;
    if (sysParaPreInfPause.setStorage() != 0) return -1;
    if (sysParaWeightSetPoint.setStorage() != 0) return -1;
    if (sysParaPidOn.setStorage() != 0) return -1;

    return storageCommit();
}

/**
 * @brief Send all current system parameter values to Blynk
 *
 * @return TODO 0 = success, < 0 = failure
 */
void writeSysParamsToBlynk(void) {
    if (BLYNK == 1 && Blynk.connected()) {
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
        Blynk.virtualWrite(V40, backflushON);
        Blynk.virtualWrite(V15, SteamON);

        #if (BREWMODE == 2)
            Blynk.virtualWrite(V18, weightSetpoint);
        #endif

        #if (COLDSTART_PID == 2)  // 2=?Blynk values, else default starttemp from config
            Blynk.virtualWrite(V11, startKp);
            Blynk.virtualWrite(V14, startTn);
        #endif
    }
}

/**
 * @brief Send all current system parameter values to MQTT
 *
 * @return TODO 0 = success, < 0 = failure
 */
void writeSysParamsToMQTT(void) {
    unsigned long currentMillisMQTT = millis();
    if ((currentMillisMQTT - previousMillisMQTT >= intervalMQTT) && (MQTT == 1)) {
        previousMillisMQTT = currentMillisMQTT;

        if (mqtt.connected() == 1) {
            mqtt_publish("temperature", number2string(Input));
            mqtt_publish("setPoint", number2string(setPoint));
            mqtt_publish("BrewSetPoint", number2string(BrewSetPoint));
            mqtt_publish("SteamSetPoint", number2string(SteamSetPoint));
            mqtt_publish("HeaterPower", number2string(Output));
            mqtt_publish("currentKp", number2string(bPID.GetKp()));
            mqtt_publish("currentKi", number2string(bPID.GetKi()));
            mqtt_publish("currentKd", number2string(bPID.GetKd()));
            mqtt_publish("pidON", number2string(pidON));
            mqtt_publish("brewtime", number2string(brewtime));
            mqtt_publish("preinfusionpause", number2string(preinfusionpause));
            mqtt_publish("preinfusion", number2string(preinfusion));
            mqtt_publish("SteamON", number2string(SteamON));
            mqtt_publish("backflushON", number2string(backflushON));

            // Normal PID
            mqtt_publish("aggKp", number2string(aggKp));
            mqtt_publish("aggTn", number2string(aggTn));
            mqtt_publish("aggTv", number2string(aggTv));

            // BD PID
            mqtt_publish("aggbKp", number2string(aggbKp));
            mqtt_publish("aggbTn", number2string(aggbTn));
            mqtt_publish("aggbTv", number2string(aggbTv));

            // Start PI
            mqtt_publish("startKp", number2string(startKp));
            mqtt_publish("startTn", number2string(startTn));

            //BD Parameter
            mqtt_publish("BrewTimer", number2string(brewtimersoftware));
            mqtt_publish("BrewLimit", number2string(brewboarder));

            #if (BREWMODE == 2)
                mqtt_publish("weightSetpoint(g)", number2string(weightSetpoint));
            #endif
        }
    }
}

/**
 * @brief Returns the firmware version as string (x.y.z).
 *
 * @return firmware version string
 */
const char* getFwVersion(void)
{
    static const String sysVersion = String(FW_VERSION) + "." +
                                     String(FW_SUBVERSION) + "." +
                                     String(FW_HOTFIX);
    return sysVersion.c_str();
}



/**
 * @brief Performs a factory reset.
 *
 * @return 0 = success, < 0 = failure
 */
int factoryReset(void) {
    int stoStatus;

    if ((stoStatus = storageFactoryReset()) != 0)
        return stoStatus;

    return readSysParamsFromStorage();
}

