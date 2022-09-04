#ifndef _RANCILIO_PID_H_
#define _RANCILIO_PID_H_

#include <stdint.h>

// default parameters
#define SETPOINT 95                // brew temperature setpoint
#define TEMPOFFSET 0               // brew temperature setpoint
#define STEAMSETPOINT 120          // steam temperature setpoint
#define BREWSENSITIVITY 120        // brew detection sensitivity, be careful: if too low, then there is the risk of wrong brew detection and rising temperature
#define AGGKP 65                   // PID Kp (regular phase)
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
#define BREW_SW_TIMER 25           // keep brew PID params for this many seconds after detection (only for software BD)
#define PRE_INFUSION_TIME 2        // pre-infusion time in seconds
#define PRE_INFUSION_PAUSE_TIME 5  // pre-infusion pause time in seconds
#define SCALE_WEIGHTSETPOINT 30    // Target weight in grams

// Functions
int factoryReset(void);
const String getFwVersion(void);
int readSysParamsFromStorage(void);
int writeSysParamsToStorage(void);

#endif
