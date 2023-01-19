#ifndef _RANCILIO_PID_H_
#define _RANCILIO_PID_H_

#include <stdint.h>
#include "SysPara.h"

// Functions
int factoryReset(void);
int readSysParamsFromStorage(void);
int writeSysParamsToStorage(void);

// system parameter defaults and ranges
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// default parameters
#define SETPOINT 95                // brew temperature setpoint
#define TEMPOFFSET 0               // brew temperature setpoint
#define STEAMSETPOINT 120          // steam temperature setpoint
#define BREWSENSITIVITY 120        // brew detection sensitivity, be careful: if too low, then there is the risk of wrong brew detection and rising temperature
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
#define PRE_INFUSION_TIME 2        // pre-infusion time in seconds
#define PRE_INFUSION_PAUSE_TIME 5  // pre-infusion pause time in seconds
#define SCALE_WEIGHTSETPOINT 30    // Target weight in grams
#define SCALE_CALIBRATION -250.00  // Calibration value
#define SCALE_KNOWNWEIGHT 200      // Known weight in grams for calibrating the scale
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
#define SCALECALIBRATION_MIN -100000
#define SCALECALIBRATION_MAX 100000
#define SCALEKNOWNWEIGHT_MIN 0
#define SCALEKNOWNWEIGHT_MAX 1500
#endif
