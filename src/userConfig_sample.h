/**
 * @file    userConfig_sample.h
 * @brief   Values must be configured by the user
 * @version 4.0.0 Master
 *
 */

#pragma once

// firmware version (must match with definitions in the main source file)
#define USR_FW_VERSION    4
#define USR_FW_SUBVERSION 0
#define USR_FW_HOTFIX     0
#define USR_FW_BRANCH     "MASTER"

// List of supported machines
enum MACHINE {
    RancilioSilvia,  // MACHINEID 0
    RancilioSilviaE, // MACHINEID 1
    Gaggia,          // MACHINEID 2
    QuickMill        // MACHINEID 3
};

/**
 * Preconfiguration
 */

// Machine
#define MACHINEID RancilioSilvia // Machine type (see enum MACHINE)

// Display
#define OLED_DISPLAY          2       // 0 = deactivated, 1 = SH1106 (e.g. 1.3 "128x64), 2 = SSD1306 (e.g. 0.96" 128x64), 3 = SH1106_126x64_SPI
#define OLED_I2C              0x3C    // I2C address for OLED, 0x3C by default
#define DISPLAYTEMPLATE       3       // 1 = Standard display template, 2 = Minimal template, 3 = only temperature, 4 = scale template, 20 = vertical display (see git Handbook for further information)
#define DISPLAYROTATE         U8G2_R0 // rotate display clockwise: U8G2_R0 = no rotation; U8G2_R1 = 90°; U8G2_R2 = 180°; U8G2_R3 = 270°
#define SCREEN_WIDTH          128     // OLED display width, in pixels
#define SCREEN_HEIGHT         64      // OLED display height, in pixels
#define FEATURE_SHOTTIMER     1       // 0 = deactivated, 1 = activated (with weight if FEATURE_SCALE activated)
#define FEATURE_HEATINGLOGO   1       // 0 = deactivated, 1 = activated
#define FEATURE_PIDOFF_LOGO   1       // 0 = deactivated, 1 = activated
#define SHOTTIMERDISPLAYDELAY 3000    // time in ms that shot timer will be shown after brew finished

#define LANGUAGE 0                    // LANGUAGE = 0 (DE), LANGUAGE = 1 (EN), LANGUAGE = 2 (ES)

// Connectivity
#define CONNECTMODE         1              // 0 = offline 1 = WIFI-MODE
#define HOSTNAME            "silvia"
#define PASS                "CleverCoffee" // default password for WiFiManager
#define MAXWIFIRECONNECTS   5              // maximum number of reconnection attempts, use -1 to deactivate
#define WIFICONNECTIONDELAY 10000          // delay between reconnects in ms

