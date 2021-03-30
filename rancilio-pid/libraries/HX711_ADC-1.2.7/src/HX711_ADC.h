/*
   -------------------------------------------------------------------------------------
   HX711_ADC
   Arduino library for HX711 24-Bit Analog-to-Digital Converter for Weight Scales
   Olav Kallhovd sept2017
   -------------------------------------------------------------------------------------
*/

#ifndef HX711_ADC_h
#define HX711_ADC_h

#include <Arduino.h>
#include "config.h"

/*
Note: HX711_ADC configuration values has been moved to file config.h
*/

#define DATA_SET 	SAMPLES + IGN_HIGH_SAMPLE + IGN_LOW_SAMPLE // total samples in memory

#if (SAMPLES  != 1) & (SAMPLES  != 2) & (SAMPLES  != 4) & (SAMPLES  != 8) & (SAMPLES  != 16) & (SAMPLES  != 32) & (SAMPLES  != 64) & (SAMPLES  != 128)
	#error "number of SAMPLES not valid!"
#endif

#if (SAMPLES  == 1) & ((IGN_HIGH_SAMPLE  != 0) | (IGN_LOW_SAMPLE  != 0))
	#error "number of SAMPLES not valid!"
#endif

#if 		(SAMPLES == 1)
#define 	DIVB 0
#elif 		(SAMPLES == 2)
#define 	DIVB 1
#elif 		(SAMPLES == 4)
#define 	DIVB 2
#elif  		(SAMPLES == 8)
#define 	DIVB 3
#elif  		(SAMPLES == 16)
#define 	DIVB 4
#elif  		(SAMPLES == 32)
#define 	DIVB 5
#elif  		(SAMPLES == 64)
#define 	DIVB 6
#elif  		(SAMPLES == 128)
#define 	DIVB 7
#endif

#define SIGNAL_TIMEOUT	100

class HX711_ADC
{	
		
	public:
		HX711_ADC(uint8_t dout, uint8_t sck); 		//constructor
		void setGain(uint8_t gain = 128); 			//value must be 32, 64 or 128*
		void begin();								//set pinMode, HX711 gain and power up the HX711
		void begin(uint8_t gain);					//set pinMode, HX711 selected gain and power up the HX711
		void start(unsigned long t); 					//start HX711 and do tare 
		void start(unsigned long t, bool dotare);		//start HX711, do tare if selected
		int startMultiple(unsigned long t); 			//start and do tare, multiple HX711 simultaniously
		int startMultiple(unsigned long t, bool dotare);	//start and do tare if selected, multiple HX711 simultaniously
		void tare(); 								//zero the scale, wait for tare to finnish (blocking)
		void tareNoDelay(); 						//zero the scale, initiate the tare operation to run in the background (non-blocking)
		bool getTareStatus();						//returns 'true' if tareNoDelay() operation is complete
		void setCalFactor(float cal); 				//set new calibration factor, raw data is divided by this value to convert to readable data
		float getCalFactor(); 						//returns the current calibration factor
		float getData(); 							//returns data from the moving average dataset 
		int getReadIndex(); 						//for testing and debugging
		float getConversionTime(); 					//for testing and debugging
		float getSPS();								//for testing and debugging
		bool getTareTimeoutFlag();					//for testing and debugging
		void disableTareTimeout();					//for testing and debugging
		long getSettlingTime();						//for testing and debugging
		void powerDown(); 							//power down the HX711
		void powerUp(); 							//power up the HX711
		long getTareOffset();						//get the tare offset (raw data value output without the scale "calFactor")
		void setTareOffset(long newoffset);			//set new tare offset (raw data value input without the scale "calFactor")
		uint8_t update(); 							//if conversion is ready; read out 24 bit data and add to dataset
		void setSamplesInUse(int samples);			//overide number of samples in use
		int getSamplesInUse();						//returns current number of samples in use
		void resetSamplesIndex();					//resets index for dataset
		bool refreshDataSet();						//Fill the whole dataset up with new conversions, i.e. after a reset/restart (this function is blocking once started)
		bool getDataSetStatus();					//returns 'true' when the whole dataset has been filled up with conversions, i.e. after a reset/restart
		float getNewCalibration(float known_mass);	//returns and sets a new calibration value (calFactor) based on a known mass input
		bool getSignalTimeoutFlag();				//returns 'true' if it takes longer time then 'SIGNAL_TIMEOUT' for the dout pin to go low after a new conversion is started

	protected:
		void conversion24bit(); 					//if conversion is ready: returns 24 bit data and starts the next conversion
		long smoothedData();						//returns the smoothed data value calculated from the dataset
		uint8_t sckPin; 							//HX711 pd_sck pin
		uint8_t doutPin; 							//HX711 dout pin
		uint8_t GAIN;								//HX711 GAIN
		float calFactor = 1.0;						//calibration factor as given in function setCalFactor(float cal)
		float calFactorRecip = 1.0;					//reciprocal calibration factor (1/calFactor), the HX711 raw data is multiplied by this value
		volatile long dataSampleSet[DATA_SET + 1];	// dataset, make voltile if interrupt is used 
		long tareOffset;
		int readIndex = 0;
		unsigned long conversionStartTime;
		unsigned long conversionTime;
		uint8_t isFirst = 1;
		uint8_t tareTimes;
		uint8_t divBit = DIVB;
		const uint8_t divBitCompiled = DIVB;
		bool doTare;
		bool startStatus;
		unsigned long startMultipleTimeStamp;
		unsigned long startMultipleWaitTime;
		uint8_t convRslt;
		bool tareStatus;
		unsigned int tareTimeOut = (SAMPLES + IGN_HIGH_SAMPLE + IGN_HIGH_SAMPLE) * 150; // tare timeout time in ms, no of samples * 150ms (10SPS + 50% margin)
		bool tareTimeoutFlag;
		bool tareTimeoutDisable = 0;
		int samplesInUse = SAMPLES;
		long lastSmoothedData = 0;
		bool dataOutOfRange = 0;
		unsigned long lastDoutLowTime = 0;
		bool signalTimeoutFlag = 0;
};	

#endif
   