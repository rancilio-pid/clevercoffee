/**
 * @file Storage.cpp
 *
 * @brief Provides functions to manage data in a non-volatile memory
 */

#include <Arduino.h>
#include <EEPROM.h>

#include "debugSerial.h"
#include "userConfig.h"
#include "Storage.h"

#define STRUCT_MEMBER_SIZE(Type, Member) sizeof(((Type*)0)->Member)

// storage data structure
typedef struct __attribute__((packed)) {
    // Any 'freeToUse' areas ensure the compatibility to origin EEPROM layout and can be used by new value.
    double pidKpRegular;
    uint8_t reserved1[2];
    double pidTnRegular;
    uint8_t pidOn;
    uint8_t freeToUse1;
    double pidTvRegular;
    double pidIMaxRegular;
    uint8_t freeToUse2;
    double brewSetpoint;
    double brewTempOffset;
    uint8_t freeToUse3;
    double brewTimeMs;
    uint8_t freeToUse4[2];
    double preInfusionTimeMs;
    uint8_t freeToUse5[2];
    double preInfusionPauseMs;
    uint8_t freeToUse6[21];
    uint8_t pidBdOn;
    double pidKpBd;
    uint8_t freeToUse7[2];
    double pidTnBd;
    uint8_t freeToUse8[2];
    double pidTvBd;
    uint8_t freeToUse9[2];
    double brewSwTimerSec;
    uint8_t freeToUse10[2];
    double brewDetectionThreshold;
    uint8_t freeToUse11;
    uint8_t useStartPonM;
    double pidKpStart;
    uint8_t freeToUse12[2];
    uint8_t softApEnabledCheck;
    uint8_t freeToUse13[9];
    double pidTnStart;
    uint8_t freeToUse14[2];
    char wifiSSID[25 + 1];
    char wifiPassword[25 + 1];
    double weightsetpoint;
    double steamkp;
} sto_data_t;


// item defaults
static const sto_data_t itemDefaults PROGMEM = {
    AGGKP,                    // STO_ITEM_PID_KP_REGULAR
    {0xFF, 0xFF},             // reserved (maybe for structure version)
    AGGTN,                    // STO_ITEM_PID_TN_REGULAR
    0,                        // STO_ITEM_PID_ON
    0xFF,                     // free to use
    AGGTV,                    // STO_ITEM_PID_TV_REGULAR
    AGGIMAX,                  // STO_ITEM_PID_I_MAX_REGULAR
    0xFF,                     // free to use
    SETPOINT,                 // STO_ITEM_BREW_SETPOINT
    TEMPOFFSET,             
    0xFF,                     // free to use
    BREW_TIME,                // STO_ITEM_BREW_TIME
    {0xFF, 0xFF},             // free to use
    PRE_INFUSION_TIME,        // STO_ITEM_PRE_INFUSION_TIME
    {0xFF, 0xFF},             // free to use
    PRE_INFUSION_PAUSE_TIME,  // STO_ITEM_PRE_INFUSION_PAUSE
    {   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF },       // free to use
    0,                                        // STO_ITEM_USE_PID_BD
    AGGBKP,                                   // STO_ITEM_PID_KP_BD
    {0xFF, 0xFF},                             // free to use
    AGGBTN,                                   // STO_ITEM_PID_TN_BD
    {0xFF, 0xFF},                             // free to use
    AGGBTV,                                   // STO_ITEM_PID_TV_BD
    {0xFF, 0xFF},                             // free to use
    BREW_SW_TIMER,                            // STO_ITEM_BREW_SW_TIMER
    {0xFF, 0xFF},                             // free to use
    BREWSENSITIVITY,                          // STO_ITEM_BD_THRESHOLD
    0xFF,                                     // free to use
    1,                                        // STO_ITEM_USE_START_PON_M
    STARTKP,                                  // STO_ITEM_PID_KP_START
    {0xFF, 0xFF},                             // free to use
    0,                                        // STO_ITEM_SOFT_AP_ENABLED_CHECK
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  // free to use
    STARTTN,       // STO_ITEM_PID_TN_START
    {0xFF, 0xFF},  // free to use
    "",            // STO_ITEM_WIFI_SSID
    "",            // STO_ITEM_WIFI_PASSWORD
    SCALE_WEIGHTSETPOINT,
    STEAMKP    
};

