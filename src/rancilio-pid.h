#ifndef _RANCILIO_PID_H_
#define _RANCILIO_PID_H_

#include <stdint.h>

// Functions
int factoryReset(void);
const char* getFwVersion(void);
int readSysParamsFromStorage(void);
int writeSysParamsToStorage(void);

#endif
