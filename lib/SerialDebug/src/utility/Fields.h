/*
 * Fields.h
 *
 */

#ifndef UTIL_FIELDS_H_
#define UTIL_FIELDS_H_

///// Includes

#include <Arduino.h>
#include <stdbool.h>

////// Definitions

///// Class

class Fields
{
	public:

		// Constructors

		Fields(String str, const char delimiter = ':', boolean ignoreEmpty = false);
		~Fields(void);

		// Methods

		uint8_t size();
	    String getString(uint8_t fieldNum);
	    char getChar(uint8_t fieldNum);
	    bool isNum(uint8_t fieldNum);
	    int32_t getInt(uint8_t fieldNum);
	    float getFloat(uint8_t fieldNum);
	    void clear();
};

#endif /* UTIL_FIELDS_H_ */

//////// End
