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

#include "Arduino.h"
#include "TSIC.h"

// Initialize inputs/outputs
TSIC::TSIC(uint8_t signal_pin, uint8_t vcc_pin, uint8_t sens_type)
	: m_signal_pin(signal_pin), m_vcc_pin(vcc_pin), m_sens_type(sens_type)
{
    if(m_vcc_pin!=NO_VCC_PIN) pinMode(m_vcc_pin, OUTPUT);
    pinMode(m_signal_pin, INPUT);
}

// read temperature
uint8_t TSIC::getTemperature(uint16_t *temp_value16){
		uint16_t temp_value1 = 0;
		uint16_t temp_value2 = 0;

		if(m_vcc_pin!=NO_VCC_PIN) TSIC_ON();
		delayMicroseconds(50);     // wait for stabilization
		if(TSIC::readSens(&temp_value1)){}			// get 1st byte
		else TSIC_EXIT();
		if(TSIC::readSens(&temp_value2)){}			// get 2nd byte
		else TSIC_EXIT();
		if(checkParity(&temp_value1)){}		// parity-check 1st byte
		else TSIC_EXIT();
		if(checkParity(&temp_value2)){}		// parity-check 2nd byte
		else TSIC_EXIT();

		if(m_vcc_pin!=NO_VCC_PIN) TSIC_OFF();		// turn off sensor
		*temp_value16 = (temp_value1 << 8) + temp_value2;
		return 1;
}

//-------------Unterprogramme-----------------------------------------------------------------------

/*	Temperature conversion from uint to float in °C with 1 decimal place.
	The calculation is speed-optimized at the cost of a sligtly worse temperature resolution (about -0,0366°C @25°C).
*/
float TSIC::calc_Celsius(uint16_t *temperature16){
	int16_t temp_value16 = 0;
	float celsius = 0;

	// optimized version of (temp_value/2047*(HT-LT)+LT) 
	// calculate temperature *10, i.e. 26,4 = 264
	if(m_sens_type==1) { // 50x sensors: LT=-10, HT=60
		temp_value16 = ((*temperature16 * 175L) >> 9) - 100;
	}
	else { // 20x,30x sensors: LT=-50, HT=150
		temp_value16 = ((*temperature16 * 250L) >> 8) - 500;
	}
	celsius = temp_value16 / 10 + (float) (temp_value16 % 10) / 10;	// shift comma by 1 digit e.g. 26,4°C
	return celsius;
}

uint8_t TSIC::readSens(uint16_t *temp_value){
	uint16_t strobelength = 0;
	uint16_t strobetemp = 0;
	uint8_t dummy = 0;
	uint16_t timeout = 0;	// max value for timeout is set in .h file
	while (TSIC_HIGH){	// wait until start bit starts
		timeout++;
		delayMicroseconds(10);
		Cancel();
	}
	// Measure strobe time, a healthy sensor will go to LOW within a few loops (~60us)
	// if no sensor is connected, the timeout cancels the operation (-> 100cycles are more than enough for this)
	strobelength = 0;
	timeout = 9900;		// max value for timeout is set in .h file
	while (TSIC_LOW) {    // wait for rising edge
		strobelength++;
		timeout++;
		delayMicroseconds(10);
		Cancel();
	}
	for (uint8_t i=0; i<9; i++) {
		// Wait for bit start
		timeout = 0;
		while (TSIC_HIGH) { // wait for falling edge
			timeout++;
			Cancel();
		}
		// Delay strobe length
		timeout = 0;
		dummy = 0;
		strobetemp = strobelength;
		while (strobetemp--) {
			timeout++;
			dummy++;
			delayMicroseconds(10);
			Cancel();
		}
		*temp_value <<= 1;
		// Read bit
		if (TSIC_HIGH) {
			*temp_value |= 1;
		}
		// Wait for bit end
		timeout = 0;
		while (TSIC_LOW) {		// wait for rising edge
			timeout++;
			Cancel();
		}
	}
	return 1;
}

uint8_t TSIC::checkParity(uint16_t *temp_value) {
	uint8_t parity = 0;

	for (uint8_t i = 0; i < 9; i++) {
		if (*temp_value & (1 << i))
			parity++;
	}
	if (parity % 2)
		return 0;				// wrong parity
	*temp_value >>= 1;          // delete parity bit
	return 1;
}
