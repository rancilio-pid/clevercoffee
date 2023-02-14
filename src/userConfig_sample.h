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
    RancilioSilvia,   // MACHINEID 0
    RancilioSilviaE,  // MACHINEID 1
    Gaggia,           // MACHINEID 2
    QuickMill         // MACHINEID 3
};

/**
 * Preconfiguration
 */

// Machine
#define MACHINEID 0                // see above list of supported machines

// Display
#define OLED_DISPLAY 2             // 0 = deactivated, 1 = SH1106 (e.g. 1.3 "128x64), 2 = SSD1306 (e.g. 0.96" 128x64), 3 = SH1106_126x64_SPI
#define OLED_I2C 0x3C              // I2C address for OLED, 0x3C by default
#define DISPLAYTEMPLATE 3          // 1 = Standard display template, 2 = Minimal template, 3 = only temperature, 4 = scale template, 20 = vertical display (see git Handbook for further information)
#define DISPLAYROTATE U8G2_R0      // rotate display clockwise: U8G2_R0 = no rotation; U8G2_R1 = 90°; U8G2_R2 = 180°; U8G2_R3 = 270°
#define SCREEN_WIDTH 128           // OLED display width, in pixels
#define SCREEN_HEIGHT 64           // OLED display height, in pixels
#define SHOTTIMER 1                // 0 = deactivated, 1 = activated 2 = with scale
#define HEATINGLOGO 0              // 0 = deactivated, 1 = Rancilio, 2 = Gaggia
#define OFFLINEGLOGO 1             // 0 = deactivated, 1 = activated
#define BREWSWITCHDELAY 3000       // time in ms that the brew switch will be delayed (shot timer will show that much longer after switching off)
#define VERBOSE 0                  // 1 = Show verbose output (serial connection), 0 = show less

#define LANGUAGE 0                 // LANGUAGE = 0 (DE), LANGUAGE = 1 (EN), LANGUAGE = 2 (ES)

// Connectivity
#define CONNECTMODE 1              // 0 = offline 1 = WIFI-MODE
#define HOSTNAME "silvia"
#define PASS "CleverCoffee"        // default password for WiFiManager
#define MAXWIFIRECONNECTS 5        // maximum number of reconnection attempts, use -1 to deactivate
#define WIFICONNECTIONDELAY 10000  // delay between reconnects in ms

// PID & Hardware
#define ONLYPID 1                  // 1 = Only PID, 0 = PID and preinfusion
#define ONLYPIDSCALE 0             // 0 = off , 1 = OnlyPID with Scale
#define BREWMODE 1                 // 1 = Brew by time (with preinfusion); 2 = Brew by weight (from scale)
#define BREWDETECTION 0            // 0 = off, 1 = Software (Onlypid 1), 2 = Hardware (Onlypid 0), 3 = Sensor/Hardware for Only PID
#define BREWSWITCHTYPE 1           // 1 = normal Switch, 2 = Trigger Switch
#define POWERSWITCHTYPE 0          // 0 = no switch connected, 1 = normal Switch, 2 = Trigger Switch
#define STEAMSWITCHTYPE 0          // 0 = no switch connected, 1 = normal Switch, 2 = Trigger Switch
#define TRIGGERTYPE HIGH           // LOW = low trigger, HIGH = high trigger relay for pump & valve
#define VOLTAGESENSORTYPE HIGH     // BREWDETECTION 3 configuration
#define PINMODEVOLTAGESENSOR INPUT // Mode INPUT_PULLUP, INPUT or INPUT_PULLDOWN_16 (Only Pin 16)
#define PRESSURESENSOR 0           // 1 = pressure sensor connected
#define TEMP_LED 1                 // Blink status LED when temp is in range

// Brew Scale
#define SCALE_SAMPLES 2                     // Load cell sample rate
#define SCALE_CALIBRATION_FACTOR 3195.83    // Raw data is divided by this value to convert to readable data

/* Pressure sensor
 *
 * measure and verify "offset" value, should be 10% of ADC bit reading @supply volate (3.3V)
 * same goes for "fullScale", should be 90%
 */
#define OFFSET      102            // 10% of ADC input @3.3V supply = 102
#define FULLSCALE   922            // 90% of ADC input @3.3V supply = 922
#define MAXPRESSURE 200

// PlatformIO OTA
#define OTA true                   // true = OTA activated, false = OTA deactivated
#define OTAHOST "silvia"           // Name to be shown in Arduino IDE/PlatformIO port
#define OTAPASS "otapass"          // Password for OTA updates

// MQTT
#define MQTT 0                             // 1 = MQTT enabled, 0 = MQTT disabled
#define MQTT_USERNAME "mymqttuser"
#define MQTT_PASSWORD "mymqttpass"
#define MQTT_TOPIC_PREFIX "custom/Küche."               // topic will be "<MQTT_TOPIC_PREFIX><HOSTNAME>/<READING>"
#define MQTT_SERVER_IP "XXX.XXX.XXX.XXX"                // IP-Address of the MQTT Server
#define MQTT_SERVER_PORT 1883                           // Port of the specified MQTT Server
#define MQTT_HASSIO_SUPPORT 0                           // Enables the Homeassistant Auto Discovery Feature
#define MQTT_HASSIO_DISCOVERY_PREFIX "homeassistant"    // Homeassistant Auto Discovery Prefix

// INFLUXDB
#define INFLUXDB 0                 // 1 = INFLUX enabled, 0 = INFLUX disabled
#define INFLUXDB_URL ""            // InfluxDB server address
#define INFLUXDB_INSECURE 1        // 1 = INFLUXClient setInsecure enabled , 0 = INFLUXClient setInsecure disabled
#define INFLUXDB_AUTH_TYPE 1       // 1 = API Token , 2 = User/Pass
#define INFLUXDB_API_TOKEN ""
#define INFLUXDB_ORG_NAME ""
#define INFLUXDB_USER ""
#define INFLUXDB_PASSWORD ""
#define INFLUXDB_DB_NAME "coffee"  // InfluxDB bucket name
#define INFLUXDB_INTERVAL 5000     // Send interval in milliseconds
#define INFLUXDB_TIMEOUT 5000      // InfluxDB httpReadTimeout
#define INFLUXDB_RETRIES 50         // Amount of retries to etablish an Influxdb Connection before disabling InfluxDB at all

// PID Parameters (not yet in Web interface)
#define EMA_FACTOR 0.6             // Smoothing of input that is used for Tv (derivative component of PID). Smaller means less smoothing but also less delay, 0 means no filtering

#define TEMPSENSOR 2               // Temp sensor type: 1 = DS18B20, 2 = TSIC306

