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
#define FW_BRANCH     "MASTER"

// Includes
#include <ArduinoOTA.h>
#include "Storage.h"
#include "SysPara.h"
#include "icon.h"        // user icons for display
#include "languages.h"   // for language translation
#include "userConfig.h"  // needs to be configured by the user
#include "debugSerial.h"

#include "rancilio-pid.h"

// Libraries
#include <Adafruit_VL53L0X.h>   // for ToF Sensor
#include <DallasTemperature.h>  // Library for dallas temp sensor
#include <WiFiManager.h>
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

int offlineMode = 0;
const int OnlyPID = ONLYPID;
const int TempSensor = TEMPSENSOR;
const int Brewdetection = BREWDETECTION;
const int triggerType = TRIGGERTYPE;
const int VoltageSensorType = VOLTAGESENSORTYPE;
const boolean ota = OTA;
const int grafana = GRAFANA;
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
WiFiManager wm;
const unsigned long wifiConnectionDelay = WIFICONNECTIONDELAY;
const unsigned int maxWifiReconnects = MAXWIFIRECONNECTS;
const char *hostname = HOSTNAME;
const char *auth = AUTH;
const char *pass = PASS;
unsigned long lastWifiConnectionAttempt = millis();
unsigned int wifiReconnects = 0;  // actual number of reconnects

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
InfluxDBClient influxClient(INFLUXDB_URL, INFLUXDB_DB_NAME);
Point influxSensor("machinestate");
const unsigned long intervalInflux = INFLUXDB_INTERVAL;
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
void setNormalPIDTunings();
void setBDPIDTunings();
void loopcalibrate();
void looppid();
void printMachineState();
char const* machinestateEnumToString(MachineState machinestate);
void initSteamQM();
boolean checkSteamOffQM();
void writeSysParamsToBlynk(void);
void writeSysParamsToMQTT(void);
char *number2string(double in);
char *number2string(float in);
char *number2string(int in);
char *number2string(unsigned int in);
float filter(float input);

// Variable declarations
uint8_t pidON = 1;               // 1 = control loop in closed loop
uint8_t usePonM = 0;             // 1 = use PonM for cold start PID, 0 = use normal PID for cold start
int relayON, relayOFF;           // used for relay trigger type. Do not change!
boolean coldstart = true;        // true = Rancilio started for first time
boolean emergencyStop = false;   // Emergency stop if temperature is too high 
double EmergencyStopTemp = 120;  // Temp EmergencyStopTemp
float inX = 0, inY = 0, inOld = 0, inSum = 0; // used for filter()
int bars = 0;                    // used for getSignalStrength()
boolean brewDetected = 0;
boolean setupDone = false;
int backflushON = 0;             // 1 = activate backflush
int flushCycles = 0;             // number of active flush cycles
int backflushState = 10;         // counter for state machine

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
int isBrewDetected = 0;      // flag is set if brew was detected
int firstreading = 1;        // Ini of the field, also used for sensor check

// PID - values for offline brewdetection
uint8_t useBDPID = 0;
double aggbKp = AGGBKP;
double aggbTn = AGGBTN;
double aggbTv = AGGBTV;

#if aggbTn == 0
    double aggbKi = 0;
#else
    double aggbKi = aggbKp / aggbTn;
#endif

double aggbKd = aggbTv * aggbKp;
double brewtimersoftware = BREW_SW_TIMER;  // use userConfig time until disabling BD PID
double brewsensitivity = BREWSENSITIVITY;  // user userConfig brew detection sensitivity 
const int PonE = PONE;

// Brewing, 1 = Normal Preinfusion , 2 = Scale & Shottimer = 2
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
double BrewTempOffset = TEMPOFFSET;
double setPoint = BrewSetPoint;
double SteamSetPoint = STEAMSETPOINT;
int SteamON = 0;
int SteamFirstON = 0;
double aggKp = AGGKP;
double aggTn = AGGTN;
double aggTv = AGGTV;
double aggIMax = AGGIMAX;
double startKp = STARTKP;
double startTn = STARTTN;
double steamKp = STEAMKP;

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
SysPara<double> sysParaPidKpStart(&startKp, 0, 200, STO_ITEM_PID_KP_START);
SysPara<double> sysParaPidTnStart(&startTn, 0, 999, STO_ITEM_PID_TN_START);
SysPara<double> sysParaPidKpReg(&aggKp, 0, 200, STO_ITEM_PID_KP_REGULAR);
SysPara<double> sysParaPidTnReg(&aggTn, 0, 999, STO_ITEM_PID_TN_REGULAR);
SysPara<double> sysParaPidTvReg(&aggTv, 0, 999, STO_ITEM_PID_TV_REGULAR);
SysPara<double> sysParaPidIMaxReg(&aggIMax, 0, 999, STO_ITEM_PID_I_MAX_REGULAR);
SysPara<double> sysParaPidKpBd(&aggbKp, 0, 200, STO_ITEM_PID_KP_BD);
SysPara<double> sysParaPidTnBd(&aggbTn, 0, 999, STO_ITEM_PID_TN_BD);
SysPara<double> sysParaPidTvBd(&aggbTv, 0, 999, STO_ITEM_PID_TV_BD);
SysPara<double> sysParaBrewSetPoint(&BrewSetPoint, 20, 105, STO_ITEM_BREW_SETPOINT);
SysPara<double> sysParaTempOffset(&BrewTempOffset, 0, 20, STO_ITEM_BREW_TEMP_OFFSET);
SysPara<double> sysParaBrewTime(&brewtime, 5, 60, STO_ITEM_BREW_TIME);
SysPara<double> sysParaBrewSwTimer(&brewtimersoftware, 5, 60, STO_ITEM_BREW_SW_TIMER);
SysPara<double> sysParaBrewThresh(&brewsensitivity, 0, 999, STO_ITEM_BD_THRESHOLD);
SysPara<double> sysParaPreInfTime(&preinfusion, 0, 10, STO_ITEM_PRE_INFUSION_TIME);
SysPara<double> sysParaPreInfPause(&preinfusionpause, 0, 20, STO_ITEM_PRE_INFUSION_PAUSE);
SysPara<double> sysParaWeightSetPoint(&weightSetpoint, 0, 500, STO_ITEM_WEIGHTSETPOINT);
SysPara<double> sysParaPidKpSteam(&steamKp, 0, 500, STO_ITEM_PID_KP_STEAM);
SysPara<uint8_t> sysParaPidOn(&pidON, 0, 1, STO_ITEM_PID_ON);
SysPara<uint8_t> sysParaUsePonM(&usePonM, 0, 1, STO_ITEM_PID_START_PONM);
SysPara<uint8_t> sysParaUseBDPID(&useBDPID, 0, 1, STO_ITEM_USE_BD_PID);


