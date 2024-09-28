/**
 * @file storage.h
 *
 * @brief Provides functions to manage data in non-volatile memory
 *
 */

#pragma once

// storage items
typedef enum {
    STO_ITEM_PID_ON,                    // PID on/off state
    STO_ITEM_PID_START_PONM,            // Use PonM for cold start phase (otherwise use normal PID and same params)
    STO_ITEM_PID_KP_START,              // PID P part at cold start phase
    STO_ITEM_PID_TN_START,              // PID I part at cold start phase
    STO_ITEM_PID_KP_REGULAR,            // PID P part at regular operation
    STO_ITEM_PID_TN_REGULAR,            // PID I part at regular operation
    STO_ITEM_PID_TV_REGULAR,            // PID D part at regular operation
    STO_ITEM_PID_I_MAX_REGULAR,         // PID Integrator upper limit
    STO_ITEM_PID_KP_BD,                 // PID P part at brew detection phase
    STO_ITEM_PID_TN_BD,                 // PID I part at brew detection phase
    STO_ITEM_PID_TV_BD,                 // PID D part at brew detection phase
    STO_ITEM_BREW_SETPOINT,             // brew setpoint
    STO_ITEM_BREW_TEMP_OFFSET,          // brew temp offset
    STO_ITEM_USE_BD_PID,                // use separate PID for brew detection (otherwise continue with regular PID)
    STO_ITEM_BREW_TIME,                 // brew time
    STO_ITEM_BREW_SW_TIME,              // brew software time
    STO_ITEM_BREW_PID_DELAY,            // brew PID delay
    STO_ITEM_BD_THRESHOLD,              // brew detection limit
    STO_ITEM_WIFI_CREDENTIALS_SAVED,    // flag for wifisetup
    STO_ITEM_PRE_INFUSION_TIME,         // pre-infusion time
    STO_ITEM_PRE_INFUSION_PAUSE,        // pre-infusion pause
    STO_ITEM_PID_KP_STEAM,              // PID P part at steam phase
    STO_ITEM_STEAM_SETPOINT,            // Setpoint for Steam mode
    STO_ITEM_SOFT_AP_ENABLED_CHECK,     // soft AP enable state
    STO_ITEM_WIFI_SSID,                 // Wifi SSID
    STO_ITEM_WIFI_PASSWORD,             // Wifi password
    STO_ITEM_WEIGHTSETPOINT,            // Brew weight setpoint
    STO_ITEM_STANDBY_MODE_ON,           // Enable tandby mode
    STO_ITEM_STANDBY_MODE_TIME,         // Time until heater is turned off
    STO_ITEM_SCALE_CALIBRATION_FACTOR,  // Calibration factor for scale
    STO_ITEM_SCALE2_CALIBRATION_FACTOR, // Calibration factor for scale 2
    STO_ITEM_SCALE_KNOWN_WEIGHT,        // Calibration weight for scale
    STO_ITEM_RESERVED_30,               // reserved
    STO_ITEM_RESERVED_21,               // reserved
    STO_ITEM_BACKFLUSH_CYCLES,          // number of cycles the backflush should run
    STO_ITEM_BACKFLUSH_FILL_TIME,       // time in ms the pump is running during backflush
    STO_ITEM_BACKFLUSH_FLUSH_TIME,      // time in ms the 3-way valve is open during backflush

    /* WHEN ADDING NEW ITEMS, THE FOLLOWING HAS TO BE UPDATED:
     * - storage structure:  sto_data_t
     * - item default value: itemDefaults
     * - item address/size:  getItemAddr()
     */

    STO_ITEM__LAST_ENUM // must be the last one!
} sto_item_id_t;

#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

#include "Logger.h"
#include "defaults.h"