/**
 * @brief Returns the storage address of given item.
 *        Optionally the max. item storage size can be delivered.
 *
 * @param itemId      - storage item ID
 * @param maxItemSize - buffer for max. item storage size (optional, default=no buffer)
 *
 * @return >=0 - item storage address
 *          <0 - error
 */
static inline int32_t getItemAddr(sto_item_id_t itemId, uint16_t* maxItemSize = NULL) {
    int32_t addr;
    uint16_t size;

    switch (itemId) {
        case STO_ITEM_PID_KP_REGULAR:
            addr = offsetof(sto_data_t, pidKpRegular);
            size = STRUCT_MEMBER_SIZE(sto_data_t, pidKpRegular);
            break;

        case STO_ITEM_PID_TN_REGULAR:
            addr = offsetof(sto_data_t, pidTnRegular);
            size = STRUCT_MEMBER_SIZE(sto_data_t, pidTnRegular);
            break;

        case STO_ITEM_PID_TV_REGULAR:
            addr = offsetof(sto_data_t, pidTvRegular);
            size = STRUCT_MEMBER_SIZE(sto_data_t, pidTvRegular);
            break;

        case STO_ITEM_PID_I_MAX_REGULAR:
            addr = offsetof(sto_data_t, pidIMaxRegular);
            size = STRUCT_MEMBER_SIZE(sto_data_t, pidIMaxRegular);
            break;

        case STO_ITEM_BREW_SETPOINT:
            addr = offsetof(sto_data_t, brewSetpoint);
            size = STRUCT_MEMBER_SIZE(sto_data_t, brewSetpoint);
            break;

        case STO_ITEM_BREW_TEMP_OFFSET:
            addr = offsetof(sto_data_t, brewTempOffset);
            size = STRUCT_MEMBER_SIZE(sto_data_t, brewTempOffset);
            break;

        case STO_ITEM_BREW_TIME:
            addr = offsetof(sto_data_t, brewTimeMs);
            size = STRUCT_MEMBER_SIZE(sto_data_t, brewTimeMs);
            break;

        case STO_ITEM_PRE_INFUSION_TIME:
            addr = offsetof(sto_data_t, preInfusionTimeMs);
            size = STRUCT_MEMBER_SIZE(sto_data_t, preInfusionTimeMs);
            break;

        case STO_ITEM_PRE_INFUSION_PAUSE:
            addr = offsetof(sto_data_t, preInfusionPauseMs);
            size = STRUCT_MEMBER_SIZE(sto_data_t, preInfusionPauseMs);
            break;

        case STO_ITEM_USE_BD_PID:
            addr = offsetof(sto_data_t, pidBdOn);
            size = STRUCT_MEMBER_SIZE(sto_data_t, pidBdOn);
            break;

        case STO_ITEM_PID_KP_BD:
            addr = offsetof(sto_data_t, pidKpBd);
            size = STRUCT_MEMBER_SIZE(sto_data_t, pidKpBd);
            break;

        case STO_ITEM_PID_TN_BD:
            addr = offsetof(sto_data_t, pidTnBd);
            size = STRUCT_MEMBER_SIZE(sto_data_t, pidTnBd);
            break;

        case STO_ITEM_PID_TV_BD:
            addr = offsetof(sto_data_t, pidTvBd);
            size = STRUCT_MEMBER_SIZE(sto_data_t, pidTvBd);
            break;

        case STO_ITEM_BREW_SW_TIMER:
            addr = offsetof(sto_data_t, brewSwTimerSec);
            size = STRUCT_MEMBER_SIZE(sto_data_t, brewSwTimerSec);
            break;

        case STO_ITEM_BD_THRESHOLD:
            addr = offsetof(sto_data_t, brewDetectionThreshold);
            size = STRUCT_MEMBER_SIZE(sto_data_t, brewDetectionThreshold);
            break;

        case STO_ITEM_PID_START_PONM:
            addr = offsetof(sto_data_t, useStartPonM);
            size = STRUCT_MEMBER_SIZE(sto_data_t, useStartPonM);
            break;

        case STO_ITEM_PID_KP_START:
            addr = offsetof(sto_data_t, pidKpStart);
            size = STRUCT_MEMBER_SIZE(sto_data_t, pidKpStart);
            break;

        case STO_ITEM_PID_TN_START:
            addr = offsetof(sto_data_t, pidTnStart);
            size = STRUCT_MEMBER_SIZE(sto_data_t, pidTnStart);
            break;

        case STO_ITEM_SOFT_AP_ENABLED_CHECK:
            addr = offsetof(sto_data_t, softApEnabledCheck);
            size = STRUCT_MEMBER_SIZE(sto_data_t, softApEnabledCheck);
            break;

        case STO_ITEM_WIFI_SSID:
            addr = offsetof(sto_data_t, wifiSSID);
            size = STRUCT_MEMBER_SIZE(sto_data_t, wifiSSID);
            break;

        case STO_ITEM_WIFI_PASSWORD:
            addr = offsetof(sto_data_t, wifiPassword);
            size = STRUCT_MEMBER_SIZE(sto_data_t, wifiPassword);
            break;

        case STO_ITEM_PID_ON:
            addr = offsetof(sto_data_t, pidOn);
            size = STRUCT_MEMBER_SIZE(sto_data_t, pidOn);
            break;

         case STO_ITEM_PID_KP_STEAM:
            addr = offsetof(sto_data_t, steamkp);
            size = STRUCT_MEMBER_SIZE(sto_data_t, steamkp);
            break;    

         case STO_ITEM_WEIGHTSETPOINT:
            addr = offsetof(sto_data_t,weightsetpoint );
            size = STRUCT_MEMBER_SIZE(sto_data_t,weightsetpoint);
            break;

        default:
            debugPrintf("%s(): invalid item ID %i!\n", __FUNCTION__, itemId);
            addr = -1;
            size = 0;
            break;
    }

    if (maxItemSize) *maxItemSize = size;

    return addr;
}