enum MQTTSettableType {
    tUInt8,
    tDouble
};

struct mqttVars_t {
    String mqttParamName;
    MQTTSettableType type;
    int minValue;
    int maxValue;
    void *mqttVarPtr;
};

std::vector<mqttVars_t> mqttVars = {
    {"BrewSetPoint", tDouble, 20, 105, (void *)&BrewSetPoint},    
    {"BrewTempOffset", tDouble, 0, 15, (void *)&BrewTempOffset},    
    {"brewtime", tDouble, 0, 60, (void *)&brewtime},
    {"preinfusion", tDouble, 0, 10, (void *)&preinfusion},
    {"preinfusionpause", tDouble, 0, 20, (void *)&preinfusionpause},
    {"pidON", tUInt8, 0, 1, (void *)&pidON},
    {"SteamON", tUInt8, 0, 1, (void *)&SteamON},
    {"backflushON", tUInt8, 0, 1, (void *)&backflushON},
    {"aggKp", tDouble, 0, 100, (void *)&aggKp},
    {"aggTn", tDouble, 0, 999, (void *)&aggTn},
    {"aggTv", tDouble, 0, 999, (void *)&aggTv},
    {"aggIMax", tDouble, 0, 999, (void *)&aggIMax},
    {"aggbKp", tDouble, 0, 100, (void *)&aggbKp},
    {"aggbTn", tDouble, 0, 999, (void *)&aggbTn},
    {"aggbTv", tDouble, 0, 999, (void *)&aggbTv},
    {"steamKp", tDouble, 0, 500, (void *)&steamKp},
};

// Embedded HTTP Server
#include "RancilioServer.h"


enum SectionNames {
    sPIDSection,
    sTempSection,
    sBDSection,
    sOtherSection
};

std::vector<editable_t> editableVars = {};

unsigned long lastTempEvent = 0;
unsigned long tempEventInterval = 1000;

/**
 * @brief Get Wifi signal strength and set bars for display
 */
void getSignalStrength() {
    if (offlineMode == 1) return;

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
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);  // e.g. 1.3"
#endif
#if OLED_DISPLAY == 2
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);  // e.g. 0.96"
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