#include "isr.h"

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
        float scaleCalibration;
        double preInfusionTimeMs;
        float scaleKnownWeight;
        double preInfusionPauseMs;
        uint8_t freeToUse6[21];
        uint8_t pidBdOn;
        double pidKpBd;
        float scale2Calibration;
        double pidTnBd;
        uint8_t freeToUse8[2];
        double pidTvBd;
        uint8_t freeToUse9[2];
        double brewSwTimeSec;
        double brewPIDDelaySec;
        uint8_t freeToUse10;
        double brewDetectionThreshold;
        uint8_t wifiCredentialsSaved;
        uint8_t useStartPonM;
        double pidKpStart;
        uint8_t freeToUse12[2];
        uint8_t softApEnabledCheck;
        uint8_t freeToUse13[9];
        double pidTnStart;
        uint8_t freeToUse14[2];
        char wifiSSID[25 + 1];
        char wifiPassword[25 + 1];
        double weightSetpoint;
        double steamkp;
        double steamSetpoint;
        uint8_t standbyModeOn;
        double standbyModeTime;
        int backflushCycles;
        double backflushFillTimeMs;
        double backflushFlushTimeMs;

} sto_data_t;

// set item defaults
static const sto_data_t itemDefaults PROGMEM = {
    AGGKP,                                                                                                                          // STO_ITEM_PID_KP_REGULAR
    {0xFF, 0xFF},                                                                                                                   // reserved (maybe for structure version)
    AGGTN,                                                                                                                          // STO_ITEM_PID_TN_REGULAR
    0,                                                                                                                              // STO_ITEM_PID_ON
    0xFF,                                                                                                                           // free to use
    AGGTV,                                                                                                                          // STO_ITEM_PID_TV_REGULAR
    AGGIMAX,                                                                                                                        // STO_ITEM_PID_I_MAX_REGULAR
    0xFF,                                                                                                                           // free to use
    SETPOINT,                                                                                                                       // STO_ITEM_BREW_SETPOINT
    TEMPOFFSET,                                                                                                                     // STO_ITEM_BREW_TEMP_OFFSET
    0xFF,                                                                                                                           // free to use
    BREW_TIME,                                                                                                                      // STO_ITEM_BREW_TIME
    SCALE_CALIBRATION_FACTOR,                                                                                                       // STO_ITEM_SCALE_CALIBRATION_FACTOR
    PRE_INFUSION_TIME,                                                                                                              // STO_ITEM_PRE_INFUSION_TIME
    SCALE_KNOWN_WEIGHT,                                                                                                             // STO_ITEM_SCALE_KNOWN_WEIGHT
    PRE_INFUSION_PAUSE_TIME,                                                                                                        // STO_ITEM_PRE_INFUSION_PAUSE
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // free to use
    0,                                                                                                                              // STO_ITEM_USE_PID_BD
    AGGBKP,                                                                                                                         // STO_ITEM_PID_KP_BD
    SCALE2_CALIBRATION_FACTOR,                                                                                                      // STO_ITEM_SCALE2_CALIBRATION_FACTOR
    AGGBTN,                                                                                                                         // STO_ITEM_PID_TN_BD
    {0xFF, 0xFF},                                                                                                                   // free to use
    AGGBTV,                                                                                                                         // STO_ITEM_PID_TV_BD
    {0xFF, 0xFF},                                                                                                                   // free to use
    BREW_SW_TIME,                                                                                                                   // STO_ITEM_BREW_SW_TIME
    BREW_PID_DELAY,                                                                                                                 // STO_ITEM_BREW_PID_DELAY
    0xFF,                                                                                                                           // free to use
    BD_SENSITIVITY,                                                                                                                 // STO_ITEM_BD_THRESHOLD
    WIFI_CREDENTIALS_SAVED,                                                                                                         // STO_ITEM_WIFI_CREDENTIALS_SAVED
    0,                                                                                                                              // STO_ITEM_USE_START_PON_M
    STARTKP,                                                                                                                        // STO_ITEM_PID_KP_START
    {0xFF, 0xFF},                                                                                                                   // free to use
    0,                                                                                                                              // STO_ITEM_SOFT_AP_ENABLED_CHECK
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},                                                                         // free to use
    STARTTN,                                                                                                                        // STO_ITEM_PID_TN_START
    {0xFF, 0xFF},                                                                                                                   // free to use
    "",                                                                                                                             // STO_ITEM_WIFI_SSID
    "",                                                                                                                             // STO_ITEM_WIFI_PASSWORD
    SCALE_WEIGHTSETPOINT,                                                                                                           // STO_ITEM_WEIGHTSETPOINT
    STEAMKP,                                                                                                                        // STO_ITEM_PID_KP_STEAM
    STEAMSETPOINT,                                                                                                                  // STO_ITEM_STEAM_SETPOINT
    STANDBY_MODE_ON,                                                                                                                // STO_ITEM_STANDBY_MODE_ON
    STANDBY_MODE_TIME,                                                                                                              // STO_ITEM_STANDBY_MODE_TIME
    BACKFLUSH_CYCLES,                                                                                                               // STO_ITEM_BACKFLUSH_CYCLES
    BACKFLUSH_FILL_TIME,                                                                                                            // STO_ITEM_BACKFLUSH_FILLTIME
    BACKFLUSH_FLUSH_TIME,                                                                                                           // STO_ITEM_BACKFLUSH_FLUSHTIME
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

        case STO_ITEM_BREW_PID_DELAY:
            addr = offsetof(sto_data_t, brewPIDDelaySec);
            size = STRUCT_MEMBER_SIZE(sto_data_t, brewPIDDelaySec);
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

        case STO_ITEM_BREW_SW_TIME:
            addr = offsetof(sto_data_t, brewSwTimeSec);
            size = STRUCT_MEMBER_SIZE(sto_data_t, brewSwTimeSec);
            break;

        case STO_ITEM_BD_THRESHOLD:
            addr = offsetof(sto_data_t, brewDetectionThreshold);
            size = STRUCT_MEMBER_SIZE(sto_data_t, brewDetectionThreshold);
            break;

        case STO_ITEM_WIFI_CREDENTIALS_SAVED:
            addr = offsetof(sto_data_t, wifiCredentialsSaved);
            size = STRUCT_MEMBER_SIZE(sto_data_t, wifiCredentialsSaved);
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
            addr = offsetof(sto_data_t, weightSetpoint);
            size = STRUCT_MEMBER_SIZE(sto_data_t, weightSetpoint);
            break;

        case STO_ITEM_STEAM_SETPOINT:
            addr = offsetof(sto_data_t, steamSetpoint);
            size = STRUCT_MEMBER_SIZE(sto_data_t, steamSetpoint);
            break;

        case STO_ITEM_STANDBY_MODE_ON:
            addr = offsetof(sto_data_t, standbyModeOn);
            size = STRUCT_MEMBER_SIZE(sto_data_t, standbyModeOn);
            break;

        case STO_ITEM_STANDBY_MODE_TIME:
            addr = offsetof(sto_data_t, standbyModeTime);
            size = STRUCT_MEMBER_SIZE(sto_data_t, standbyModeTime);
            break;

        case STO_ITEM_SCALE_CALIBRATION_FACTOR:
            addr = offsetof(sto_data_t, scaleCalibration);
            size = STRUCT_MEMBER_SIZE(sto_data_t, scaleCalibration);
            break;

        case STO_ITEM_SCALE2_CALIBRATION_FACTOR:
            addr = offsetof(sto_data_t, scale2Calibration);
            size = STRUCT_MEMBER_SIZE(sto_data_t, scale2Calibration);
            break;

        case STO_ITEM_SCALE_KNOWN_WEIGHT:
            addr = offsetof(sto_data_t, scaleKnownWeight);
            size = STRUCT_MEMBER_SIZE(sto_data_t, scaleKnownWeight);
            break;

        case STO_ITEM_BACKFLUSH_CYCLES:
            addr = offsetof(sto_data_t, backflushCycles);
            size = STRUCT_MEMBER_SIZE(sto_data_t, backflushCycles);
            break;

        case STO_ITEM_BACKFLUSH_FILL_TIME:
            addr = offsetof(sto_data_t, backflushFillTimeMs);
            size = STRUCT_MEMBER_SIZE(sto_data_t, backflushFillTimeMs);
            break;

        case STO_ITEM_BACKFLUSH_FLUSH_TIME:
            addr = offsetof(sto_data_t, backflushFlushTimeMs);
            size = STRUCT_MEMBER_SIZE(sto_data_t, backflushFlushTimeMs);
            break;

        default:
            LOGF(ERROR, "invalid item ID %i!", itemId);
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

    for (idx = 0; idx < storageBufSize; idx++) {               // check all item value bytes...
        if (((const uint8_t*)storageBuf)[idx] != emptyPattern) // other than "empty" value?
            break;                                             // yes -> abort loop (item storage not empty)
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

    for (idx = 0; idx < bufSize; idx++) {    // check all buffer bytes...
        if (((const uint8_t*)buf)[idx] == 0) // null terminator?
            return true;                     // yes -> string exist
    }

    return false;
}