/**
 * @brief Checks if an item storage area is considered as "empty", which means
 *        no value was stored up to now.
 *
 * @param storageBuf     - item storage
 * @param storageBufSize - item storage size
 * @param emptyPattern   - empty pattern (optional, default=Flash erase value)
 *
 * @return true  - item storage is "empty" (no value was stored)
 *         false - item storage is not "empty" (value was stored)
 */
static inline bool isEmpty(const void* storageBuf, uint16_t storageBufSize, uint8_t emptyPattern = 0xFF) {
    uint16_t idx;

    for (idx = 0; idx < storageBufSize; idx++) { // check all item value bytes...
        if (((const uint8_t*)storageBuf)[idx] != emptyPattern)  // other than "empty" value?
            break;           // yes -> abort loop (item storage not empty)
    }

    return (idx == storageBufSize);
}

/**
 * @brief Checks if a buffer contains a C string null terminator.
 *
 * @param buf     - buffer to check
 * @param bufSize - buffer size
 *
 * @return true  - buffer contains a C string (string may be empty)
 *         false - buffer does not contain a C string
 */
static inline bool isString(const void* buf, uint16_t bufSize) {
    uint16_t idx;

    for (idx = 0; idx < bufSize; idx++) {     // check all buffer bytes...
        if (((const uint8_t*)buf)[idx] == 0)  // null terminator?
        return true;                          // yes -> string exist
    }

    return false;
}

#if 0  // not needed a.t.m.
/**
 * @brief Sets the default values.
 */
static void setDefaults(void) {
  debugPrintf("%s(): %p <- %p (%u)\n", __FUNCTION__, EEPROM.getDataPtr(), &itemDefaults, sizeof(itemDefaults));
  memcpy_P(EEPROM.getDataPtr(), &itemDefaults, sizeof(itemDefaults));
}
#endif

/**
 * @brief Setups the module.
 *
 * @return  0 - succeed
 *         <0 - failed
 */
