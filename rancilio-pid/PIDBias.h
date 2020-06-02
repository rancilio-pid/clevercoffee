/**********************************************************************************************
 * This is another PID test implementation based on the great work of
 * 
 * * Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 * * Arduino PID Library - Version 1.2.1
 * * This Library is licensed under the MIT License
 * 
 * all credits go to him. (Tobias <medlor@web.de>)
 **********************************************************************************************/
 
#ifndef PIDBias_h
#define PIDBias_h
#define LIBRARY_VERSION	0.0.1

#include "rancilio-pid.h"

class PIDBias
{
  public:

  //Constants used in some of the functions below
  #define AUTOMATIC	1
  #define MANUAL	0

  //commonly used functions **************************************************************************
    PIDBias(double*, double*, double*, double*, unsigned long*, int*, double*, double, double, double);

    void SetMode(int Mode);               // * sets PIDBias to either Manual (0) or Auto (non-0)

    int Compute();                       // * performs the PIDBias calculation.  it should be
                                          //   called every time loop() cycles. ON/OFF and
                                          //   calculation frequency can be set using SetMode
                                          //   SetSampleTime respectively
                                          // returns:
                                          // 1 := when compute is run
                                          // 0 := compute not run yet
                                          // 2 := time for compute to run, but pid is disabled

    void SetOutputLimits(double, double); // * clamps the output to a specific range. 0-255 by default, but
										                      //   it's likely the user will want to change this depending on
										                      //   the application

  //available but not commonly used functions ********************************************************
    void SetTunings(double, double,       // * While most users will set the tunings once in the 
                    double);         	    //   constructor, this function gives the user the option
                                          //   of changing tunings during runtime for Adaptive control      	  

    void SetSampleTime(int);              // * sets the frequency, in Milliseconds, with which 
                                          //   the PIDBias calculation is performed.  default is 100
										  
										  
										  
  //Display functions ****************************************************************
	double GetKp();						  // These functions query the pid for interal values.
	double GetKi();						  //  they were created mainly for the pid front-end,
	double GetKd();						  // where it's important to know what is actually 
	int GetMode();						  //  inside the PIDBias.

  double GetOutputP();
  double GetOutputI();
  double GetSumOutputI();
  double GetOutputD(); 
  double GetLastOutput();
  void SetBurst(double);
  void SetSumOutputI(double);
  int signnum_c(double);
  void SetFilterSumOutputI(double);
  double GetFilterSumOutputI();
  //void SetSteadyPowerOffset(double);
  //double GetSteadyPowerOffset();
  void SetAutoTune(boolean);
  void SetSteadyPowerDefault(double);
  double GetSteadyPowerOffset();
  //void UpdateSteadyPowerOffset(unsigned long, unsigned long);
  double CalculateSteadyPowerOffset();
  double GetSteadyPowerOffsetCalculated();

  private:
	void Initialize();
	
	double dispKp;				// * we'll hold on to the tuning parameters in user-entered 
	double dispKi;				//   format for display purposes
	double dispKd;				//
    
	double kp;                  // * (P)roportional Tuning Parameter
  double ki;                  // * (I)ntegral Tuning Parameter
  double kd;                  // * (D)erivative Tuning Parameter

  double outputP;
  double outputI;
  double outputD;
  double sumOutputD;
  double sumOutputI;
  double burstOutput;
  double filterSumOutputI;
  double steadyPowerDefault;
  double *mySteadyPowerOffset;
  unsigned long* mySteadyPowerOffset_Activated;
  int* mySteadyPowerOffset_Time;
  double steadyPowerOffsetCalculated;
  boolean steadyPowerAutoTune;

  double *myInput;              // * Pointers to the Input, Output, and Setpoint variables
  double *myOutput;             //   This creates a hard link between the variables and the 
  double *mySetpoint;           //   PIDBias, freeing the user from having to constantly tell us
                                //   what these values are.  with pointers we'll just know.
  double *mySteadyPower;
	unsigned long lastTime;
  unsigned long lastTrigger, lastTrigger2;
  double lastOutput, lastError;
	unsigned long SampleTime;
	double outMin, outMax;
	bool inAuto;
};
#endif
