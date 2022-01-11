#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <stdint.h>



/******************************************************************************
 * DEFINES
 ******************************************************************************/




/******************************************************************************
 * TYPEDEFS
 ******************************************************************************/

// storage items
typedef enum
{
  STO_ITEM_PID_KP_REGULAR,                                                      // PID P part at regular operation
  STO_ITEM_PID_TN_REGULAR,                                                      // PID I part at regular operation
  STO_ITEM_PID_TV_REGULAR,                                                      // PID D part at regular operation
  STO_ITEM_BREW_SETPOINT,                                                       // brew setpoint
  STO_ITEM_BREW_TIME,                                                           // brew time
  STO_ITEM_PRE_INFUSION_TIME,                                                   // pre-infusion time
  STO_ITEM_PRE_INFUSION_PAUSE,                                                  // pre-infusion pause
  STO_ITEM_RESERVED_70,                                                         // reserved
  STO_ITEM_RESERVED_80,                                                         // reserved
  STO_ITEM_PID_KP_BD,                                                           // PID P part at brew detection phase
  STO_ITEM_PID_TN_BD,                                                           // PID I part at brew detection phase
  STO_ITEM_PID_TV_BD,                                                           // PID D part at brew detection phase
  STO_ITEM_BREW_SW_TIMER,                                                       // brew software timer
  STO_ITEM_BD_THRESHOLD,                                                        // brew detection limit
  STO_ITEM_PID_KP_START,                                                        // PID P part at cold start phase
  STO_ITEM_SOFT_AP_ENABLED_CHECK,                                               // soft AP enable state
  STO_ITEM_WIFI_SSID,                                                           // Wifi SSID
  STO_ITEM_WIFI_PASSWORD,                                                       // Wifi password

  STO_ITEM__LAST_ENUM                                                           // must be the last one!
}sto_item_id_t;




/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

int storageGet(sto_item_id_t itemId, double& itemValue);
int storageGet(sto_item_id_t itemId, int& itemValue);
int storageGet(sto_item_id_t itemId, const char** itemValue);
int storageGet(sto_item_id_t itemId, String& itemValue);
int storageSet(sto_item_id_t itemId, double itemValue, bool commit=false);
int storageSet(sto_item_id_t itemId, int itemValue, bool commit=false);
int storageSet(sto_item_id_t itemId, const char* itemValue, bool commit=false);
int storageSet(sto_item_id_t itemId, String& itemValue, bool commit=false);
int storageSetup(void);
int storageCommit(void);


#endif // _STORAGE_H_