int storageSetup(void) {
    #if defined(ESP8266)
        EEPROM.begin(sizeof(sto_data_t));
    #elif defined(ESP32)
        if (!EEPROM.begin(sizeof(sto_data_t))) {
            debugPrintf("%s(): EEPROM initialization failed!\n", __FUNCTION__);
            return -1;
        }
    #else
        #error("not supported MCU");
    #endif

    /* It's not necessary here to check if any valid data are stored,
     * because storageGet() returns a default value in this case.
     */

    return 0;
}

/**
 * @brief Internal template function to read a number of any type.
 *
 * @param itemId    - storage item ID
 * @param itemValue - buffer for item value
 *
 * @return  0 - succeed
 *         <0 - failed
 */
template <typename T>
static inline int getNumber(sto_item_id_t itemId, T& itemValue) {
    uint16_t maxItemSize;
    int32_t itemAddr = getItemAddr(itemId, &maxItemSize);

    if (itemAddr < 0) {
        debugPrintf("%s(): invalid item address!\n", __FUNCTION__);
        return -1;
    }

    debugPrintf("%s(): addr=%i size=%u/%u\n", __FUNCTION__, itemAddr, sizeof(itemValue), maxItemSize);

    if (sizeof(itemValue) != maxItemSize) {
        debugPrintf("%s(): invalid item size (wrong data type)!\n", __FUNCTION__);
        return -2;
    }

    EEPROM.get(itemAddr, itemValue);

    if (isEmpty(&itemValue, sizeof(itemValue))) { // item storage empty?
        debugPrintf("%s(): storage empty -> returning default\n", __FUNCTION__);

        memcpy_P(&itemValue, (PGM_P)&itemDefaults + itemAddr, sizeof(itemValue));  // set default value
    }

    return 0;
}

/**
 * @brief Internal template function to set a number of any type.
 *        The value is set in the RAM only and still not written to the
 *        non-volatile memory!
 *
 * @param itemId    - storage item ID
 * @param itemValue - item value to set
 * @param commit    - true=write current data to medium (optional, default=false)
 *
 * @return  0 - succeed
 *         <0 - failed
 */
template <typename T>
static inline int setNumber(sto_item_id_t itemId, const T& itemValue, bool commit = false) {
    uint16_t maxItemSize;
    int32_t itemAddr = getItemAddr(itemId, &maxItemSize);

    if (itemAddr < 0) {
        debugPrintf("%s(): invalid item address!\n", __FUNCTION__);
        return -1;
    }

    debugPrintf("%s(): addr=%i size=%u/%u commit=%u\n", __FUNCTION__, itemAddr, sizeof(itemValue), maxItemSize, commit);

    if (sizeof(itemValue) != maxItemSize) {
        debugPrintf("%s(): invalid item size (wrong data type)!\n", __FUNCTION__);
        return -2;
    }

    if (isEmpty(&itemValue, sizeof(T))) {
        debugPrintf("%s(): invalid item value!\n", __FUNCTION__);
        return -3;
    }

    EEPROM.put(itemAddr, itemValue);

    if (commit) return storageCommit();

    return 0;
}

/**
 * @brief Reads the value of a storage item.
 *
 * @param itemId    - storage item ID
 * @param itemValue - buffer for item value
 *
 * @return  0 - succeed
 *         <0 - failed
 */
int storageGet(sto_item_id_t itemId, float& itemValue) {
    int retCode = getNumber(itemId, itemValue);

    if (retCode != 0) return retCode;

    debugPrintf("%s(): item=%i value=%.1f\n", __FUNCTION__, itemId, itemValue);

    return 0;
}

int storageGet(sto_item_id_t itemId, double& itemValue) {
    int retCode = getNumber(itemId, itemValue);

    if (retCode != 0) return retCode;

    debugPrintf("%s(): item=%i value=%.1f\n", __FUNCTION__, itemId, itemValue);

    return 0;
}

int storageGet(sto_item_id_t itemId, int8_t& itemValue) {
    int retCode = getNumber(itemId, itemValue);

    if (retCode != 0) return retCode;

    debugPrintf("%s(): item=%i value=%i\n", __FUNCTION__, itemId, itemValue);

    return 0;
}

