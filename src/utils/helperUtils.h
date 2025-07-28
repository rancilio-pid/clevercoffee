/**
 * @file helperUtils.h
 * @brief Common utility functions used across the application
 */

#pragma once

#include <Arduino.h>
#include <cmath>

// ==================== UTILITY FUNCTIONS ====================

/**
 * @brief Flip uint8_t value between 0 and 1
 * @param value The value to flip
 * @return 1 if value is 0, otherwise 0
 */
inline uint8_t flipUintValue(const uint8_t value) {
    return value == 0 ? 1 : 0;
}

/**
 * @brief Modulo operation that handles negative numbers correctly
 * @param a The dividend
 * @param b The divisor
 * @return The modulo result
 */
inline int mod(const int a, const int b) {
    const int r = a % b;
    return r < 0 ? r + b : r;
}

/**
 * @brief Round a double value to 2 decimal places
 * @param value The value to round
 * @return The rounded value
 */
inline double round2(const double value) {
    return std::round(value * 100.0) / 100.0;
}

/**
 * @brief Validate if a string represents a valid number
 * @param str The string to validate
 * @return true if the string is a valid number, false otherwise
 */
inline bool isValidNumber(const String& str) {
    if (str.length() == 0) return false;

    for (size_t i = 0; i < str.length(); i++) {
        if (!isdigit(str[i]) && str[i] != '.' && str[i] != '-') {
            return false;
        }
    }
    return true;
}
