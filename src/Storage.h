/**
 * @file Storage.h
 *
 * @brief
 */

#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <stdint.h>


// storage items
typedef enum
{
  STO_ITEM_PID_KP_REGULAR,          // PID P part at regular operation
  STO_ITEM_PID_TN_REGULAR,          // PID I part at regular operation
  STO_ITEM_PID_TV_REGULAR,          // PID D part at regular operation
  STO_ITEM_BREW_SETPOINT,           // brew setpoint
  STO_ITEM_BREW_TIME,               // brew time
  STO_ITEM_PRE_INFUSION_TIME,       // pre-infusion time
  STO_ITEM_PRE_INFUSION_PAUSE,      // pre-infusion pause
  STO_ITEM_RESERVED_70,             // reserved
  STO_ITEM_RESERVED_80,             // reserved
  STO_ITEM_PID_KP_BD,               // PID P part at brew detection phase
  STO_ITEM_PID_TN_BD,               // PID I part at brew detection phase
  STO_ITEM_PID_TV_BD,               // PID D part at brew detection phase
  STO_ITEM_BREW_SW_TIME,            // brew software time
  STO_ITEM_BD_THRESHOLD,            // brew detection limit
  STO_ITEM_PID_KP_START,            // PID P part at cold start phase
  STO_ITEM_PID_TN_START,            // PID I part at cold start phase
  STO_ITEM_SOFT_AP_ENABLED_CHECK,   // soft AP enable state
  STO_ITEM_WIFI_SSID,               // Wifi SSID
  STO_ITEM_WIFI_PASSWORD,           // Wifi password
  STO_ITEM_PID_ON,                  // PID on/off state
  STO_ITEM_WEIGHTSETPOINT,          // Brewweight setpoint
  STO_ITEM_PID_KP_STEAM,            // PID P part at steam phase
  STO_ITEM_PID_I_MAX_REGULAR,       // PID Integrator upper limit
  STO_ITEM_BREW_TEMP_OFFSET,        // brew temp offset
  STO_ITEM_PID_START_PONM,          // Use PonM for cold start phase (otherwise use normal PID and same params)
  STO_ITEM_USE_BD_PID,              // use separate PID for brew detection (otherwise continue with regular PID)
  STO_ITEM_STEAM_SETPOINT,          // Setpoint for Steam mode
  STO_ITEM_SCALECALIBRATION,        // Calibration value for scale
  STO_ITEM_SCALEKNOWNWEIGHT,        // Known weight for scale calibration

  /* WHEN ADDING NEW ITEMS, THE FOLLOWING HAS TO BE UPDATED:
   * - storage structure:  sto_data_t
   * - item default value: itemDefaults
   * - item address/size:  getItemAddr()
   */

  STO_ITEM__LAST_ENUM       // must be the last one!
} sto_item_id_t;


// Functions
int storageSetup(void);
int storageCommit(void);
int storageFactoryReset(void);

int storageGet(sto_item_id_t itemId, float& itemValue);
int storageGet(sto_item_id_t itemId, double& itemValue);
int storageGet(sto_item_id_t itemId, int8_t& itemValue);
int storageGet(sto_item_id_t itemId, int16_t& itemValue);
int storageGet(sto_item_id_t itemId, int32_t& itemValue);
int storageGet(sto_item_id_t itemId, uint8_t& itemValue);
int storageGet(sto_item_id_t itemId, uint16_t& itemValue);
int storageGet(sto_item_id_t itemId, uint32_t& itemValue);
int storageGet(sto_item_id_t itemId, const char** itemValue);
int storageGet(sto_item_id_t itemId, String& itemValue);

int storageSet(sto_item_id_t itemId, float itemValue, bool commit=false);
int storageSet(sto_item_id_t itemId, double itemValue, bool commit=false);
int storageSet(sto_item_id_t itemId, int8_t itemValue, bool commit=false);
int storageSet(sto_item_id_t itemId, int16_t itemValue, bool commit=false);
int storageSet(sto_item_id_t itemId, int32_t itemValue, bool commit=false);
int storageSet(sto_item_id_t itemId, uint8_t itemValue, bool commit=false);
int storageSet(sto_item_id_t itemId, uint16_t itemValue, bool commit=false);
int storageSet(sto_item_id_t itemId, uint32_t itemValue, bool commit=false);
int storageSet(sto_item_id_t itemId, const char* itemValue, bool commit=false);
int storageSet(sto_item_id_t itemId, String& itemValue, bool commit=false);


#endif
