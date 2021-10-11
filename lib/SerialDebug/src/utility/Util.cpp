/*****************************************
 * Project   : Util - Utilities
 * Programmer: Joao Lopes
 * Module    : Util - generic utilities
 * Comments  : TODO: make it a library to Arduino and ESP-IDF
 *             Note this not is a C++ class (yet)
 * Versions  :
 * ------- 	----------  -------------------------
 * 0.1.0 	2018-05-09 	First version
 */

///// Includes

#include <Arduino.h>


////// Routines

// Is CR or LF ?

boolean isCRLF(char character) {

	return (character == '\r' || character == '\n');

}

//String is numeric ?

boolean strIsNum(String str) {

    bool isNum = false;

    for (uint8_t i = 0; i < str.length(); i++) {
        isNum = isdigit(str[i]) || str[i] == '+' ||
                        str[i] == '.' || str[i] == '-';
        if (!isNum)
            return false;
    }

    return isNum;
}