int storageGet(sto_item_id_t itemId, int16_t& itemValue) {
    int retCode = getNumber(itemId, itemValue);

    if (retCode != 0) return retCode;

    debugPrintf("%s(): item=%i value=%i\n", __FUNCTION__, itemId, itemValue);

    return 0;
}

int storageGet(sto_item_id_t itemId, int32_t& itemValue) {
    int retCode = getNumber(itemId, itemValue);

    if (retCode != 0) return retCode;

    debugPrintf("%s(): item=%i value=%i\n", __FUNCTION__, itemId, itemValue);

    return 0;
}

int storageGet(sto_item_id_t itemId, uint8_t& itemValue) {
    int retCode = getNumber(itemId, itemValue);

    if (retCode != 0) return retCode;

    debugPrintf("%s(): item=%i value=%u\n", __FUNCTION__, itemId, itemValue);

    return 0;
}

int storageGet(sto_item_id_t itemId, uint16_t& itemValue) {
    int retCode = getNumber(itemId, itemValue);
    if (retCode != 0) return retCode;
    debugPrintf("%s(): item=%i value=%u\n", __FUNCTION__, itemId, itemValue);

    return 0;
}

int storageGet(sto_item_id_t itemId, uint32_t& itemValue) {
    int retCode = getNumber(itemId, itemValue);

    if (retCode != 0) return retCode;

    debugPrintf("%s(): item=%i value=%u\n", __FUNCTION__, itemId, itemValue);

    return 0;
}

#if 0
/* The ESP32 EEPROM class does not support getConstDataPtr(). The available
 * getDataPtr() always leads to a "dirty" status (= erase/program cycle), even
 * if only read operations will be done!
 * If this functions is needed, a temporary read buffer has to be used.
 */
int storageGet(sto_item_id_t itemId, const char** itemValue) {
    size_t itemSize;
    int32_t itemAddr = getItemAddr(itemId);

    if ((itemAddr < 0) || (itemValue == NULL))
        return -1;

    // instead of copying the string, give back pointer to string in the data structure
    *itemValue = (const char*)(EEPROM.getConstDataPtr() + itemAddr);
    itemSize = strlen(*itemValue) + 1;

    if (isEmpty(*itemValue, itemSize)) { // item storage empty?
        debugPrintf("%s(): storage empty -> returning default\n", __FUNCTION__);
        memcpy_P(&itemValue, (const void*)(&itemDefaults+itemAddr), itemSize);      // set default value
    }

    debugPrintf("%s(): addr=%i size=%u item=%i value=\"%s\"\n", __FUNCTION__, itemAddr, itemSize, itemId, *itemValue);

    return 0;
}
#endif

int storageGet(sto_item_id_t itemId, String& itemValue) {
    uint16_t maxItemSize;
    int32_t itemAddr = getItemAddr(itemId, &maxItemSize);

    if (itemAddr < 0) {
        debugPrintf("%s(): invalid item address!\n", __FUNCTION__);
        return -1;
    }

    #if defined(ESP8266)
        const uint8_t* storageDataPtr;
        storageDataPtr = EEPROM.getConstDataPtr() + itemAddr;

        if (isString(storageDataPtr, maxItemSize))  { // exist a null terminator?
            itemValue = String((const char*)storageDataPtr); // convert to C++ string
        } else {
            debugPrintf("%s(): storage empty -> returning default\n", __FUNCTION__);
            itemValue = String((PGM_P)&itemDefaults + itemAddr);  // set default string
        }
    #elif defined(ESP32)
        // The ESP32 EEPROM class does not support getConstDataPtr()!
        uint8_t buf[maxItemSize];
        EEPROM.readBytes(itemAddr, buf, maxItemSize);

        if (isString(buf, maxItemSize)) { // exist a null terminator?
            itemValue = String((const char*)buf);
        } else {
            debugPrintf("%s(): storage empty -> returning default\n", __FUNCTION__);
            itemValue = String((PGM_P)&itemDefaults + itemAddr);  // set default string
        }
    #else
        #error("MCU not supported");
    #endif

    debugPrintf("%s(): addr=%i size=%u item=%i value=\"%s\"\n", __FUNCTION__, itemAddr, itemValue.length() + 1, itemId, itemValue.c_str());

    return 0;
}