#if 0 // not needed a.t.m.
/**
 * @brief Sets the default values.
 */
static void setDefaults(void) {
    LOGF(TRACE, "%p <- %p (%u)", EEPROM.getDataPtr(), &itemDefaults, sizeof(itemDefaults));
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

    if (!EEPROM.begin(sizeof(sto_data_t))) {
        LOG(FATAL, "EEPROM initialization failed!");
        return -1;
    }

    /* It's not necessary here to check if any valid data are stored,
     * because storageGet() returns a default value in this case.
     */

    return 0;
}

/**
 * @brief Writes all items to the storage medium.
 *
 * @return  0 - succeed
 *         <0 - failed
 */
int storageCommit(void) {
    LOG(INFO, "save all data to EEPROM memory");

    // really write data to storage...
    int returnCode = EEPROM.commit() ? 0 : -1;

    return returnCode;
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
        LOG(ERROR, "invalid item address!");
        return -1;
    }

    LOGF(TRACE, "addr=%i size=%u/%u", itemAddr, sizeof(itemValue), maxItemSize);

    if (sizeof(itemValue) != maxItemSize) {
        LOG(WARNING, "invalid item size (wrong data type)!");
        return -2;
    }

    EEPROM.get(itemAddr, itemValue);

    if (isEmpty(&itemValue, sizeof(itemValue))) {                                 // item storage empty?
        LOG(INFO, "storage empty -> returning default");

        memcpy_P(&itemValue, (PGM_P)&itemDefaults + itemAddr, sizeof(itemValue)); // set default value
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
        LOG(WARNING, "invalid item address!");
        return -1;
    }

    LOGF(TRACE, "addr=%i size=%u/%u commit=%u", itemAddr, sizeof(itemValue), maxItemSize, commit);

    if (sizeof(itemValue) != maxItemSize) {
        LOG(WARNING, "invalid item size (wrong data type)!");
        return -2;
    }

    if (isEmpty(&itemValue, sizeof(T))) {
        LOG(WARNING, "invalid item value!");
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

    LOGF(TRACE, "item=%i value=%.1f", itemId, itemValue);

    return 0;
}

