/**
 * @file defaults.h
 *
 * @brief Default values for system parameters
 *
 */

#pragma once

// system parameter defaults and ranges
#define STR_HELPER(x) #x
#define STR(x)        STR_HELPER(x)

// default parameters
#define HOSTNAME                 "silvia"          // default hostname
#define OTAPASS                  "otapass"         // default password for over-the-air updates
#define WM_PASS                  "CleverCoffee"    // default password for WiFiManager
#define SETPOINT                 95.0              // brew temperature setpoint
#define TEMPOFFSET               0.0               // brew temperature setpoint
#define STEAMSETPOINT            120.0             // steam temperature setpoint
#define SCALE_CALIBRATION_FACTOR 1.00              // Raw data is divided by this value to convert to readable data
#define SCALE_KNOWN_WEIGHT       267.00            // Calibration weight for scale (weight of the tray)
#define SCALE_SAMPLES            2                 // Number of samples used for calibration
#define AGGKP                    62.0              // PID Kp (regular phase)
#define AGGTN                    52.0              // PID Tn (regular phase)
#define AGGTV                    11.5              // PID Tv (regular phase)
#define AGGIMAX                  55.0              // PID Integrator Max (regular phase)
#define STEAMKP                  150.0             // PID kp (steam phase)
#define AGGBKP                   50.0              // PID Kp (brew detection phase)
#define AGGBTN                   0.0               // PID Tn (brew detection phase)
#define AGGBTV                   20.0              // PID Tv (brew detection phase)
#define EMA_FACTOR               0.6               // Smoothing of input that is used for Tv (derivative component of PID). Smaller means less smoothing but also less delay, 0 means no filtering
#define TARGET_BREW_TIME         25.0              // brew time in seconds (only used if pump is being controlled)
#define BREW_PID_DELAY           10.0              // delay until enabling PID controller during brew (no heating during this time)
#define PRE_INFUSION_TIME        2.0               // pre-infusion time in seconds
#define PRE_INFUSION_PAUSE_TIME  5.0               // pre-infusion pause time in seconds
#define TARGET_BREW_WEIGHT       36.0              // Target weight in grams
#define STANDBY_MODE_TIME        35.0              // Time in minutes until the heater is turned off
#define BACKFLUSH_CYCLES         5                 // number of cycles the backflush should run
#define BACKFLUSH_FILL_TIME      5.0               // time in seconds the pump is running during backflush
#define BACKFLUSH_FLUSH_TIME     10.0              // time in seconds the 3-way valve is open during backflush
#define POST_BREW_TIMER_DURATION 3.0               // time in seconds that brew timer will be shown after brew finished
#define BLINKING_DELTA           0.3               // distance between measured temperature and setpoint to enable display blinking
#define MAXWIFIRECONNECTS        5                 // maximum number of reconnection attempts, use -1 to deactivate
#define WIFICONNECTIONDELAY      10000             // delay between reconnects in ms
#define MQTT_USERNAME            "rancilio"        // default MQTT username
#define MQTT_PASSWORD            "silvia"          // default MQTT password
#define MQTT_TOPIC               "custom/kitchen/" // default MQTT topic prefix
#define MQTT_HASSIO_PREFIX       "homeassistant"   // default MQTT prefix for Home Assistant
#define SCREEN_WIDTH             128               // OLED display width, in pixels
#define SCREEN_HEIGHT            64                // OLED display height, in pixels
#define AUTH_PASSWORD            "admin"           // default password for web authentication
#define AUTH_USERNAME            "admin"           // default username for web authentication
#define PSM_PRESSURE_KP          20.0              // PID PSM control Pressure target Kp
#define PSM_PRESSURE_KI          10.0              // PID PSM control Pressure target Ki
#define PSM_PRESSURE_KD          1.5               // PID PSM control Pressure target Kd
#define PSM_FLOW_KP              8.0               // PID PSM control Flow target Kp
#define PSM_FLOW_KI              30.0              // PID PSM control Flow target Ki
#define PSM_FLOW_KD              0.0               // PID PSM control Flow target Kd
#define PHASE_PRESSURE_KP        10.0              // PID Phase control Pressure target Kp
#define PHASE_PRESSURE_KI        20.0              // PID Phase control Pressure target Ki
#define PHASE_PRESSURE_KD        1.0               // PID Phase control Pressure target Kd
#define PHASE_FLOW_KP            2.0               // PID Phase control Flow target Kp
#define PHASE_FLOW_KI            15.0              // PID Phase control Flow target Ki
#define PHASE_FLOW_KD            0.0               // PID Phase control Flow target Kd
#define PUMP_I_MAX               100.0             // PID Dimmer integral anti windup
#define PUMP_POWER_SETPOINT      100.0             // PID Power target
#define PUMP_PRESSURE_SETPOINT   9.0               // PID Pressure target
#define PUMP_FLOW_SETPOINT       8.0               // PID Flow target
#define PUMP_CALIBRATE_FLOW1     292.4             // Weight of water out of the group head in 30s
#define PUMP_CALIBRATE_FLOW2     124.4             // Weight of water out of the return line in 30s
#define PUMP_OPV_PRESSURE        10.0              // Pressure when OPV valve is open

