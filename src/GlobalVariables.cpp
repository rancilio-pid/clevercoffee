/**
 * @file GlobalVariables.cpp
 * @brief Global variable definitions for CleverCoffee
 *
 * This file contains the actual storage/definitions of all global variables.
 * The declarations are in GlobalVariables.h
 */

#include "GlobalVariables.h"
#include "defaults.h"

// PID Parameters
bool pidON = false;
bool usePonM = false;
double aggKp = AGGKP;
double aggTn = AGGTN;
double aggTv = AGGTV;
double aggIMax = AGGIMAX;
double steamKp = STEAMKP;
double emaFactor = EMA_FACTOR;

// Brew Detection PID
bool useBDPID = false;
double aggbKp = AGGBKP;
double aggbTn = AGGBTN;
double aggbTv = AGGBTV;

// Temperature Parameters
double brewSetpoint = SETPOINT;
double brewTempOffset = TEMPOFFSET;
double brewPidDelay = BREW_PID_DELAY;
double steamSetpoint = STEAMSETPOINT;

// Brew Parameters
double targetBrewTime = TARGET_BREW_TIME;
double preinfusion = PRE_INFUSION_TIME;
double preinfusionPause = PRE_INFUSION_PAUSE_TIME;
double targetBrewWeight = TARGET_BREW_WEIGHT;
int backflushCycles = BACKFLUSH_CYCLES;
double backflushFillTime = BACKFLUSH_FILL_TIME;
double backflushFlushTime = BACKFLUSH_FLUSH_TIME;

// Power Management
bool standbyModeOn = false;
double standbyModeTime = STANDBY_MODE_TIME;

// Display Features
bool featureFullscreenBrewTimer = false;
bool featureFullscreenManualFlushTimer = false;
bool featureFullscreenHotWaterTimer = false;
double postBrewTimerDuration = POST_BREW_TIMER_DURATION;
bool featureHeatingLogo = true;
bool featurePidOffLogo = true;

// Runtime variables (not configurable)
bool steamON = false;
bool backflushOn = false;
double temperature = 0.0;
bool scaleTareOn = false;
bool scaleCalibrationOn = false;
const char sysVersion[64] = "4.0.0-beta2+feat-ui.3cfd5e7";
String systemVersion = String(sysVersion);
bool includeDisplayInLogs = true;
bool timingDebugActive = false;

// System settings
String hostname = HOSTNAME;
String otaPassword = OTAPASS;
bool offlineMode = false;
bool authEnabled = false;
String authUsername = AUTH_USERNAME;
String authPassword = AUTH_PASSWORD;

// Display settings
int displayTemplate = 0;
bool displayInverted = false;
int displayLanguage = 0;

// Hardware - OLED
bool oledEnabled = true;
int oledType = 0;
int oledAddress = 0;

// Hardware - Relays
int heaterTriggerType = 1;
int valveTriggerType = 1;
int pumpTriggerType = 1;

// Hardware - Switches
bool brewSwitchEnabled = false;
int brewSwitchType = 1;
int brewSwitchMode = 0;
bool steamSwitchEnabled = false;
int steamSwitchType = 1;
int steamSwitchMode = 0;
bool powerSwitchEnabled = false;
int powerSwitchType = 1;
int powerSwitchMode = 0;
bool hotWaterSwitchEnabled = false;
int hotWaterSwitchType = 1;
int hotWaterSwitchMode = 0;

// Hardware - LEDs
bool statusLedEnabled = false;
bool statusLedInverted = false;
bool brewLedEnabled = false;
bool brewLedInverted = false;
bool steamLedEnabled = false;
bool steamLedInverted = false;

// Hardware - Sensors
int temperatureSensorType = 0;
bool pressureSensorEnabled = false;
bool waterTankSensorEnabled = false;
int waterTankSensorMode = 1;

// Scale settings
bool scaleEnabled = false;
int scaleSamples = SCALE_SAMPLES;
int scaleType = 0;
double scaleCalibrationFactor = SCALE_CALIBRATION_FACTOR;
double scaleCalibrationFactor2 = SCALE_CALIBRATION_FACTOR;
double scaleKnownWeight = SCALE_KNOWN_WEIGHT;

// MQTT settings
bool mqttEnabled = false;
String mqttBroker = "";
int mqttPort = 1883;
String mqttUsername = MQTT_USERNAME;
String mqttPassword = MQTT_PASSWORD;
String mqttTopic = MQTT_TOPIC;
bool mqttHassioEnabled = false;
String mqttHassioPrefix = MQTT_HASSIO_PREFIX;

// Additional variables that were in Config.cpp but missing from GlobalVariables.h
bool brewByTimeEnabled = false;
bool brewByWeightEnabled = false;
bool brewByWeightAutoTare = false;
bool preinfusionEnabled = false;
