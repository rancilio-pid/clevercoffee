/*
 * Standard pin mappings based on MCU type 
 */

#if defined(ESP32)
    #include "esp32devkitcv4.h"
#elif defined(ESP8266)
    #include "esp8266nodemcuv2.h"
#else
    #error("MCU not supported");
#endif