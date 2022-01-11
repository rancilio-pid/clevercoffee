/******************************************************************************
 * \brief Provides functions to manage data in a non-volatile memory.
 ******************************************************************************/

#include <Arduino.h>
#include "DebugStreamManager.h"
#include <EEPROM.h>
#include "userConfig.h"
#include "Storage.h"



/******************************************************************************
 * DEFINES
 ******************************************************************************/



/******************************************************************************
 * TYPEDEFS
 ******************************************************************************/

// storage data structure
typedef struct __attribute__((packed))
{
// any fill bytes ensure the compatibility to current EEPROM usage!
// All fill bytes will be removed as soon as a structure version is implemented.
  double pidKpRegular;           uint8_t fill1[2];
  double pidTnRegular;           uint8_t fill2[2];
  double pidTvRegular;           uint8_t fill3[2];
  double brewSetpoint;           uint8_t fill4[2];
  double brewTimeMs;             uint8_t fill5[2];
  double preInfusionTimeMs;      uint8_t fill6[2];
  double preInfusionPauseMs;     uint8_t fill7[2];
  double reserved1;              uint8_t fill8[2];
  double reserved2;              uint8_t fill9[2];
  double pidKpBd;                uint8_t fill10[2];
  double pidTnBd;                uint8_t fill12[2];
  double pidTvBd;                uint8_t fill13[2];
  double brewSwTimerSec;         uint8_t fill14[2];
  double brewDetectionThreshold; uint8_t fill15[2];
  double pidKpStart;             uint8_t fill16[2];
  int softApEnabledCheck;
  char wifiSSID[25+1];
  char wifiPassword[25+1];
}sto_data_t;



/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

// item defaults
static const sto_data_t itemDefaults PROGMEM =
{
  AGGKP,              {0xFF, 0xFF},                                             // STO_ITEM_PID_KP_REGULAR
  AGGTN,              {0xFF, 0xFF},                                             // STO_ITEM_PID_TN_REGULAR
  AGGTV,              {0xFF, 0xFF},                                             // STO_ITEM_PID_TV_REGULAR
  SETPOINT,           {0xFF, 0xFF},                                             // STO_ITEM_BREW_SETPOINT
  25000,              {0xFF, 0xFF},                                             // STO_ITEM_BREW_TIME
  2000,               {0xFF, 0xFF},                                             // STO_ITEM_PRE_INFUSION_TIME
  5000,               {0xFF, 0xFF},                                             // STO_ITEM_PRE_INFUSION_PAUSE
  0,                  {0xFF, 0xFF},                                             // reserved
  0,                  {0xFF, 0xFF},                                             // reserved
  AGGBKP,             {0xFF, 0xFF},                                             // STO_ITEM_PID_KP_BD
  AGGBTN,             {0xFF, 0xFF},                                             // STO_ITEM_PID_TN_BD
  AGGBTV,             {0xFF, 0xFF},                                             // STO_ITEM_PID_TV_BD
  45,                 {0xFF, 0xFF},                                             // STO_ITEM_BREW_SW_TIMER
  BREWDETECTIONLIMIT, {0xFF, 0xFF},                                             // STO_ITEM_BD_THRESHOLD
  STARTKP,            {0xFF, 0xFF},                                             // STO_ITEM_PID_KP_START
  0,                                                                            // STO_ITEM_SOFT_AP_ENABLED_CHECK
  "",                                                                           // STO_ITEM_WIFI_SSID
  "",                                                                           // STO_ITEM_WIFI_PASSWORD
};



/******************************************************************************
 * VARIABLES
 ******************************************************************************/

extern DebugStreamManager debugStream;



/**************************************************************************//**
 * \brief Returns the storage address of given item ID.
 *
 * \param itemId - storage item ID
 *
 * \return >=0 - item storage address
 *          <0 - error
 ******************************************************************************/
