/**********************************************************************************************
 * This is another PID test implementation based on the great work of
 * Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 * Arduino PID Library - Version 1.2.1
 *
 * This Library is licensed under the MIT License
 **********************************************************************************************/
 
// http://bestune.50megs.com/typeABC.htm

#ifndef VelocityControllerTypeC_h
#define VelocityControllerTypeC_h
#define LIBRARY_VERSION	0.0.1

#include "RemoteDebug.h"  //https://github.com/JoaoLopesF/RemoteDebug

class VelocityControllerTypeC
{


  public:

  //Constants used in some of the functions below
  #define AUTOMATIC	1
  #define MANUAL	0

  #define DEBUG_printFmtln(fmt, ...) if ((*myDebug).isActive((*myDebug).DEBUG))   (*myDebug).printf("%0u " fmt, millis()/1000, ##__VA_ARGS__)


  //commonly used functions **************************************************************************
    VelocityControllerTypeC(double*, double*, double*,        // * constructor.  links the VelocityControllerTypeC to the Input, Output, and 
        double, double, double, RemoteDebug*);//   Setpoint.  Initial tuning parameters are also set here.
                                          //   (overload for specifying proportional mode)
	
    void SetMode(int Mode);               // * sets VelocityControllerTypeC to either Manual (0) or Auto (non-0)

    bool Compute();                       // * performs the VelocityControllerTypeC calculation.  it should be
                                          //   called every time loop() cycles. ON/OFF and
                                          //   calculation frequency can be set using SetMode
                                          //   SetSampleTime respectively

    void SetOutputLimits(double, double); // * clamps the output to a specific range. 0-255 by default, but
										                      //   it's likely the user will want to change this depending on
										                      //   the application
	


  //available but not commonly used functions ********************************************************
    void SetTunings(double, double,       // * While most users will set the tunings once in the 
                    double);         	    //   constructor, this function gives the user the option
                                          //   of changing tunings during runtime for Adaptive control      	  

    void SetSampleTime(int);              // * sets the frequency, in Milliseconds, with which 
                                          //   the VelocityControllerTypeC calculation is performed.  default is 100
										  
										  
										  
  //Display functions ****************************************************************
	double GetKp();						  // These functions query the pid for interal values.
	double GetKi();						  //  they were created mainly for the pid front-end,
	double GetKd();						  // where it's important to know what is actually 
	int GetMode();						  //  inside the VelocityControllerTypeC.

  private:
	void Initialize();
	
	double dispKp;				// * we'll hold on to the tuning parameters in user-entered 
	double dispKi;				//   format for display purposes
	double dispKd;				//
    
	double kp;                  // * (P)roportional Tuning Parameter
  double ki;                  // * (I)ntegral Tuning Parameter
  double kd;                  // * (D)erivative Tuning Parameter

	int controllerDirection;
	int pOn;

  double *myInput;              // * Pointers to the Input, Output, and Setpoint variables
  double *myOutput;             //   This creates a hard link between the variables and the 
  double *mySetpoint;           //   VelocityControllerTypeC, freeing the user from having to constantly tell us
                                //   what these values are.  with pointers we'll just know.
  RemoteDebug *myDebug;
	unsigned long lastTime;
	//double outputSum;

  double lastInput, lastLastInput, lastOutput;

	unsigned long SampleTime;
	double outMin, outMax;
	bool inAuto, pOnE;
};
#endif