int storageGet(sto_item_id_t itemId, double& itemValue) {
    int retCode = getNumber(itemId, itemValue);

    if (retCode != 0) return retCode;

    LOGF(TRACE, "item=%i value=%.1f", itemId, itemValue);

    return 0;
}

int storageGet(sto_item_id_t itemId, int8_t& itemValue) {
    int retCode = getNumber(itemId, itemValue);

    if (retCode != 0) return retCode;

    LOGF(TRACE, "item=%i value=%i", itemId, itemValue);

    return 0;
}

int storageGet(sto_item_id_t itemId, int16_t& itemValue) {
    int retCode = getNumber(itemId, itemValue);

    if (retCode != 0) return retCode;

    LOGF(TRACE, "item=%i value=%i", itemId, itemValue);

    return 0;
}

int storageGet(sto_item_id_t itemId, int32_t& itemValue) {
    int retCode = getNumber(itemId, itemValue);

    if (retCode != 0) return retCode;

    LOGF(TRACE, "item=%i value=%i", itemId, itemValue);

    return 0;
}

int storageGet(sto_item_id_t itemId, uint8_t& itemValue) {
    int retCode = getNumber(itemId, itemValue);

    if (retCode != 0) return retCode;

    LOGF(TRACE, "item=%i value=%u", itemId, itemValue);

    return 0;
}