static inline int32_t getItemAddr(sto_item_id_t itemId)
{
  int32_t addr;

  switch (itemId)
  {
    case STO_ITEM_PID_KP_REGULAR:
      addr = offsetof(sto_data_t, pidKpRegular);
      break;
    case STO_ITEM_PID_TN_REGULAR:
      addr = offsetof(sto_data_t, pidTnRegular);
      break;
    case STO_ITEM_PID_TV_REGULAR:
      addr = offsetof(sto_data_t, pidTvRegular);
      break;
    case STO_ITEM_BREW_SETPOINT:
      addr = offsetof(sto_data_t, brewSetpoint);
      break;
    case STO_ITEM_BREW_TIME:
      addr = offsetof(sto_data_t, brewTimeMs);
      break;
    case STO_ITEM_PRE_INFUSION_TIME:
      addr = offsetof(sto_data_t, preInfusionTimeMs);
      break;
    case STO_ITEM_PRE_INFUSION_PAUSE:
      addr = offsetof(sto_data_t, preInfusionPauseMs);
      break;
    case STO_ITEM_PID_KP_BD:
      addr = offsetof(sto_data_t, pidKpBd);
      break;
    case STO_ITEM_PID_TN_BD:
      addr = offsetof(sto_data_t, pidTnBd);
      break;
    case STO_ITEM_PID_TV_BD:
      addr = offsetof(sto_data_t, pidTvBd);
      break;
    case STO_ITEM_BREW_SW_TIMER:
      addr = offsetof(sto_data_t, brewSwTimerSec);
      break;
    case STO_ITEM_BD_THRESHOLD:
      addr = offsetof(sto_data_t, brewDetectionThreshold);
      break;
    case STO_ITEM_PID_KP_START:
      addr = offsetof(sto_data_t, pidKpStart);
      break;
    case STO_ITEM_SOFT_AP_ENABLED_CHECK:
      addr = offsetof(sto_data_t, softApEnabledCheck);
      break;
    case STO_ITEM_WIFI_SSID:
      addr = offsetof(sto_data_t, wifiSSID);
      break;
    case STO_ITEM_WIFI_PASSWORD:
      addr = offsetof(sto_data_t, wifiPassword);
      break;
    default:
      debugStream.writeW("%s(): invalid item ID %i!", __FUNCTION__, itemId);
      addr = -1;
      break;
  }

  return addr;
}



/**************************************************************************//**
 * \brief Sets the default values.
 ******************************************************************************/
static void setDefaults(void)
{
  memcpy(EEPROM.getDataPtr(), &itemDefaults, sizeof(itemDefaults));
}



/**************************************************************************//**
 * \brief Setups the module.
 *
 * \return  0 - succeed
 *         <0 - failed
 ******************************************************************************/
int storageSetup(void)
{
  int addr;

  #if defined(ESP8266)
  EEPROM.begin(sizeof(sto_data_t));
  #elif defined(ESP32)
  if (!EEPROM.begin(sizeof(sto_data_t)))
  {
    debugStream.writeE("%s(): EEPROM initialization failed!", __FUNCTION__);
    return -1;
  }
  #else
  #error("not supported MCU");
  #endif

  // check if any data are programmed...
  // An erased (or never programmed) Flash memory is filled with 0xFF.
  for (addr=0; addr<10; addr++)                                                 // check 1st 10 bytes...
  {
    if (EEPROM.read(addr) != 0xFF)                                              // programmed byte?
      break;                                                                    // yes -> abort loop
  }
  if (addr >= 10)                                                               // all bytes "empty"?
  {                                                                             // yes, write defaults...
    debugStream.writeD("%s(): no data found -> write defaults\n", __FUNCTION__);
    setDefaults();
    if (storageCommit() != 0)
      return -2;
  }

  #if 0
  // check first value, if there is a valid number...
  double dummy;
  EEPROM.get(0, dummy);
  if (isnan(dummy))                                                             // invalid floating point number?
  {                                                                             // yes...
    debugStream.writeD("%s(): no NV data found (addr 0=%f)", __FUNCTION__, dummy);
    return -2;
  }
  #endif

  return 0;
}



/**************************************************************************//**
 * \brief Internal template function to read a number of any type.
 *
 * \param itemId    - storage item ID
 * \param itemValue - buffer for item value
 *
 * \return  0 - succeed
 *         <0 - failed
 ******************************************************************************/
template<typename T>
static inline int getNumber(sto_item_id_t itemId, T &itemValue)
{
  int itemAddr;

  // sanity check...
  if (itemId >= STO_ITEM__LAST_ENUM)
  {
    debugStream.writeE("%s(): invalid item ID %i!", __FUNCTION__, itemId);
    return -1;
  }

  itemAddr = getItemAddr(itemId);
  if (itemAddr < 0)
    return -2;

  debugStream.writeD("%s(): addr=%i size=%u", __FUNCTION__, itemAddr, sizeof(itemValue));
  EEPROM.get(itemAddr, itemValue);

  return 0;
}



/**************************************************************************//**
 * \brief Internal template function to set a number of any type.
 *        The value is set in the RAM only and still not written to the
 *        non-volatile memory!
 *
 * \param itemId    - storage item ID
 * \param itemValue - item value to set
 * \param commit    - true=write current data to medium (optional, default=false)
 *
 * \return  0 - succeed
 *         <0 - failed
 ******************************************************************************/
template<typename T>
static inline int setNumber(sto_item_id_t itemId, const T &itemValue, bool commit=false)
{
  int itemAddr;

  // sanity check...
  if (itemId >= STO_ITEM__LAST_ENUM)
  {
    debugStream.writeE("%s(): invalid item ID %i!", __FUNCTION__, itemId);
    return -1;
  }

  itemAddr = getItemAddr(itemId);
  if (itemAddr < 0)
    return -2;

  debugStream.writeD("%s(): addr=%i size=%u commit=%u", __FUNCTION__, itemAddr,
                     sizeof(itemValue), commit);
  EEPROM.put(itemAddr, itemValue);

  if (commit)
    return storageCommit();

  return 0;
}



