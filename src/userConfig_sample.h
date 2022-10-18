/**
 * @file    userConfig_sample.h
 * @brief   Values must be configured by the user
 * @version 3.1.0 Master
 *
 */
#ifndef _userConfig_H
#define _userConfig_H

// firmware version (must match with definitions in the main source file)
#define USR_FW_VERSION    3
#define USR_FW_SUBVERSION 1
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
#define OLED_DISPLAY 2             // 0 = deactivated, 1 = SH1106 (e.g. 1.3 "128x64), 2 = SSD1306 (e.g. 0.96" 128x64)
#define OLED_I2C 0x3C              // I2C address for OLED, 0x3C by default
#define DISPLAYTEMPLATE 3          // 1 = Standard display template, 2 = Minimal template, 3 = only temperature, 4 = scale template, 20 = vertical display (see git Handbook for further information)
#define DISPLAYROTATE U8G2_R0      // rotate display clockwise: U8G2_R0 = no rotation; U8G2_R1 = 90°; U8G2_R2 = 180°; U8G2_R3 = 270°
#define SHOTTIMER 1                // 0 = deactivated, 1 = activated 2 = with scale
#define HEATINGLOGO 0              // 0 = deactivated, 1 = Rancilio, 2 = Gaggia
#define OFFLINEGLOGO 1             // 0 = deactivated, 1 = activated
#define BREWSWITCHDELAY 3000       // time in ms that the brew switch will be delayed (shot timer will show that much longer after switching off)
#define VERBOSE 0                  // 1 = Show verbose output (serial connection), 0 = show less

// Connectivity
#define CONNECTMODE 1              // 0 = offline 1 = WIFI-MODE 2 = AP-MODE (not working in the moment)
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
#define TRIGGERTYPE HIGH           // LOW = low trigger, HIGH = high trigger relay
#define VOLTAGESENSORTYPE HIGH     // BREWDETECTION 3 configuration
#define PINMODEVOLTAGESENSOR INPUT // Mode INPUT_PULLUP, INPUT or INPUT_PULLDOWN_16 (Only Pin 16)
#define PRESSURESENSOR 0           // 1 = pressure sensor connected to A0; PINBREWSWITCH must be set to the connected input!
#define TEMPLED 0                  // set led pin high when brew or steam set point is within range

// TOF sensor for water level
#define TOF 0                      // 0 = no TOF sensor connected; 1 = water level by TOF sensor
#define TOF_I2C 0x29               // I2C address of TOF sensor; 0x29 by default
#define CALIBRATION_MODE 0         // 1 = enable to obtain water level calibration values; 0 = disable for normal PID operation; can also be done in Blynk
#define WATER_FULL 102             // value for full water tank (=100%) obtained in calibration procedure (in mm); can also be set in Blynk
#define WATER_EMPTY 205            // value for empty water tank (=0%) obtained in calibration procedure (in mm); can also be set in Blynk

// E-Trigger
#define ETRIGGER 0                 // 0 = no trigger (for Rancilio except Rancilio E), 1 = trigger for CPU of Rancilio E
#define ETRIGGERTIME 600           // seconds, time between trigger signal
#define TRIGGERRELAYTYPE HIGH      // LOW = low trigger, HIGH = high trigger relay for E-Trigger

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
#define MQTT_TOPIC_PREFIX "custom/Küche."  // topic will be "<MQTT_TOPIC_PREFIX><HOSTNAME>/<READING>"
#define MQTT_SERVER_IP "XXX.XXX.XXX.XXX"   // IP-Address of locally installed mqtt server
#define MQTT_SERVER_PORT 1883

// INFLUXDB
#define INFLUXDB 0                 // 1 = INFLUX enabled, 0 = INFLUX disabled
#define INFLUXDB_URL ""            // InfluxDB server address
#define INFLUXDB_AUTH_TYPE 1       // 1 = API Token , 2 = User/Pass
#define INFLUXDB_API_TOKEN ""
#define INFLUXDB_ORG_NAME ""
#define INFLUXDB_USER ""
#define INFLUXDB_PASSWORD ""
#define INFLUXDB_DB_NAME "coffee"  // InfluxDB bucket name
#define INFLUXDB_INTERVAL 5000     // Send interval in milliseconds

// PID Parameters (not yet in Web interface)
#define EMA_FACTOR 0.6             // Smoothing of input that is used for Tv (derivative component of PID). Smaller means less smoothing but also less delay, 0 means no filtering
#define BREWPID_DELAY 10           // delay until enabling PID controller during brew (no heating during this time)

// Backflush values
#define FILLTIME 3000              // time in ms the pump is running
#define FLUSHTIME 6000             // time in ms the 3-way valve is open -> backflush
#define MAXFLUSHCYCLES 5           // number of cycles the backflush should run, 0 = disabled

#define TEMPSENSOR 2               // Temp sensor type: 1 = DS18B20, 2 = TSIC306, 3 = MAX31865

// Pin Layout
#define PINTEMPSENSOR 2
#define PINPRESSURESENSOR 99
#define PINVALVE 12
#define PINPUMP 13
#define PINHEATER 14
#define PINVOLTAGESENSOR 15        // Input pin for voltage sensor (optocoupler to detect brew switch)
#define PINETRIGGER 16             // PIN for E-Trigger relay
#define PINBREWSWITCH 0
#define PINSTEAMSWITCH 17
#define LEDPIN    18               // LED PIN ON near setpoint
#define OLED_SCL 5                 // Output pin for display clock pin
#define OLED_SDA 4                 // Output pin for display data pin
#define HXDATPIN 99                // weight scale data pin
#define HXCLKPIN 99                // weight scale clock pin
#define SCREEN_WIDTH 128           // OLED display width, in pixels
#define SCREEN_HEIGHT 64           // OLED display height, in pixels
#define MAX31865_MOSI 23           // MAX31865 Temp sensor SPI MOSI (VSPI default 23)
#define MAX31865_MISO 19           // MAX31865 Temp sensor SPI MISO (VSPI default 19)
#define MAX31865_CLK  18           // MAX31865 Temp sensor SPI Clock (VSPI default 18)
#define MAX31865_CS   5            // MAX31865 Temp sensor Chip Select (VSPI default 5)
#define MAX31865_PT100 true        // MAX31865 type. PT1000 if false

// Check BrewSwitch
#if (defined(ESP8266) && ((PINBREWSWITCH != 15 && PINBREWSWITCH != 0 && PINBREWSWITCH != 16 )))
  #error("WRONG Brewswitch PIN for ESP8266, Only PIN 15 and PIN 16");
#endif

// defined compiler errors
#if (PRESSURESENSOR == 1) && (PINPRESSURESENSOR == 0) && (PINBREWSWITCH == 0)
  #error Change PINBREWSWITCH or PRESSURESENSOR!
#endif

#endif