// PID & Hardware
#define FEATURE_BREWCONTROL   0                       // 0 = deactivated, 1 = activated
#define FEATURE_BREWDETECTION 1                       // 0 = deactivated, 1 = activated
#define BREWDETECTION_TYPE    1                       // 1 = Software (FEATURE_BREWCONTROL 0), 2 = Hardware (FEATURE_BREWCONTROL 1), 3 = Optocoupler (FEATURE_BREWCONTROL 0)
#define FEATURE_POWERSWITCH   0                       // 0 = deactivated, 1 = activated
#define POWERSWITCH_TYPE      Switch::TOGGLE          // Switch::TOGGLE or Switch::MOMENTARY (trigger)
#define POWERSWITCH_MODE      Switch::NORMALLY_OPEN   // Switch::NORMALLY_OPEN or Switch::NORMALLY_CLOSED
#define FEATURE_BREWSWITCH    0                       // 0 = deactivated, 1 = activated
#define BREWSWITCH_TYPE       Switch::TOGGLE          // Switch::TOGGLE or Switch::MOMENTARY (trigger)
#define BREWSWITCH_MODE       Switch::NORMALLY_OPEN   // Switch::NORMALLY_OPEN or Switch::NORMALLY_CLOSED
#define FEATURE_WATERSWITCH   1                       // 0 = deactivated, 1 = activated
#define WATERSWITCH_TYPE      Switch::TOGGLE          // Switch::TOGGLE or Switch::MOMENTARY (trigger)
#define WATERSWITCH_MODE      Switch::NORMALLY_OPEN   // Switch::NORMALLY_OPEN or Switch::NORMALLY_CLOSED
#define FEATURE_STEAMSWITCH   0                       // 0 = deactivated, 1 = activated
#define STEAMSWITCH_TYPE      Switch::TOGGLE          // Switch::TOGGLE or Switch::MOMENTARY (trigger)
#define OPTOCOUPLER_TYPE      HIGH                    // BREWDETECTION 3 configuration; HIGH or LOW trigger optocoupler
#define STEAMSWITCH_MODE      Switch::NORMALLY_OPEN   // Switch::NORMALLY_OPEN or Switch::NORMALLY_CLOSED
#define HEATER_SSR_TYPE       Relay::HIGH_TRIGGER     // HIGH_TRIGGER = relay switches when input is HIGH, vice versa for LOW_TRIGGER
#define PUMP_VALVE_SSR_TYPE   Relay::HIGH_TRIGGER     // HIGH_TRIGGER = relay switches when input is HIGH, vice versa for LOW_TRIGGER
#define FEATURE_STATUS_LED    0                       // Blink status LED when temp is in range, 0 = deactivated, 1 = activated
#define FEATURE_BREW_LED      0                       // Turn on brew LED when brew is started, 0 = deactivated, 1 = activated
#define LED_TYPE              LED::STANDARD           // STANDARD_LED for an LED connected to a GPIO pin, WS2812 for adressable LEDs
#define FEATURE_WATER_SENS    0                       // 0 = deactivated, 1 = activated
#define WATER_SENS_TYPE       Switch::NORMALLY_CLOSED // Switch::NORMALLY_CLOSED for sensor XKC-Y25-NPN or Switch::NORMALLY_OPEN for XKC-Y25-PNP

#define FEATURE_PRESSURESENSOR 0                      // 0 = deactivated, 1 = activated

// Brew Scale
#define FEATURE_SCALE 0 // 0 = deactivated, 1 = activated
#define SCALE_TYPE    0 // 0 = one HX711 per load cell, 1 = Only a single HX711 with two channels
#define SCALE_SAMPLES 2 // Load cell sample rate

// PlatformIO OTA
#define OTA     true      // true = OTA activated, false = OTA deactivated
#define OTAPASS "otapass" // Password for OTA updates

// MQTT
#define FEATURE_MQTT                 0                 // 0 = deactivated, 1 = activated
#define MQTT_USERNAME                "mymqttuser"
#define MQTT_PASSWORD                "mymqttpass"
#define MQTT_TOPIC_PREFIX            "custom/Küche."   // topic will be "<MQTT_TOPIC_PREFIX><HOSTNAME>/<READING>"
#define MQTT_SERVER_IP               "XXX.XXX.XXX.XXX" // IP-Address of the MQTT Server
#define MQTT_SERVER_PORT             1883              // Port of the specified MQTT Server
#define MQTT_HASSIO_SUPPORT          0                 // Enables the Homeassistant Auto Discovery Feature
#define MQTT_HASSIO_DISCOVERY_PREFIX "homeassistant"   // Homeassistant Auto Discovery Prefix

// PID Parameters (not yet in Web interface)
#define EMA_FACTOR 0.6 // Smoothing of input that is used for Tv (derivative component of PID). Smaller means less smoothing but also less delay, 0 means no filtering

#define TEMP_SENSOR 2  // Temp sensor type: 1 = DS18B20, 2 = TSIC306

// Log level for serial console, valid options (all with Logger::Level:: prefix) are:
// TRACE, DEBUG, INFO, WARNING, ERROR, FATAL
#define LOGLEVEL Logger::Level::INFO