/**************************************************************************//**
 * \brief Reads the value of given storage item ID.
 *
 * \param itemId    - storage item ID
 * \param itemValue - buffer for item value
 *
 * \return  0 - succeed
 *         <0 - failed
 ******************************************************************************/
int storageGet(sto_item_id_t itemId, double& itemValue)
{
  int retCode = getNumber(itemId, itemValue);
  if (retCode != 0)
    return retCode;
  debugStream.writeD("%s(): item=%i value=%.1f", __FUNCTION__, itemId, itemValue);
  return 0;
}
int storageGet(sto_item_id_t itemId, int& itemValue)
{
  int retCode = getNumber(itemId, itemValue);
  if (retCode != 0)
    return retCode;
  debugStream.writeD("%s(): item=%i value=%i", __FUNCTION__, itemId, itemValue);
  return 0;
}
int storageGet(sto_item_id_t itemId, const char** itemValue)
{
  int itemAddr = getItemAddr(itemId);
  if (itemAddr < 0)
    return -1;
  // instead of copying the string, give back pointer to string in the data structure
  *itemValue = (const char*)(EEPROM.getDataPtr() + itemAddr);
  debugStream.writeD("%s(): addr=%i size=%u item=%i value=\"%s\"", __FUNCTION__,
                     itemAddr, strlen(*itemValue)+1, itemId, *itemValue);
  return 0;
}
int storageGet(sto_item_id_t itemId, String& itemValue)
{
  int itemAddr = getItemAddr(itemId);
  if (itemAddr < 0)
    return -1;
  itemValue = String((const char *)(EEPROM.getDataPtr() + itemAddr));
  debugStream.writeD("%s(): addr=%i size=%u item=%i value=\"%s\"", __FUNCTION__,
                     itemAddr, itemValue.length()+1, itemId, itemValue.c_str());
  return 0;
}



/**************************************************************************//**
 * \brief Sets a value of given storage item ID.
 *        The value is set in the RAM only! Use 'commit=true' or call
 *        storageCommit() to write the RAM content to the non-volatile memory!
 *
 * \param itemId    - storage item ID
 * \param itemValue - item value to set
 * \param commit    - true=write current RAM content to NV memory (optional, default=false)
 *
 * \return  0 - succeed
 *         <0 - failed
 ******************************************************************************/
int storageSet(sto_item_id_t itemId, double itemValue, bool commit)
{
  debugStream.writeD("%s(): item=%i value=%.1f", __FUNCTION__, itemId, itemValue);
  return setNumber(itemId, itemValue, commit);
}
int storageSet(sto_item_id_t itemId, int itemValue, bool commit)
{
  debugStream.writeD("%s(): item=%i value=%i", __FUNCTION__, itemId, itemValue);
  return setNumber(itemId, itemValue, commit);
}
int storageSet(sto_item_id_t itemId, const char* itemValue, bool commit)
{
  size_t valueSize, maxItemSize;
  int32_t nextItemAddr;
  int32_t itemAddr = getItemAddr(itemId);
  if (itemAddr < 0)
    return -1;
  valueSize = strlen(itemValue) + 1;
  // determine max. item size...
  nextItemAddr = getItemAddr((sto_item_id_t)(itemId+1));
  if (nextItemAddr <= 0)                                                        // last item?
    nextItemAddr = sizeof(sto_data_t);                                          // yes ->
  maxItemSize = nextItemAddr - itemAddr;
  debugStream.writeD("%s(): item=%i value=\"%s\" addr=%i size=%u/%u", __FUNCTION__,
                     itemId, itemValue, itemAddr, valueSize, maxItemSize);
  if (valueSize > maxItemSize)                                                  // invalid value size?
  {                                                                             // yes...
    debugStream.writeW("%s(): string too large!", __FUNCTION__);
    return -2;
  }
  memcpy(EEPROM.getDataPtr() + itemAddr, itemValue, valueSize);                 // copy value to data structure in RAM
  if (commit)
    return storageCommit();
  return 0;
}
int storageSet(sto_item_id_t itemId, String& itemValue, bool commit)
{
  return storageSet(itemId, itemValue.c_str(), commit);
}



/**************************************************************************//**
 * \brief Writes all items to storage medium.
 *
 * \return  0 - succeed
 *         <0 - failed
 ******************************************************************************/
int storageCommit(void)
{
  int returnCode;
  // bool isTimerEnabled;

  debugStream.writeD("%s(): save all data to NV memory", __FUNCTION__);

  // While Flash memory erase/write operations no other code must be executed from Flash!
  // Since all code of the Timer1-ISR is placed in RAM, the interrupt does not need to be disabled.
  #if 0
  // disable any ISRs...
  isTimerEnabled = isTimer1Enabled();
  disableTimer1();
  #endif

  // really write data to storage...
  returnCode = EEPROM.commit()? 0: -1;

  #if 0
  // recover any ISRs...
  if (isTimerEnabled)                                                           // was timer enabled before?
    enableTimer1();                                                             // yes -> re-enable timer
  #endif

  return returnCode;
}

