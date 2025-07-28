#pragma once

#include <Arduino.h>

// Existing parameter global variables
extern bool pidON;
extern bool usePonM;
extern double aggKp;
extern double aggTn;
extern double aggTv;
extern double aggIMax;
extern double steamKp;
extern double brewSetpoint;
extern double brewTempOffset;
extern double brewPidDelay;
extern bool useBDPID;
extern double aggbKp;
extern double aggbTn;
extern double aggbTv;
extern double emaFactor;
extern double steamSetpoint;
extern double targetBrewTime;
extern double preinfusion;
extern double preinfusionPause;
extern double targetBrewWeight;
extern int backflushCycles;
extern double backflushFillTime;
extern double backflushFlushTime;
extern bool standbyModeOn;
extern double standbyModeTime;
extern bool featureFullscreenBrewTimer;
extern bool featureFullscreenManualFlushTimer;
extern bool featureFullscreenHotWaterTimer;
extern double postBrewTimerDuration;
extern bool featureHeatingLogo;
extern bool featurePidOffLogo;
extern bool steamON;
extern bool backflushOn;
extern double temperature;
extern bool scaleTareOn;
extern bool scaleCalibrationOn;
extern const char sysVersion[64];
extern String systemVersion;
extern bool includeDisplayInLogs;
extern bool timingDebugActive;

// System settings
extern String hostname;
extern String otaPassword;
extern bool offlineMode;
extern bool authEnabled;
extern String authUsername;
extern String authPassword;

// Display settings
extern int displayTemplate;
extern bool displayInverted;
extern int displayLanguage;

// Hardware - OLED
extern bool oledEnabled;
extern int oledType;
extern int oledAddress;

// Hardware - Relays
extern int heaterTriggerType;
extern int valveTriggerType;
extern int pumpTriggerType;

// Hardware - Switches
extern bool brewSwitchEnabled;
extern int brewSwitchType;
extern int brewSwitchMode;
extern bool steamSwitchEnabled;
extern int steamSwitchType;
extern int steamSwitchMode;
extern bool powerSwitchEnabled;
extern int powerSwitchType;
extern int powerSwitchMode;
extern bool hotWaterSwitchEnabled;
extern int hotWaterSwitchType;
extern int hotWaterSwitchMode;

// Hardware - LEDs
extern bool statusLedEnabled;
extern bool statusLedInverted;
extern bool brewLedEnabled;
extern bool brewLedInverted;
extern bool steamLedEnabled;
extern bool steamLedInverted;

// Hardware - Sensors
extern int temperatureSensorType;
extern bool pressureSensorEnabled;
extern bool waterTankSensorEnabled;
extern int waterTankSensorMode;

// Scale settings
extern bool scaleEnabled;
extern int scaleSamples;
extern int scaleType;
extern double scaleCalibrationFactor;
extern double scaleCalibrationFactor2;
extern double scaleKnownWeight;

// MQTT settings
extern bool mqttEnabled;
extern String mqttBroker;
extern int mqttPort;
extern String mqttUsername;
extern String mqttPassword;
extern String mqttTopic;
extern bool mqttHassioEnabled;
extern String mqttHassioPrefix;

// Brew control flags
extern bool brewByTimeEnabled;
extern bool brewByWeightEnabled;
extern bool brewByWeightAutoTare;
extern bool preinfusionEnabled;

// External reference to logLevel from Logger library
extern int logLevel;