int storageGet(sto_item_id_t itemId, uint16_t& itemValue) {
    int retCode = getNumber(itemId, itemValue);
    if (retCode != 0) return retCode;
    LOGF(TRACE, "item=%i value=%u", itemId, itemValue);

    return 0;
}

int storageGet(sto_item_id_t itemId, uint32_t& itemValue) {
    int retCode = getNumber(itemId, itemValue);

    if (retCode != 0) return retCode;

    LOGF(TRACE, "item=%i value=%u", itemId, itemValue);

    return 0;
}

#if 0
/* The ESP32 EEPROM class does not support getConstDataPtr(). The available
 * getDataPtr() always leads to a "dirty" status (= erase/program cycle), even
 * if only read operations will be done!
 * If this function is needed, a temporary read buffer has to be used.
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
        LOG(TRACE, "storage empty -> returning default");
        memcpy_P(&itemValue, (const void*)(&itemDefaults+itemAddr), itemSize);      // set default value
    }

    LOGF(TRACE, "addr=%i size=%u item=%i value=\"%s\"", itemAddr, itemSize, itemId, *itemValue);

    return 0;
}
#endif

int storageGet(sto_item_id_t itemId, String& itemValue) {
    uint16_t maxItemSize;
    int32_t itemAddr = getItemAddr(itemId, &maxItemSize);

    if (itemAddr < 0) {
        LOG(WARNING, "invalid item address!");
        return -1;
    }

    // The ESP32 EEPROM class does not support getConstDataPtr()!
    uint8_t buf[maxItemSize];
    EEPROM.readBytes(itemAddr, buf, maxItemSize);

    if (isString(buf, maxItemSize)) { // exist a null terminator?
        itemValue = String((const char*)buf);
    }
    else {
        LOG(INFO, "storage empty -> returning default");
        itemValue = String((PGM_P)&itemDefaults + itemAddr); // set default string
    }

    LOGF(TRACE, "addr=%i size=%u item=%i value=\"%s\"", itemAddr, itemValue.length() + 1, itemId, itemValue.c_str());

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
    LOGF(TRACE, "item=%i value=%.1f", itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, double itemValue, bool commit) {
    LOGF(TRACE, "item=%i value=%.1f", itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, int8_t itemValue, bool commit) {
    LOGF(TRACE, "item=%i value=%i", itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, int16_t itemValue, bool commit) {
    LOGF(TRACE, "item=%i value=%i", itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, int32_t itemValue, bool commit) {
    LOGF(TRACE, "item=%i value=%i", itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, uint8_t itemValue, bool commit) {
    LOGF(TRACE, "item=%i value=%u", itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, uint16_t itemValue, bool commit) {
    LOGF(TRACE, "item=%i value=%u", itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, uint32_t itemValue, bool commit) {
    LOGF(TRACE, "item=%i value=%u", itemId, itemValue);
    return setNumber(itemId, itemValue, commit);
}

int storageSet(sto_item_id_t itemId, const char* itemValue, bool commit) {
    uint16_t maxItemSize;
    size_t valueSize;
    int32_t itemAddr = getItemAddr(itemId, &maxItemSize);

    if (itemAddr < 0) {
        LOG(WARNING, "invalid item address!");
        return -1;
    }

    valueSize = strlen(itemValue) + 1;
    LOGF(TRACE, "item=%i value=\"%s\" addr=%i size=%u/%u", itemId, itemValue, itemAddr, valueSize, maxItemSize);

    if (valueSize > maxItemSize) { // invalid value size?
        LOG(WARNING, "string too large!");
        return -2;
    }

    memcpy(EEPROM.getDataPtr() + itemAddr, itemValue, valueSize); // copy value to data structure in RAM

    if (commit) return storageCommit();

    return 0;
}

int storageSet(sto_item_id_t itemId, String& itemValue, bool commit) {
    return storageSet(itemId, itemValue.c_str(), commit);
}

/**
 * @brief Resets all storage data.
 *
 * @return  0 - succeed
 *         <0 - failed
 */
int storageFactoryReset(void) {
    LOG(INFO, "reset all values");
    memset(EEPROM.getDataPtr(), 0xFF, sizeof(sto_data_t));

    return storageCommit();
}
