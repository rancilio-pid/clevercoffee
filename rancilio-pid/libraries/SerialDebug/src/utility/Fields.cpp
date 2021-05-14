/*****************************************
 * Project   : Util - Utilities
 * Programmer: Joao Lopes
 * Module    : fields - toprocess delimited fields in string
 * Comments  : TODO: make it a library to Arduino and ESP-IDF
 * Versions  :
 * ------- 	----------	-------------------------
 * 0.1.0 	2018-05-09 	First version
 */

///// Includes

#include <Arduino.h>

#include <utility/Fields.h>
#include <utility/Util.h>

////// Variables

// Vector

// Arduino arch have C++ std vector ?

#if defined ESP8266 || defined ESP32 || defined _GLIBCXX_VECTOR

	#define VECTOR_STD true

	// C++ std vector

	#include <vector>

	using namespace std;

	static vector <String> fields;

#else // This Arduino arch is not compatible with std::vector

	// Using a lightweight Arduino_Vector library: https://github.com/zacsketches/Arduino_Vector/
	// Changed and otimized by JoaoLopesF

	#include <utility/Vector.h>

	static Vector <String> fields;


#endif

////// Class

/**
* @brief Fields C++ class constructor
*/
Fields::Fields(String str, const char delimiter, boolean ignoreEmpty) {

	// Process the string and populate a vector with this fields
    // TODO: see how ignore substring between '"'

	clear();

	// Split and add it to vector

    int16_t pos = 0;
    int16_t lastPos = 0;

    str.concat(delimiter);
    int16_t size = str.length();

//    Serial.print("field str=");
//    Serial.println(str);

    do {

        pos = str.indexOf(delimiter, lastPos);

        //Serial.printf("pos=%u last=%u\r\n", pos, lastPos);

        if (pos >= 0) {

        	if (!(ignoreEmpty && pos == lastPos)) {

            	String field = str.substring(lastPos, pos);

//#if defined ESP8266 || defined ESP32
//            	Serial.printf("field %s len=%u pos=%u last=%u\r\n", field.c_str(), field.length(), pos, lastPos);
//#else
//            	Serial.print ("field ");
//            	Serial.print (field);
//				Serial.print (" len=");
//				Serial.print (field.length());
//				Serial.print (" pos-");
//				Serial.print (pos);
//				Serial.print (" last=");
//				Serial.println(lastPos);
//#endif
            	if (!(ignoreEmpty && field.length() == 0)) {

//            		// Create a char* and alloc memory to it
//
//            		size_t size = field.length() + 1;
//            		char* item = (char*) malloc(sizeof(char) * size);
//            		memset(item, '\0', size);
//            		strncpy (item, field.c_str(), size - 1);
//
//            		// Put it in vector
//
//            		fields.push_back((const char*) item);

					// Put it in vector

					fields.push_back(field);

//            		Serial.print("push_back ");
//            		Serial.println(field);
            	}
        	}

            if (pos == size) {
                break;
            }

            lastPos = pos + 1;
        }
    }
    while (pos >= 0);

//    // Debug (give it commented)
//
//#if defined ESP8266 || defined ESP32
//    Serial.printf("fields.size() = %u \r\n", fields.size());
//    for (uint8_t i=1; i <= fields.size(); i++) {
//		Serial.printf("field %u = %s \r\n", i,  getString(i).c_str());
//    }
//#else
//    Serial.print("fields.size() = ");
//    Serial.println(fields.size());
//    for (uint8_t i=1; i <= fields.size(); i++) {
//		Serial.print("field ");
//		Serial.print(i);
//		Serial.print(" = ");
//		Serial.println(getString(i).c_str());
//    }
//#endif
}

/**
* @brief Fields C++ class destructor
*/
Fields::~Fields()
{
	// Clear the vector, free memory of items before

	clear();

}

/////// Methods

/**
* @brief clear the vector
*/

void Fields::clear() {

//	// Clear the vector, free memory of items before
//
//    for (uint8_t i = 0; i < fields.size(); i++) {
//    	if (fields[i]) {
//    		free ((void*)fields[i]);
//    	}
//    }

	// Clear the vector

	fields.clear();

}

/**
* @brief Return the size of fields load in vector
*/
uint8_t Fields::size() {

	return fields.size();
}

/**
* @brief Get field type string
*/
String Fields::getString(uint8_t fieldNum) {

	if (fieldNum > 0 && fieldNum <= fields.size()) {
		return fields[fieldNum - 1];
	} else {
		return "";
	}
}

/**
* @brief Get field type char
*/
char Fields::getChar(uint8_t fieldNum) {

	if (fieldNum > 0 && fieldNum <= fields.size()) {
		return fields[fieldNum - 1][0];
	} else {
		return '\0';
	}

}

/**
* @brief Returns if the contents is numeric or not
*/
bool Fields::isNum(uint8_t fieldNum) {

	if (fieldNum > 0 && fieldNum <= fields.size()) {
		return strIsNum(fields[fieldNum - 1]);
	} else {
		return false;
	}
}

/**
* @brief Get field type int 
*/
int32_t Fields::getInt(uint8_t fieldNum) {

	if (fieldNum > 0 && fieldNum <= fields.size()) {
		return (strIsNum(fields[fieldNum - 1]) ? String(fields[fieldNum - 1]).toInt() : -1);
	} else {
		return 0;
	}
}

/**
* @brief Get field type float
*/
float Fields::getFloat(uint8_t fieldNum) {

	if (fieldNum > 0 && fieldNum <= fields.size()) {
		return (strIsNum(fields[fieldNum - 1]) ? String(fields[fieldNum - 1]).toFloat() : -1.0f);
	} else {
		return 0.0f;
	}

}

//////// End
