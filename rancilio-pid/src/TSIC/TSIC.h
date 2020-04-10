/**
* Library for reading TSIC digital temperature sensors 20x, 30x, 50x
* using the Arduino platform.
*
* Version 2.3 (by Roman Schmitz, 2016-11-01)
*		- sensor can be operated with external VCC, so no extra pin is neccessary
*		- standard vcc-pin set to 255 (NO_VCC_PIN)
*		- example for external powering
*
* Version 2.2 (by Roman Schmitz, 2016-06-30)
*		- added calculation for 50x sensors
*		- a parameter in the constructor is used to select sensor type, standard is set to 20x/30x for backward compatibility
*		- example was chaged to show usage of sensor types
* 
* Version 2.1 (changes by Matthias Eibl, 2015-03-31)
* 		- if the TSIC returns an error, the Power PIN is 
* 		  turned LOW (otherwise it produces errors as the 
* 		  start for a healthy sensor is not defined properly.)
* 		- the timeouts are optimized for a faster identification of 
* 		  not connected sensors (if no sensor is connected, the 
* 		  Data Pin will remain in state LOW. As the strobe is usually
* 		  ~ 60us, it is sufficient to set the timeout to a value of
* 		  <<100 loops in the second while loop "while (TSIC_LOW){..."
* 		  in the function "TSIC::readSens". One cycle is -depending on
* 		  the CPU frequency used- ~10us.)
* 
* Version 2 (by Rolf Wagner, 2014-03-09)
*		Improvements:
*		- Arduino > 1.0 compatible
*		- corrected offset (about +2°C)
*		- code run time optimization
*		- no freezing of system in case sensor gets unplugged
*		- measure strobetime instead of constant waiting time (-> high temperature stable)
*		- code cleanup
*		- space optimization, now only 239 byte
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
* http://playground.arduino.cc/Code/Tsic
*/

#ifndef TSIC_h
#define TSIC_h

#include "Arduino.h"

#define TSIC_20x	0
#define TSIC_30x	0
#define TSIC_50x	1

#define NO_VCC_PIN 255

#define TSIC_ON()	digitalWrite(m_vcc_pin, HIGH)
#define TSIC_OFF()	digitalWrite(m_vcc_pin, LOW)
#define TSIC_HIGH	digitalRead(m_signal_pin)
#define TSIC_LOW	!digitalRead(m_signal_pin)
#define TSIC_EXIT()	{TSIC_OFF(); return 0;}
#define Cancel()	if (timeout > 10000){return 0;}				// Cancel if sensor is disconnected

class TSIC {
	public:
		explicit TSIC(uint8_t signal_pin, uint8_t vcc_pin=NO_VCC_PIN, uint8_t sens_type=TSIC_30x);
		uint8_t getTemperature(uint16_t *temp_value16);
		float calc_Celsius(uint16_t *temperature16);
		unsigned long wait_time;
		unsigned long data_start_time;
		unsigned long timeout_tmp;
		unsigned long start;
		unsigned long end;
		unsigned long readSens1;
		unsigned long readSens2;
		bool flag;

	private:
		uint8_t m_signal_pin;
		uint8_t m_vcc_pin;
		uint8_t m_sens_type;
		uint8_t readSens(uint16_t *temp_value);
		uint8_t checkParity(uint16_t *temp_value);
};

#endif /* TSIC_H */