#define PID_KP_REGULAR_MIN            0.0
#define PID_KP_REGULAR_MAX            999.0
#define PID_TN_REGULAR_MIN            0.0
#define PID_TN_REGULAR_MAX            999.0
#define PID_TV_REGULAR_MIN            0.0
#define PID_TV_REGULAR_MAX            999.0
#define PID_I_MAX_REGULAR_MIN         0.0
#define PID_I_MAX_REGULAR_MAX         999.0
#define PID_KP_BD_MIN                 0.0
#define PID_KP_BD_MAX                 999.0
#define PID_TN_BD_MIN                 0.0
#define PID_TN_BD_MAX                 999.0
#define PID_TV_BD_MIN                 0.0
#define PID_TV_BD_MAX                 999.0
#define PID_EMA_FACTOR_MIN            0.0
#define PID_EMA_FACTOR_MAX            1.0
#define BREW_SETPOINT_MIN             20.0
#define BREW_SETPOINT_MAX             110.0
#define STEAM_SETPOINT_MIN            100.0
#define STEAM_SETPOINT_MAX            140.0
#define BREW_TEMP_OFFSET_MIN          0.0
#define BREW_TEMP_OFFSET_MAX          20.0
#define TARGET_BREW_TIME_MIN          1.0
#define TARGET_BREW_TIME_MAX          120.0
#define BREW_PID_DELAY_MIN            0.0
#define BREW_PID_DELAY_MAX            60.0
#define PRE_INFUSION_TIME_MIN         0.0
#define PRE_INFUSION_TIME_MAX         60.0
#define PRE_INFUSION_PAUSE_MIN        0.0
#define PRE_INFUSION_PAUSE_MAX        60.0
#define TARGET_BREW_WEIGHT_MIN        0.0
#define TARGET_BREW_WEIGHT_MAX        500.0
#define PID_KP_STEAM_MIN              0.0
#define PID_KP_STEAM_MAX              999.0
#define STANDBY_MODE_TIME_MIN         1.0
#define STANDBY_MODE_TIME_MAX         120.0
#define BACKFLUSH_CYCLES_MIN          2
#define BACKFLUSH_CYCLES_MAX          20
#define BACKFLUSH_FILL_TIME_MIN       3.0
#define BACKFLUSH_FILL_TIME_MAX       10.0
#define BACKFLUSH_FLUSH_TIME_MIN      5.0
#define BACKFLUSH_FLUSH_TIME_MAX      20.0
#define POST_BREW_TIMER_DURATION_MIN  0.0
#define POST_BREW_TIMER_DURATION_MAX  60.0
#define BLINKING_DELTA_MIN            0.2
#define BLINKING_DELTA_MAX            10.0
#define SCALE_SAMPLES_MIN             1
#define SCALE_SAMPLES_MAX             20
#define SCALE_CALIBRATION_MIN         (-999999.0)
#define SCALE_CALIBRATION_MAX         999999.0
#define SCALE_KNOWN_WEIGHT_MIN        1.0
#define SCALE_KNOWN_WEIGHT_MAX        2000.0
#define MQTT_BROKER_MAX_LENGTH        64
#define USERNAME_MAX_LENGTH           32
#define PASSWORD_MAX_LENGTH           64
#define MQTT_TOPIC_MAX_LENGTH         48
#define MQTT_HASSIO_PREFIX_MAX_LENGTH 24
#define HOSTNAME_MAX_LENGTH           64
#define PUMP_KP_MIN                   0.0
#define PUMP_KP_MAX                   50.0
#define PUMP_KI_MIN                   0.0
#define PUMP_KI_MAX                   50.0
#define PUMP_KD_MIN                   0.0
#define PUMP_KD_MAX                   50.0
#define PUMP_I_MAX_MIN                0.0
#define PUMP_I_MAX_MAX                100.0
#define PUMP_POWER_SETPOINT_MIN       0.0
#define PUMP_POWER_SETPOINT_MAX       100.0
#define PUMP_PRESSURE_SETPOINT_MIN    0.0
#define PUMP_PRESSURE_SETPOINT_MAX    12.0
#define PUMP_FLOW_SETPOINT_MIN        0.0
#define PUMP_FLOW_SETPOINT_MAX        12.0
#define PUMP_CALIBRATION_FLOW_MIN     0.0
#define PUMP_CALIBRATION_FLOW_MAX     600.0