/**
 * @brief Sets a value of a storage item.
 *        The value is set in the RAM only! Use 'commit=true' or call
 *        storageCommit() to write the RAM content to the non-volatile memory!
*
*  @param itemId    - storage item ID
 * @param itemValue - item value to set
 * @param commit    - true=write current RAM content to NV memory (optional, default=false)
 *
 * @return  0 - succeed
 *         <0 - failed
 */
int storageSet(sto_item_id_t itemId, float itemValue, bool commit) {
    debugPrintf("%s(): item=%i value=%.1f\n", __FUNCTION__, itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, double itemValue, bool commit) {
    debugPrintf("%s(): item=%i value=%.1f\n", __FUNCTION__, itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, int8_t itemValue, bool commit) {
    debugPrintf("%s(): item=%i value=%i\n", __FUNCTION__, itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, int16_t itemValue, bool commit) {
    debugPrintf("%s(): item=%i value=%i\n", __FUNCTION__, itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, int32_t itemValue, bool commit) {
    debugPrintf("%s(): item=%i value=%i\n", __FUNCTION__, itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, uint8_t itemValue, bool commit) {
    debugPrintf("%s(): item=%i value=%u\n", __FUNCTION__, itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, uint16_t itemValue, bool commit) {
    debugPrintf("%s(): item=%i value=%u\n", __FUNCTION__, itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, uint32_t itemValue, bool commit) {
    debugPrintf("%s(): item=%i value=%u\n", __FUNCTION__, itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, const char* itemValue, bool commit) {
    uint16_t maxItemSize;
    size_t valueSize;
    int32_t itemAddr = getItemAddr(itemId, &maxItemSize);

    if (itemAddr < 0) {
        debugPrintf("%s(): invalid item address!\n", __FUNCTION__);
        return -1;
    }

    valueSize = strlen(itemValue) + 1;
    debugPrintf("%s(): item=%i value=\"%s\" addr=%i size=%u/%u\n", __FUNCTION__, itemId, itemValue, itemAddr, valueSize, maxItemSize);

    if (valueSize > maxItemSize) { // invalid value size?
        debugPrintf("%s(): string too large!\n", __FUNCTION__);
        return -2;
    }

    memcpy(EEPROM.getDataPtr() + itemAddr, itemValue, valueSize);  // copy value to data structure in RAM

    if (commit) return storageCommit();

    return 0;
}

int storageSet(sto_item_id_t itemId, String& itemValue, bool commit) {
    return storageSet(itemId, itemValue.c_str(), commit);
}

/**
 * @brief Writes all items to the storage medium.
 *
 * @return  0 - succeed
 *         <0 - failed
 */
int storageCommit(void) {
    int returnCode;
    // bool isTimerEnabled;

    debugPrintf("%s(): save all data to NV memory\n", __FUNCTION__);

    /* While Flash memory erase/write operations no other code must be executed from
     * Flash! Since all code of the Timer1-ISR is placed in RAM, the interrupt does
     * not need to be disabled.
     */
    #if 0
        // disable any ISRs...
        isTimerEnabled = isTimer1Enabled();
        disableTimer1();
    #endif

    // really write data to storage...
    returnCode = EEPROM.commit() ? 0 : -1;

    #if 0
    // recover any ISRs...
    if (isTimerEnabled)   // was timer enabled before?
        enableTimer1();   // yes -> re-enable timer
    #endif

    return returnCode;
}

/**
 * @brief Resets all storage data.
 *
 * @return  0 - succeed
 *         <0 - failed
 */
int storageFactoryReset(void) {
    debugPrintf("%s(): reset all values\n", __FUNCTION__);
    memset(EEPROM.getDataPtr(), 0xFF, sizeof(sto_data_t));

    return storageCommit();
}
