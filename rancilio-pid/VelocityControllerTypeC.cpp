/**********************************************************************************************
 * This is another PID test implementation based on the great work of
 * Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 * Arduino PID Library - Version 1.2.1
 * This Library is licensed under the MIT License
 **********************************************************************************************/
 
#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "VelocityControllerTypeC.h"

VelocityControllerTypeC::VelocityControllerTypeC(double* Input, double* Output, double* Setpoint,
        double Kp, double Ki, double Kd, RemoteDebug* Debug)
{
    myOutput = Output;
    myInput = Input;
    mySetpoint = Setpoint;
    inAuto = false;
    lastInput = *myInput;
    lastLastInput = *myInput;
    lastOutput = *myOutput;
    myDebug = Debug;

    VelocityControllerTypeC::SetOutputLimits(0, 1000);
    SampleTime = 1000;

    VelocityControllerTypeC::SetTunings(Kp, Ki, Kd);

    lastTime = millis()-SampleTime;
}

bool VelocityControllerTypeC::Compute()
{
   if(!inAuto) return false;
   unsigned long now = millis();
   unsigned long timeChange = (now - lastTime);
   
   if(timeChange>=SampleTime)
   {
      double output = 0;
      lastOutput = *myOutput;
      double input = *myInput;
      double error = *mySetpoint - input;

      output = 
        lastOutput 
        - kp * (input - lastInput)
        + ki * error 
        - kd * (input - 2*lastInput + lastLastInput);
      if (output > outMax) output = outMax;
      else if(output < outMin) output = outMin;
      DEBUG_printFmtln("output %0.2f = (%0.2f) - (%0.2f) + (%0.2f) - (%0.2f)\n",  output, lastOutput, kp * (input - lastInput), ki * error, kd * (input - 2*lastInput + lastLastInput) );
      
      *myOutput = output;
      lastLastInput = lastInput;
      lastInput = input;
      lastTime = now;
      return true;
   }
   else return false;
}

void VelocityControllerTypeC::SetTunings(double Kp, double Ki, double Kd)
{
   if (Kp<0 || Ki<0 || Kd<0) return;

   dispKp = Kp; dispKi = Ki; dispKd = Kd;

   double SampleTimeInSec = ((double)SampleTime)/1000;
   kp = Kp;
   ki = Ki * SampleTimeInSec;
   kd = Kd / SampleTimeInSec;
}

void VelocityControllerTypeC::SetSampleTime(int NewSampleTime)
{
   if (NewSampleTime > 0)
   {
      double ratio = (double)NewSampleTime / (double)SampleTime;
      ki *= ratio;
      kd /= ratio;
      SampleTime = (unsigned long)NewSampleTime;
   }
}

void VelocityControllerTypeC::SetOutputLimits(double Min, double Max)
{
   if(Min > Max) return;
   outMin = Min;
   outMax = Max;
   if(inAuto)
   {
	   if(*myOutput > outMax) *myOutput = outMax;
	   else if(*myOutput < outMin) *myOutput = outMin;

	   if(lastOutput > outMax) lastOutput = outMax;
	   else if(lastOutput < outMin) lastOutput = outMin;
   }
}

void VelocityControllerTypeC::SetMode(int Mode)
{
    bool newAuto = (Mode == AUTOMATIC);
    if(newAuto && !inAuto)
    {  /*we just went from manual to auto*/
        VelocityControllerTypeC::Initialize();
    }
    inAuto = newAuto;
}

void VelocityControllerTypeC::Initialize()
{
   // outputSum = *myOutput;
   lastInput = *myInput;
   lastLastInput = *myInput;
   lastOutput = *myOutput;
   if(*myOutput > outMax) *myOutput = outMax;
   else if(*myOutput < outMin) *myOutput = outMin;
   if(lastOutput > outMax) lastOutput = outMax;
   else if(lastOutput < outMin) lastOutput = outMin;
}

double VelocityControllerTypeC::GetKp(){ return  dispKp; }
double VelocityControllerTypeC::GetKi(){ return  dispKi;}
double VelocityControllerTypeC::GetKd(){ return  dispKd;}
int VelocityControllerTypeC::GetMode(){ return  inAuto ? AUTOMATIC : MANUAL;}
