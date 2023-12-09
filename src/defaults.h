/**
 * @file defaults.h
 *
 * @brief Default values for system parameters
 *
 */

#pragma once

#include <stdint.h>
#include "SysPara.h"

// Functions
int factoryReset(void);
int readSysParamsFromStorage(void);
int writeSysParamsToStorage(void);

// System parameter defaults and ranges
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// Default parameters
#define SETPOINT 95                // brew temperature setpoint
#define TEMPOFFSET 0               // brew temperature setpoint
#define STEAMSETPOINT 120          // steam temperature setpoint
#define AGGKP 62                   // PID Kp (regular phase)
#define AGGTN 52                   // PID Tn (regular phase)
#define AGGTV 11.5                 // PID Tv (regular phase)
#define AGGIMAX 55                 // PID Integrator Max (regular phase)
#define STARTKP 45                 // PID Kp (coldstart phase)
#define STARTTN 130                // PID Tn (coldstart phase)
#define STEAMKP 150                // PID kp (steam phase)
#define AGGBKP 50                  // PID Kp (brew detection phase)
#define AGGBTN 0                   // PID Tn (brew detection phase)
#define AGGBTV 20                  // PID Tv (brew detection phase)
#define BREW_TIME 25               // brew time in seconds (only used if pump is being controlled)
#define BREW_SW_TIME 25            // keep brew PID params for this many seconds after detection (only for software BD)
#define BREW_PID_DELAY 10          // delay until enabling PID controller during brew (no heating during this time)
#define BD_SENSITIVITY 120         // brew detection sensitivity, be careful: if too low, then there is the risk of wrong brew detection and rising temperature
#define PRE_INFUSION_TIME 2        // pre-infusion time in seconds
#define PRE_INFUSION_PAUSE_TIME 5  // pre-infusion pause time in seconds
#define SCALE_WEIGHTSETPOINT 30    // Target weight in grams
#define WIFI_CREDENTIALS_SAVED 0   // Flag if wifi setup is done. 0: not set up, 1: credentials set up via wifi manager
#define STANDBY_MODE_ON 0          // Standby mode off by default
#define STANDBY_MODE_TIME 30       // Time in minutes until the heater is turned off   

#define PID_KP_START_MIN 0
#define PID_KP_START_MAX 350
#define PID_TN_START_MIN 0
#define PID_TN_START_MAX 999
#define PID_KP_REGULAR_MIN 0
#define PID_KP_REGULAR_MAX 200
#define PID_TN_REGULAR_MIN 0
#define PID_TN_REGULAR_MAX 999
#define PID_TV_REGULAR_MIN 0
#define PID_TV_REGULAR_MAX 999
#define PID_I_MAX_REGULAR_MIN 0
#define PID_I_MAX_REGULAR_MAX 999
#define PID_KP_BD_MIN 0
#define PID_KP_BD_MAX 200
#define PID_TN_BD_MIN 0
#define PID_TN_BD_MAX 999
#define PID_TV_BD_MIN 0
#define PID_TV_BD_MAX 999
#define BREW_SETPOINT_MIN 20
#define BREW_SETPOINT_MAX 110
#define STEAM_SETPOINT_MIN 100
#define STEAM_SETPOINT_MAX 140
#define BREW_TEMP_OFFSET_MIN 0
#define BREW_TEMP_OFFSET_MAX 20
#define BREW_TEMP_TIME_MIN 1
#define BREW_TEMP_TIME_MAX 180
#define BREW_TIME_MIN 1
#define BREW_TIME_MAX 180
#define BREW_PID_DELAY_MIN 0
#define BREW_PID_DELAY_MAX 60
#define BREW_SW_TIME_MIN 1
#define BREW_SW_TIME_MAX 180
#define BD_THRESHOLD_MIN 0
#define BD_THRESHOLD_MAX 999
#define PRE_INFUSION_TIME_MIN 0
#define PRE_INFUSION_TIME_MAX 60
#define PRE_INFUSION_PAUSE_MIN 0
#define PRE_INFUSION_PAUSE_MAX 60
#define WEIGHTSETPOINT_MIN 0
#define WEIGHTSETPOINT_MAX 500
#define PID_KP_STEAM_MIN 0
#define PID_KP_STEAM_MAX 500
#define STANDBY_MODE_TIME_MIN 30
#define STANDBY_MODE_TIME_MAX 120

#if ROTARY_MENU == 1
    #define ENCODER_CLICKS_PER_NOTCH 4

    // Menu position and size
    #define _LCDML_DISP_box_x0 0
    #define _LCDML_DISP_box_y0 0

    static const int _LCDML_DISP_box_x1 = SCREEN_WIDTH;
    static const int _LCDML_DISP_box_y1 = SCREEN_HEIGHT;

    #define _LCDML_DISP_scrollbar_w 6
    #define _LCDML_DISP_cols_max ((_LCDML_DISP_box_x1-_LCDML_DISP_box_x0)/_LCDML_DISP_font_w)
    #define _LCDML_DISP_rows_max ((_LCDML_DISP_box_y1-_LCDML_DISP_box_y0-((_LCDML_DISP_box_y1-_LCDML_DISP_box_y0)/_LCDML_DISP_font_h))/_LCDML_DISP_font_h)
    #define _LCDML_DISP_rows _LCDML_DISP_rows_max
    #define _LCDML_DISP_cols 20
    #define _LCDML_DISP_draw_frame 0

    // Settings for u8g lib and LCD
    static const int _LCDML_DISP_w = SCREEN_WIDTH; 
    static const int _LCDML_DISP_h = SCREEN_HEIGHT;

    // Font settings
    #define _LCDML_DISP_font u8g_font_6x13
    #define _LCDML_DISP_font_w 4
    #define _LCDML_DISP_font_h 13

    // Cursor settings
    #define _LCDML_DISP_cursor_char "X"
    #define _LCDML_DISP_cur_space_before 2
    #define _LCDML_DISP_cur_space_behind 4
#endif
