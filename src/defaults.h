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
#define SETPOINT                              95.0   // brew temperature setpoint
#define TEMPOFFSET                            0.0    // brew temperature setpoint
#define STEAMSETPOINT                         120.0  // steam temperature setpoint
#define SCALE_CALIBRATION_FACTOR              1.00   // Raw data is divided by this value to convert to readable data
#define SCALE2_CALIBRATION_FACTOR             1.00   // Raw data is divided by this value to convert to readable data
#define SCALE_KNOWN_WEIGHT                    267.00 // Calibration weight for scale (weight of the tray)
#define AGGKP                                 62.0   // PID Kp (regular phase)
#define AGGTN                                 52.0   // PID Tn (regular phase)
#define AGGTV                                 11.5   // PID Tv (regular phase)
#define AGGIMAX                               55.0   // PID Integrator Max (regular phase)
#define STEAMKP                               150.0  // PID kp (steam phase)
#define AGGBKP                                50.0   // PID Kp (brew detection phase)
#define AGGBTN                                0.0    // PID Tn (brew detection phase)
#define AGGBTV                                20.0   // PID Tv (brew detection phase)
#define TARGET_BREW_TIME                      25.0   // brew time in seconds (only used if pump is being controlled)
#define BREW_PID_DELAY                        10.0   // delay until enabling PID controller during brew (no heating during this time)
#define PRE_INFUSION_TIME                     2.0    // pre-infusion time in seconds
#define PRE_INFUSION_PAUSE_TIME               5.0    // pre-infusion pause time in seconds
#define TARGET_BREW_WEIGHT                    30.0   // Target weight in grams
#define WIFI_CREDENTIALS_SAVED                0      // Flag if wifi setup is done. 0: not set up, 1: credentials set up via wifi manager
#define STANDBY_MODE_ON                       0      // Standby mode off by default
#define STANDBY_MODE_TIME                     30.0   // Time in minutes until the heater is turned off
#define BACKFLUSH_CYCLES                      5      // number of cycles the backflush should run
#define BACKFLUSH_FILL_TIME                   5.0    // time in seconds the pump is running during backflush
#define BACKFLUSH_FLUSH_TIME                  10.0   // time in seconds the 3-way valve is open during backflush
#define FEATURE_BREW_CONTROL                  0      // enables function to control pump and solenoid valve
#define FEATURE_FULLSCREEN_BREW_TIMER         0      // enables full screen brew timer
#define FEATURE_FULLSCREEN_MANUAL_FLUSH_TIMER 0      // enables full screen manual flush timer
#define POST_BREW_TIMER_DURATION              3.0    // time in seconds that brew timer will be shown after brew finished
#define FEATURE_HEATING_LOGO                  1      // enables full screen logo if mashine is heating
#define FEATURE_PID_OFF_LOGO                  1      // enables full screen logo if pid is switched off

#define PID_KP_REGULAR_MIN           0.0
#define PID_KP_REGULAR_MAX           999.0
#define PID_TN_REGULAR_MIN           0.0
#define PID_TN_REGULAR_MAX           999.0
#define PID_TV_REGULAR_MIN           0.0
#define PID_TV_REGULAR_MAX           999.0
#define PID_I_MAX_REGULAR_MIN        0.0
#define PID_I_MAX_REGULAR_MAX        999.0
#define PID_KP_BD_MIN                0.0
#define PID_KP_BD_MAX                999.0
#define PID_TN_BD_MIN                0.0
#define PID_TN_BD_MAX                999.0
#define PID_TV_BD_MIN                0.0
#define PID_TV_BD_MAX                999.0
#define BREW_SETPOINT_MIN            20.0
#define BREW_SETPOINT_MAX            110.0
#define STEAM_SETPOINT_MIN           100.0
#define STEAM_SETPOINT_MAX           140.0
#define BREW_TEMP_OFFSET_MIN         0.0
#define BREW_TEMP_OFFSET_MAX         20.0
#define TARGET_BREW_TIME_MIN         0.0
#define TARGET_BREW_TIME_MAX         180.0
#define BREW_PID_DELAY_MIN           0.0
#define BREW_PID_DELAY_MAX           60.0
#define PRE_INFUSION_TIME_MIN        0.0
#define PRE_INFUSION_TIME_MAX        60.0
#define PRE_INFUSION_PAUSE_MIN       0.0
#define PRE_INFUSION_PAUSE_MAX       60.0
#define TARGET_BREW_WEIGHT_MIN       0.0
#define TARGET_BREW_WEIGHT_MAX       500.0
#define PID_KP_STEAM_MIN             0.0
#define PID_KP_STEAM_MAX             500.0
#define STANDBY_MODE_TIME_MIN        30.0
#define STANDBY_MODE_TIME_MAX        120.0
#define BACKFLUSH_CYCLES_MIN         2
#define BACKFLUSH_CYCLES_MAX         20
#define BACKFLUSH_FILL_TIME_MIN      3.0
#define BACKFLUSH_FILL_TIME_MAX      10.0
#define BACKFLUSH_FLUSH_TIME_MIN     5.0
#define BACKFLUSH_FLUSH_TIME_MAX     20.0
#define POST_BREW_TIMER_DURATION_MIN 0.0
#define POST_BREW_TIMER_DURATION_MAX 60.0

#define MQTT_BROKER_MAX_LENGTH        64
#define MQTT_USERNAME_MAX_LENGTH      32
#define MQTT_PASSWORD_MAX_LENGTH      64
#define MQTT_TOPIC_MAX_LENGTH         48
#define MQTT_HASSIO_PREFIX_MAX_LENGTH 24
