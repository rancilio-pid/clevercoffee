/**
 * @file    userConfig_sample.h
 * @brief   Values must be configured by the user
 * @version 4.0.0 Master
 *
 */

#pragma once

// Display
#define OLED_DISPLAY  2       // 0 = deactivated, 1 = SH1106 (e.g. 1.3 "128x64), 2 = SSD1306 (e.g. 0.96" 128x64)
#define OLED_I2C      0x3C    // I2C address for OLED, 0x3C by default
#define DISPLAYROTATE U8G2_R0 // rotate display clockwise: U8G2_R0 = no rotation; U8G2_R1 = 90°; U8G2_R2 = 180°; U8G2_R3 = 270°

#define LANGUAGE 0            // LANGUAGE = 0 (DE), LANGUAGE = 1 (EN), LANGUAGE = 2 (ES)

// Connectivity
#define CONNECTMODE 1              // 0 = offline 1 = WIFI-MODE
#define PASS        "CleverCoffee" // default password for WiFiManager

#define TEMP_SENSOR 2              // Temp sensor type: 1 = DS18B20, 2 = TSIC306
