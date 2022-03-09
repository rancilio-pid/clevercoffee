#ifndef _RANCILIO_PID_H_
#define _RANCILIO_PID_H_

#include <stdint.h>
#include "SysPara.h"


// system parameters
extern SysPara<double> sysParaPidKpStart;
extern SysPara<double> sysParaPidTnStart;
extern SysPara<double> sysParaPidKpReg;
extern SysPara<double> sysParaPidTnReg;
extern SysPara<double> sysParaPidTvReg;
extern SysPara<double> sysParaPidKpBd;
extern SysPara<double> sysParaPidTnBd;
extern SysPara<double> sysParaPidTvBd;
extern SysPara<double> sysParaBrewSetPoint;
extern SysPara<double> sysParaBrewTime;
extern SysPara<double> sysParaBrewSwTimer;
extern SysPara<double> sysParaBrewThresh;
extern SysPara<double> sysParaPreInfTime;
extern SysPara<double> sysParaPreInfPause;
extern SysPara<double> sysParaWeightSetPoint;
extern SysPara<double> sysParaPidKpSteam;
extern SysPara<uint8_t> sysParaPidOn;


// Functions
int factoryReset(void);
const char* getFwVersion(void);
int readSysParamsFromStorage(void);
int writeSysParamsToStorage(void);

#endif