// Blynk define pins and read values
BLYNK_CONNECTED() {
    if (offlineMode == 0 && BLYNK == 1) {
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

BLYNK_WRITE(V34) { brewsensitivity = param.asDouble(); }

BLYNK_WRITE(V40) { backflushON = param.asInt(); }

BLYNK_WRITE(V11) { startKp = param.asDouble(); }

BLYNK_WRITE(V14) { startTn = param.asDouble(); }

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

            debugPrintf("pressure raw / filtered: %f / %f\n", inputPressure, inputPressureFilter);
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
            debugPrintf(
                "*** WARNING: temperature sensor reading: consec_errors = %i, "
                "temp_current = %.1f\n",
                "temp_prev = %.1f\n",
                error, tempInput, previousInput);
        }
    } else if (badCondition == false && sensorOK == false) {
        error = 0;
        sensorOK = true;
    }

    if (error >= maxErrorCounter && !sensorError) {
        sensorError = true;
        debugPrintf(
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

            Input = sensors.getTempCByIndex(0) - BrewTempOffset;

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
        // Temperature_C = 94;
        if (!checkSensor(Temperature_C - BrewTempOffset) && firstreading == 0)
            return; // if sensor data is not valid, abort function; Sensor must
                    // be read at least one time at system startup

            Input = Temperature_C - BrewTempOffset;

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

    debugPrintln("Start offline mode with eeprom values, no wifi :(");
    offlineMode = 1;

    if (readSysParamsFromStorage() != 0) {
        #if OLED_DISPLAY != 0
            displayMessage("", "", "", "", "No eeprom,", "Values");
        #endif

        debugPrintln(
            "No working eeprom value, I am sorry, but use default offline value "
            ":)");

        delay(1000);
    }
}

/**
 * @brief Check if Wifi is connected, if not reconnect abort function if offline, or brew is running
 */
void checkWifi() {
    if (offlineMode == 1 || brewcounter > 11) return;

    /* if coldstart ist still true when checkWifi() is called, then there was no WIFI connection
     * at boot -> connect or offlinemode
     */
    do {
        if ((millis() - lastWifiConnectionAttempt >= wifiConnectionDelay) && (wifiReconnects <= maxWifiReconnects)) {
            int statusTemp = WiFi.status();

            if (statusTemp != WL_CONNECTED) {  // check WiFi connection status
                lastWifiConnectionAttempt = millis();
                wifiReconnects++;
                debugPrintf("Attempting WIFI reconnection: %i\n", wifiReconnects);

                if (!setupDone) {
                    #if OLED_DISPLAY != 0
                        displayMessage("", "", "", "", langstring_wifirecon, String(wifiReconnects));
                    #endif
                }

                wm.disconnect();
                wm.autoConnect();

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
        influxSensor.clearFields();
        influxSensor.addField("value", Input);
        influxSensor.addField("setPoint", setPoint);
        influxSensor.addField("HeaterPower", Output);
        influxSensor.addField("Kp", bPID.GetKp());
        influxSensor.addField("Ki", bPID.GetKi());
        influxSensor.addField("Kd", bPID.GetKd());
        influxSensor.addField("pidON", pidON);
        influxSensor.addField("brewtime", brewtime);
        influxSensor.addField("preinfusionpause", preinfusionpause);
        influxSensor.addField("preinfusion", preinfusion);
        influxSensor.addField("SteamON", SteamON);

        byte mac[6];
        WiFi.macAddress(mac);
        String macaddr0 = number2string(mac[0]);
        String macaddr1 = number2string(mac[1]);
        String macaddr2 = number2string(mac[2]);
        String macaddr3 = number2string(mac[3]);
        String macaddr4 = number2string(mac[4]);
        String macaddr5 = number2string(mac[5]);
        String completemac = macaddr0 + macaddr1 + macaddr2 + macaddr3 + macaddr4 + macaddr5;
        influxSensor.addField("mac", completemac);

        // Write point
        if (!influxClient.writePoint(influxSensor)) {
            debugPrintf("InfluxDB write failed: %s\n", influxClient.getLastErrorMessage().c_str());
        }
    }
}

/**
 * @brief Check if Blynk is connected, if not reconnect abort function if offline, or brew is running
 *      blynk is also using maxWifiReconnects!
 */
void checkBlynk() {
    if (offlineMode == 1 || BLYNK == 0 || brewcounter > 11) return;

    if ((millis() - lastBlynkConnectionAttempt >= wifiConnectionDelay) && (blynkReCnctCount <= maxWifiReconnects)) {
        int statusTemp = Blynk.connected();

        if (statusTemp != 1) {
            lastBlynkConnectionAttempt = millis();  // Reconnection Timer Function
            blynkReCnctCount++;                     // Increment reconnection Counter
            debugPrintf("Attempting blynk reconnection: %i\n", blynkReCnctCount);
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
    if (offlineMode == 1 || brewcounter > 11) return;

    if ((millis() - lastMQTTConnectionAttempt >= wifiConnectionDelay) && (MQTTReCnctCount <= maxWifiReconnects)) {
        int statusTemp = mqtt.connected();

        if (statusTemp != 1) {
            lastMQTTConnectionAttempt = millis();  // Reconnection Timer Function
            MQTTReCnctCount++;                     // Increment reconnection Counter
            debugPrintf("Attempting MQTT reconnection: %i\n", MQTTReCnctCount);

            if (mqtt.connect(hostname, mqtt_username, mqtt_password, topic_will, 0, 0,"exit") == true) {
                mqtt.subscribe(topic_set);
                debugPrintln("Subscribe to MQTT Topics");
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
    if (offlineMode == 1) return;

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
    if (brewsensitivity == 0) return;  // abort brewdetection if deactivated

    // Brew detection: 1 = software solution, 2 = hardware, 3 = voltage sensor
    if (Brewdetection == 1) {
        if (isBrewDetected == 1) {
            timeBrewed = millis() - timeBrewdetection;
        }

        // deactivate brewtimer after end of brewdetection pid
        if (millis() - timeBrewdetection > brewtimersoftware * 1000 && isBrewDetected == 1) {
            isBrewDetected = 0;  // rearm brewdetection
            timeBrewed = 0;
        }
    } else if (Brewdetection == 2) {
        if (millis() - timeBrewdetection > brewtimersoftware * 1000 && isBrewDetected == 1) {
            isBrewDetected = 0;  // rearm brewdetection
        }
    } else if (Brewdetection == 3) {
        // timeBrewed counter
        if ((digitalRead(PINVOLTAGESENSOR) == VoltageSensorON) && brewDetected == 1) {
            timeBrewed = millis() - startingTime;
            lastbrewTime = timeBrewed;
        }

        // OFF: reset brew
        if ((digitalRead(PINVOLTAGESENSOR) == VoltageSensorOFF) && (brewDetected == 1 || coolingFlushDetectedQM == true)) {
            brewDetected = 0;
            timePVStoON = timeBrewed;  // for QuickMill
            timeBrewed = 0;
            startingTime = 0;
            coolingFlushDetectedQM = false;
            debugPrintln("HW Brew - Voltage Sensor - End");
        }
    }

    // Activate brew detection
    if (Brewdetection == 1) {  // SW BD
        // BD PID only +/- 4 Grad Celsius, no detection if HW was active
        if (heatrateaverage <= -brewsensitivity && isBrewDetected == 0 && (fabs(Input - BrewSetPoint) < 5)) {
            debugPrintln("SW Brew detected");
            timeBrewdetection = millis();
            isBrewDetected = 1;
        }
    } else if (Brewdetection == 2) {  // HW BD
        if (brewcounter > 10 && brewDetected == 0 && brewsensitivity != 0) {
            debugPrintln("HW Brew detected");
            timeBrewdetection = millis();
            isBrewDetected = 1;
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
                        isBrewDetected = 1;
                        brewDetected = 0;
                        lastbrewTime = 0;
                        brewSteamDetectedQM = 1;
                        debugPrintln("Quick Mill: setting brewSteamDetectedQM = 1");
                        logbrew.reset();
                    }

                    const unsigned long minBrewDurationForSteamModeQM_ON = 50;
                    if (brewSteamDetectedQM == 1 && millis()-timePVStoON > minBrewDurationForSteamModeQM_ON)
                    {

                        if (pvs == VoltageSensorOFF) {
                            brewSteamDetectedQM = 0;

                            if (millis() - timePVStoON < maxBrewDurationForSteamModeQM_ON) {
                                debugPrintln("Quick Mill: steam-mode detected");
                                initSteamQM();
                            } else {
                                debugPrintf("*** ERROR: QuickMill: neither brew nor steam\n");
                            }
                        } else if (millis() - timePVStoON > maxBrewDurationForSteamModeQM_ON) {
                            if (Input < BrewSetPoint + 2) {
                                debugPrintln("Quick Mill: brew-mode detected");
                                startingTime = timePVStoON;
                                brewDetected = 1;
                                brewSteamDetectedQM = 0;
                            } else {
                                debugPrintln("Quick Mill: cooling-flush detected");
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
                    debugPrintln("HW Brew - Voltage Sensor - Start");
                    timeBrewdetection = millis();
                    startingTime = millis();
                    isBrewDetected = 1;
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
float filter(float input) {
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
    boolean paramValid = false;
    boolean paramInRange = false;

    for (mqttVars_t m : mqttVars) {
        if (m.mqttParamName.equals(key)) {
            if (value >= m.minValue && value <= m.maxValue) {
                switch (m.type) {
                    case tDouble:
                        *(double *)m.mqttVarPtr = value;
                        paramValid = true;
                        break;
                    case tUInt8:
                        *(uint8_t *)m.mqttVarPtr = value;
                        paramValid = true;
                        break;
                    default:
                        debugPrintln((String(m.type) + " is not a recognized type for this MQTT parameter.").c_str());
                }

                paramInRange = true;
            }
            else {
                debugPrintln(("Value out of range for MQTT parameter "+ key + ".").c_str());
                paramInRange = false;
            }

            break;
        }
    }

    if (paramValid && paramInRange) {
        if (key.equals("SteamON")) {
            SteamFirstON = value;
        }

        mqtt_publish(param, number2string(value));
        writeSysParamsToBlynk();
        writeSysParamsToStorage();
    }
    else {
        debugPrintln((key + " is not a valid MQTT parameter.").c_str());
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
    debugPrintln(topic_pattern);

    if ((sscanf(topic_str, topic_pattern, &configVar, &cmd) != 2) || (strcmp(cmd, "set") != 0)) {
        debugPrintln(topic_str);
        return;
    }

    debugPrintln(topic_str);
    debugPrintln(data_str);

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
    } else if (SteamON == 0) {
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
                debugPrintf("%d\n", Input);
                debugPrintf("%d\n", machinestate);

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
                        debugPrintln(
                            "Input >= (BrewSetPoint-1), wait 10 sec before machinestate SetPointNegative");
                    }
                    break;

                case 10:
                    if (Input < (BrewSetPoint - 1)) {
                        machinestatecold = 0;  //  Input was only one time above
                                               //  BrewSetPoint, reset machinestatecold
                        debugPrintln("Reset timer for machinestate SetPointNegative: Input < (BrewSetPoint-1)");

                        break;
                    }

                    if (machinestatecoldmillis + 10 * 1000 < millis())  // 10 sec Input above BrewSetPoint, no set new state
                    {
                        machinestate = kSetPointNegative;
                        debugPrintln("5 sec Input >= (BrewSetPoint-1) finished, switch to state SetPointNegative");
                    }
                    break;
            }

            if (SteamON == 1) {
                machinestate = kSteam;
            }

            if ((timeBrewed > 0 && ONLYPID == 1) ||  // timeBrewed with Only PID
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42))
            {
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

            if ((timeBrewed > 0 && ONLYPID == 1) ||  // timeBrewed with Only PID
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42))
            {
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
            brewdetection();     // if brew detected, set PID values

            if ((timeBrewed > 0 && ONLYPID == 1) ||  // timeBrewed with Only PID
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42))
            {
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

            // Output brew time, temp and heatrateaverage during brew
            if (BREWDETECTION == 1 && logbrew.check()) {
                debugPrintf("(tB,T,hra) --> %5.2f %6.2f %8.2f\n",
                            (double)(millis() - startingTime) / 1000, Input, heatrateaverage);
            }

            if ((timeBrewed == 0 && Brewdetection == 3 && ONLYPID == 1) ||  // OnlyPID+: Voltage sensor BD timeBrewed == 0 -> switch is off again
                ((brewcounter == 10 || brewcounter == 43) && ONLYPID == 0)) // Hardware BD
            {
                // delay shot timer display for voltage sensor or hw brew toggle switch (brew counter)
                machinestate = kShotTimerAfterBrew;
                lastbrewTimeMillis = millis();  // for delay
            } else if (Brewdetection == 1 && ONLYPID == 1 && isBrewDetected == 0) {   // SW BD, kBrew was active for set time
                // when Software brew is finished, direct to PID BD
                machinestate = kBrewDetectionTrailing;
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
                debugPrintf("Shot time: %4.1f s\n", lastbrewTime / 1000);
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

            if (isBrewDetected == 0) {
                machinestate = kPidNormal;
            }

            if ((timeBrewed > 0 && ONLYPID == 1 && Brewdetection == 3) ||  // New Brew inner BD only by Only PID AND Voltage Sensor
                (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42))
            {
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

            if ((Brewdetection == 3 || Brewdetection == 2) && Input < BrewSetPoint + 2) {
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
                if (coldstart) {
                    machinestate = kColdStart;
                } else if (!coldstart && (Input > (BrewSetPoint - 10))) {  // Input higher BrewSetPoint-10, normal PID
                    machinestate = kPidNormal;
                } else if (Input <= (BrewSetPoint - 10)) {
                    machinestate = kColdStart;  // Input 10C below set point, enter cold start
                    coldstart = true;
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
        printMachineState();
        lastmachinestate = machinestate;
    }
}

void printMachineState() {
    debugPrintf("new machinestate: %s -> %s\n",
                machinestateEnumToString(lastmachinestate), machinestateEnumToString(machinestate));
}

char const* machinestateEnumToString(MachineState machinestate) {
    switch (machinestate) {
        case kInit:
            return "Init";
        case kColdStart:
            return "Cold Start";
        case kSetPointNegative:
            return "Set Point Negative";
        case kPidNormal:
            return "PID Normal";
        case kBrew:
            return "Brew";
        case kShotTimerAfterBrew:
            return "Shot Timer After Brew";
        case kBrewDetectionTrailing:
            return "Brew Detection Trailing";
        case kSteam:
            return "Steam";
        case kCoolDown:
            return "Cool Down";
        case kBackflush:
            return "Backflush";
        case kEmergencyStop:
            return "Emergency Stop";
        case kPidOffline:
            return "PID Offline";
        case kSensorError:
            return "Sensore Error";
        case keepromError:
            return "EEPROM Error";
    }

    return "Unknown";
} 

void debugVerboseOutput() {
    static PeriodicTrigger trigger(10000);

    if (trigger.check()) {
        debugPrintf(
            "Tsoll=%5.1f  Tist=%5.1f Machinestate=%2i KP=%4.2f "
            "KI=%4.2f KD=%4.2f\n",
            setPoint, Input, machinestate, bPID.GetKp(), bPID.GetKi(), bPID.GetKd());
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
    wm.setCleanConnect(true);
    wm.setConfigPortalTimeout(60); // sec Timeout for Portal
    wm.setConnectTimeout(10); // Try 10 Sec to Connect to WLAN
    wm.setBreakAfterConfig(true);

    if (wm.autoConnect(hostname, pass)) {
        debugPrintf("WiFi connected - IP = %i.%i.%i.%i\n", WiFi.localIP()[0],
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
        debugPrintf("MAC-ADDRESS: %s\n", completemac.c_str());

    } else {
        debugPrintln("WiFi connection timed out...");

        #if OLED_DISPLAY != 0
            displayLogo(langstring_nowifi[0], langstring_nowifi[1]);
        #endif

        wm.disconnect();
        delay(1000);

        offlineMode = 1;
    }

    #if OLED_DISPLAY != 0
        displayLogo(langstring_connectwifi1, wm.getWiFiSSID(true));
    #endif
        
    startRemoteSerialServer();
}

/**
 * @brief Blynk Setup
 */
void BlynkSetup() {
    if (BLYNK == 1) {
        debugPrintln("Wifi works, now try Blynk (timeout 30s)");
        Blynk.config(auth, blynkaddress, blynkport);
        Blynk.connect(30000);

        if (Blynk.connected() == true) {
            #if OLED_DISPLAY != 0
                displayLogo(langstring_connectblynk2[0], langstring_connectblynk2[1]);
            #endif

            debugPrintln("Blynk is online");
            debugPrintln("sync all variables and write new values to eeprom");

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
            debugPrintln("No connection to Blynk");

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
    const String sysVersion = "Version " + getFwVersion() + " " + FW_BRANCH;

    Serial.begin(115200);

    initTimer1();

    storageSetup();
    
    editableVars =  {
        {"PID_ON", "Enable PID Controller", false, "", kUInt8, 0, []{ return true; }, (void *)&pidON},
        {"START_USE_PONM", "Enable PonM", true, F("Use PonM mode (<a href='http://brettbeauregard.com/blog/2017/06/introducing-proportional-on-measurement/' target='_blank'>details</a>) while heating up the machine. Otherwise, just use the same PID values that are used later"), kUInt8, 0, []{ return true; }, (void *)&usePonM},
        {"START_KP", "Start Kp", true, F("Proportional gain for cold start controller. This value is not used with the the error as usual but the absolute value of the temperature and counteracts the integral part as the temperature rises. Ideally, both parameters are set so that they balance each other out when the target temperature is reached."), kDouble, sPIDSection, []{ return true && usePonM; }, (void *)&startKp},
        {"START_TN", "Start Tn", true, F("Integral gain for cold start controller (PonM mode, <a href='http://brettbeauregard.com/blog/2017/06/introducing-proportional-on-measurement/' target='_blank'>details</a>)"), kDouble, sPIDSection, []{ return true && usePonM; }, (void *)&startTn},
        {"PID_KP", "PID Kp", true, F("Proportional gain (in Watts/C°) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). The higher this value is, the higher is the output of the heater for a given temperature difference. E.g. 5°C difference will result in P*5 Watts of heater output."), kDouble, sPIDSection, []{ return true; }, (void *)&aggKp},
        {"PID_TN", "PID Tn (=Kp/Ki)", true, F("Integral time constant (in seconds) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). The larger this value is, the slower the integral part of the PID will increase (or decrease) if the process value remains above (or below) the setpoint in spite of proportional action. The smaller this value, the faster the integral term changes."), kDouble, sPIDSection, []{ return true; }, (void *)&aggTn},
        {"PID_TV", "PID Tv (=Kd/Kp)", true, F("Differential time constant (in seconds) for the main PID controller (in P-Tn-Tv form, <a href='http://testcon.info/EN_BspPID-Regler.html#strukturen' target='_blank'>Details<a>). This value determines how far the PID equation projects the current trend into the future. The higher the value, the greater the dampening. Select it carefully, it can cause oscillations if it is set too high or too low."), kDouble, sPIDSection, []{ return true; }, (void *)&aggTv},
        {"PID_I_MAX", "PID Integrator Max", true, F("Internal integrator limit to prevent windup (in Watts). This will allow the integrator to only grow to the specified value. This should be approximally equal to the output needed to hold the temperature after the setpoint has been reached and is depending on machine type and whether the boiler is insulated or not."), kDouble, sPIDSection, []{ return true; }, (void *)&aggIMax},
        {"STEAM_KP", "Steam Kp", true, F("Proportional gain for the steaming mode (I or D are not used)"), kDouble, sPIDSection, []{ return true; }, (void *)&steamKp},
        {"TEMP", "Temperature", false, "", kDouble, sPIDSection, []{ return false; }, (void *)&Input},
        {"BREW_SET_POINT", "Set point (°C)", true, F("The temperature that the PID will attempt to reach and hold"), kDouble, sTempSection, []{ return true; }, (void *)&BrewSetPoint},
        {"BREW_TEMP_OFFSET", "Offset (°C)", true, F("Optional offset that is added to the user-visible setpoint. Can be used to compensate sensor offsets and the average temperature loss between boiler and group so that the setpoint represents the approximate brew temperature."), kDouble, sTempSection, []{ return true; }, (void *)&BrewTempOffset},
        {"BREW_TIME", "Brew Time (s)", true, F("Stop brew after this time"), kDouble, sTempSection, []{ return true && ONLYPID == 0; }, (void *)&brewtime},
        {"BREW_PREINFUSIONPAUSE", "Preinfusion Pause Time (s)", false, "", kDouble, sTempSection, []{ return true && ONLYPID == 0; }, (void *)&preinfusionpause},
        {"BREW_PREINFUSION", "Preinfusion Time (s)", false, "", kDouble, sTempSection, []{ return true && ONLYPID == 0; }, (void *)&preinfusion},
        {"SCALE_WEIGHTSETPOINT", "Brew weight setpoint (g)", true, F("Brew until this weight has been measured."), kDouble, sTempSection, []{ return true && (ONLYPIDSCALE == 1 || BREWMODE == 2); }, (void *)&weightSetpoint},
        {"PID_BD_ON", "Enable Brew PID", true, F("Use separate PID parameters while brew is running"), kUInt8, sBDSection, []{ return true && BREWDETECTION > 0; }, (void *)&useBDPID},
        {"PID_BD_KP", "BD Kp", true, F("Proportional gain (in Watts/°C) for the PID when brewing has been detected. Use this controller to either increase heating during the brew to counter temperature drop from fresh cold water in the boiler. Some machines, e.g. Rancilio Silvia, actually need to heat less not at all during the brew because of high temperature stability (<a href='https://www.kaffee-netz.de/threads/installation-eines-temperatursensors-in-silvia-bruehgruppe.111093/#post-1453641' target='_blank'>Details<a>)"), kDouble, sBDSection, []{ return true && BREWDETECTION > 0 && useBDPID; }, (void *)&aggbKp},
        {"PID_BD_TN", "BD Tn (=Kp/Ki)", true, F("Integral time constant (in seconds) for the PID when brewing has been detected."), kDouble, sBDSection, []{ return true && BREWDETECTION > 0 && useBDPID; }, (void *)&aggbTn},
        {"PID_BD_TV", "BD Tv (=Kd/Kp)", true, F("Differential time constant (in seconds) for the PID when brewing has been detected."), kDouble, sBDSection, []{ return true && BREWDETECTION > 0 && useBDPID; }, (void *)&aggbTv},
        {"PID_BD_TIMER", "PID BD Time (s)", true, F("Fixed time in seconds for which the BD PID will stay enabled (also after Brew switch is inactive again)."), kDouble, sBDSection, []{ return true && BREWDETECTION > 0 && useBDPID; }, (void *)&brewtimersoftware},
        {"PID_BD_BREWSENSITIVITY", "PID BD Sensitivity", true, F("Software brew detection sensitivity that looks at average temperature, <a href='https://manual.rancilio-pid.de/de/customization/brueherkennung.html' target='_blank'>Details</a>. Needs to be &gt;0 also for Hardware switch detection."), kDouble, sBDSection, []{ return true && BREWDETECTION == 1; }, (void *)&brewsensitivity},
        {"STEAM_MODE", "Steam Mode", false, "", kUInt8, sOtherSection, []{ return false; }, (void *)&SteamON},
        {"BACKFLUSH_ON", "Backflush", false, "", kUInt8, sOtherSection, []{ return false; }, (void *)&backflushON},
    };

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

        if (INFLUXDB == 1) {
            if (INFLUXDB_AUTH_TYPE == 1) {
                influxClient.setConnectionParams(INFLUXDB_URL, INFLUXDB_ORG_NAME, INFLUXDB_DB_NAME, INFLUXDB_API_TOKEN);
            }
            else if (INFLUXDB_AUTH_TYPE == 2 && (strlen(INFLUXDB_USER) > 0) && (strlen(INFLUXDB_PASSWORD) > 0)) {
                influxClient.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
            }
        }
    }

    // Initialize PID controller
    bPID.SetSampleTime(windowSize);
    bPID.SetOutputLimits(0, windowSize);
    bPID.SetIntegratorLimits(0, AGGIMAX);
    bPID.SetSmoothingFactor(EMA_FACTOR);
    bPID.SetMode(AUTOMATIC);

    // Temp sensor
    if (TempSensor == 1) {
        sensors.begin();
        sensors.getAddress(sensorDeviceAddress, 0);
        sensors.setResolution(sensorDeviceAddress, 10);
        sensors.requestTemperatures();
        Input = sensors.getTempCByIndex(0) - BrewTempOffset;
    }

    if (TempSensor == 2) {
        temperature = 0;

        #if (ONE_WIRE_BUS == 16 && defined(ESP8266))
            Sensor1.getTemperature(&temperature);
            Input = Sensor1.calc_Celsius(&temperature) - BrewTempOffset;
        #endif

        #if ((ONE_WIRE_BUS != 16 && defined(ESP8266)) || defined(ESP32))
            Input = Sensor2.getTemp() - BrewTempOffset;
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
}

void loop() {
    if (calibration_mode == 1 && TOF == 1) {
        loopcalibrate();
    } else {
        looppid();
    }

    checkForRemoteSerialClients();
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
        debugPrintf("%d mm\n", distance);

        #if OLED_DISPLAY != 0
            displayDistance(distance);
        #endif
    }
}

void looppid() {
    // Only do Wifi stuff, if Wifi is connected
    if (WiFi.status() == WL_CONNECTED && offlineMode == 0) {
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
                debugPrintf("%d\n", percentage);
            }
        }
    }

    refreshTemp();        // update temperature values
    testEmergencyStop();  // test if temp is too high
    bPID.Compute();       //the variable Output now has new values from PID (will be written to heater pin in ISR.h)

    if ((millis() - lastTempEvent) > tempEventInterval) {
        //send temperatures to website endpoint
        sendTempEvent(Input, BrewSetPoint, Output);
        lastTempEvent = millis();

        #if VERBOSE
        if (pidON) {
            debugPrintf("Current PID mode: %s\n", bPID.GetPonE() ? "PonE" : "PonM");

            //P-Part
            debugPrintf("Current PID input error: %f\n", bPID.GetInputError());
            debugPrintf("Current PID P part: %f\n", bPID.GetLastPPart());
            debugPrintf("Current PID kP: %f\n", bPID.GetKp());
            //I-Part
            debugPrintf("Current PID I sum: %f\n", bPID.GetLastIPart());
            debugPrintf("Current PID kI: %f\n", bPID.GetKi());
            //D-Part
            debugPrintf("Current PID diff'd input: %f\n", bPID.GetDeltaInput());
            debugPrintf("Current PID D part: %f\n", bPID.GetLastDPart());
            debugPrintf("Current PID kD: %f\n", bPID.GetKd());

            //Combined PID output
            debugPrintf("Current PID Output: %f\n\n", Output);
            debugPrintf("Current Machinestate: %s\n\n", machinestateEnumToString(machinestate));
            debugPrintf("timeBrewed %f\n", timeBrewed);
            debugPrintf("brewtimersoftware %f\n", brewtimersoftware);
            debugPrintf("isBrewDetected %i\n", isBrewDetected);
            debugPrintf("Brewdetection %i\n", Brewdetection);

    
        }  
        #endif
    }

    #if (BREWMODE == 2 || ONLYPIDSCALE == 1)
        checkWeight();  // Check Weight Scale in the loop
    #endif

    #if (PRESSURESENSOR == 1)
        checkPressure();
    #endif

    brew();                  // start brewing if button pressed
    checkSteamON();          // check for steam
    setEmergencyStopTemp();
    sendToBlynkMQTT();
    machinestatevoid();      // update machinestate
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

    // Check if PID should run or not. If not, set to manual and force output to zero
#if OLED_DISPLAY != 0
    unsigned long currentMillisDisplay = millis();
    if (currentMillisDisplay - previousMillisDisplay >= 100) {
        displayShottimer();
    }
    if (currentMillisDisplay - previousMillisDisplay >= intervalDisplay) {
        previousMillisDisplay = currentMillisDisplay;
    #if DISPLAYTEMPLATE < 20  // not using vertical template
        Displaymachinestate();
    #endif
        printScreen();  // refresh display
    }
#endif

    if (machinestate == kPidOffline || machinestate == kSensorError || machinestate == kEmergencyStop || machinestate == keepromError || brewPIDdisabled) {
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
    if ((machinestate == kInit || machinestate == kColdStart || machinestate == kSetPointNegative)) {
        if (usePonM) { 
            if (startTn != 0) {
                startKi = startKp / startTn;
            } else {
                startKi = 0;
            }

            if (lastmachinestatepid != machinestate) {
                debugPrintf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", startKp, startKi, 0.0);
                lastmachinestatepid = machinestate;
            }

            bPID.SetTunings(startKp, startKi, 0, P_ON_M);
        } else {
            setNormalPIDTunings();
        }
    }

    if (machinestate == kPidNormal) {
        setNormalPIDTunings();
        coldstart = false;
    }

    // BD PID
    if (machinestate >= kBrew && machinestate <= kBrewDetectionTrailing) {
        if (BREWPID_DELAY > 0 && timeBrewed > 0 && timeBrewed < BREWPID_DELAY*1000) {
            //disable PID for BREWPID_DELAY seconds, enable PID again with new tunings after that
            if (!brewPIDdisabled) {
                brewPIDdisabled = true;
                bPID.SetMode(MANUAL);
                debugPrintf("disabled PID, waiting for %d seconds before enabling PID again\n", BREWPID_DELAY);
            }
        } else {
            if (brewPIDdisabled) {
                //enable PID again
                bPID.SetMode(AUTOMATIC);
                brewPIDdisabled = false;
                debugPrintln("Enabled PID again after delay");
            }

            if (useBDPID) {
                setBDPIDTunings();
            } else {
                setNormalPIDTunings();
            }
        }
    }

    // Steam on
    if (machinestate == kSteam) {
        if (lastmachinestatepid != machinestate) {
            debugPrintf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", 150.0, 0.0, 0.0);
            lastmachinestatepid = machinestate;
        }

        bPID.SetTunings(steamKp, 0, 0, PonE);
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
            debugPrintf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", aggbKp, aggbKi, aggbKd);
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
    writeSysParamsToStorage();
}

void setNormalPIDTunings() {
    // Prevent overwriting of brewdetection values
    // calc ki, kd
    if (aggTn != 0) {
        aggKi = aggKp / aggTn;
    } else {
        aggKi = 0;
    }

    aggKd = aggTv * aggKp;

    bPID.SetIntegratorLimits(0, aggIMax);

    if (lastmachinestatepid != machinestate) {
        debugPrintf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", aggKp, aggKi, aggKd);
        lastmachinestatepid = machinestate;
    }

    bPID.SetTunings(aggKp, aggKi, aggKd, PonE);
}

void setBDPIDTunings() {
    // calc ki, kd
    if (aggbTn != 0) {
        aggbKi = aggbKp / aggbTn;
    } else {
        aggbKi = 0;
    }

    aggbKd = aggbTv * aggbKp;

    if (lastmachinestatepid != machinestate) {
        debugPrintf("new PID-Values: P=%.1f  I=%.1f  D=%.1f\n", aggbKp, aggbKi, aggbKd);
        lastmachinestatepid = machinestate;
    }

    bPID.SetTunings(aggbKp, aggbKi, aggbKd, PonE);
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
    if (sysParaPidIMaxReg.getStorage() != 0) return -1;
    if (sysParaPidKpBd.getStorage() != 0) return -1;
    if (sysParaPidTnBd.getStorage() != 0) return -1;
    if (sysParaPidTvBd.getStorage() != 0) return -1;
    if (sysParaBrewSetPoint.getStorage() != 0) return -1;
    if (sysParaTempOffset.getStorage() != 0) return -1;
    if (sysParaBrewTime.getStorage() != 0) return -1;
    if (sysParaBrewSwTimer.getStorage() != 0) return -1;
    if (sysParaBrewThresh.getStorage() != 0) return -1;
    if (sysParaPreInfTime.getStorage() != 0) return -1;
    if (sysParaPreInfPause.getStorage() != 0) return -1;
    if (sysParaWeightSetPoint.getStorage() != 0) return -1;
    if (sysParaPidOn.getStorage() != 0) return -1;
    if (sysParaPidKpSteam.getStorage() != 0) return -1;
    if (sysParaUsePonM.getStorage() != 0) return -1;
    if (sysParaUseBDPID.getStorage() != 0) return -1;

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
    if (sysParaPidIMaxReg.setStorage() != 0) return -1;
    if (sysParaPidKpBd.setStorage() != 0) return -1;
    if (sysParaPidTnBd.setStorage() != 0) return -1;
    if (sysParaPidTvBd.setStorage() != 0) return -1;
    if (sysParaBrewSetPoint.setStorage() != 0) return -1;
    if (sysParaTempOffset.setStorage() != 0) return -1;
    if (sysParaBrewTime.setStorage() != 0) return -1;
    if (sysParaBrewSwTimer.setStorage() != 0) return -1;
    if (sysParaBrewThresh.setStorage() != 0) return -1;
    if (sysParaPreInfTime.setStorage() != 0) return -1;
    if (sysParaPreInfPause.setStorage() != 0) return -1;
    if (sysParaWeightSetPoint.setStorage() != 0) return -1;
    if (sysParaPidOn.setStorage() != 0) return -1;
    if (sysParaPidKpSteam.setStorage() != 0) return -1;
    if (sysParaUsePonM.setStorage() != 0) return -1;
    if (sysParaUseBDPID.setStorage() != 0) return -1;

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

        Blynk.virtualWrite(V11, startKp);
        Blynk.virtualWrite(V14, startTn);
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
            mqtt_publish("BrewTempOffset", number2string(BrewTempOffset));
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
            mqtt_publish("aggIMax", number2string(aggIMax));

            // BD PID
            mqtt_publish("aggbKp", number2string(aggbKp));
            mqtt_publish("aggbTn", number2string(aggbTn));
            mqtt_publish("aggbTv", number2string(aggbTv));

            // Start PI
            mqtt_publish("startKp", number2string(startKp));
            mqtt_publish("startTn", number2string(startTn));

             // Steam P
            mqtt_publish("steamKp", number2string(steamKp));

            //BD Parameter
            mqtt_publish("BrewTimer", number2string(brewtimersoftware));
            mqtt_publish("BrewLimit", number2string(brewsensitivity));

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
const String getFwVersion(void) {
    static const String sysVersion = String(FW_VERSION) + "." +
                                     String(FW_SUBVERSION) + "." +
                                     String(FW_HOTFIX);
    return sysVersion;
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
