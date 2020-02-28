/**********************************************************************************************
 * This is another PID test implementation based on the great work of
 * Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 * Arduino PID Library - Version 1.2.1
 * This Library is licensed under the MIT License
 **********************************************************************************************/

#include <stdio.h>

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

    VelocityControllerTypeC::SetOutputLimits(0, 5000);
    SampleTime = 5000;

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
      lastOutput = *myOutput;
      double input = *myInput;
      double error = *mySetpoint - input;

      outputK = 0.0;
      outputI = 0.0;
      outputD = 0.0;
      if ( input > 15 && fabs(input - lastInput) < 5 ) {  //TODO: BUG: why is input=0 after startup!
        outputK = -kp * (input - lastInput);
        outputI = ki * error;
        //outputD = -kd * (input - 2*lastInput + lastLastInput);
      }

      setPointInSeconds = 0 ;

      double deadTime = 40;
      double pastChange = pastTemperatureChange(10);  // in seconds. Must be < deadTime
      if ( fabs(pastChange) > 0 ) {  // dont over-estimate near setPoint. Let PI do the job.
         //input + pastChange * SetPointInSeconds = mySetpoint;
        setPointInSeconds = error / pastChange;
        int direction = +1;
        if (error < 0) {
          direction = -1;
        }
        if ( fabs(setPointInSeconds) < deadTime ) {  // only tune if setPoint is hit in the next "deadTime" seconds
          if (setPointInSeconds >=0 && setPointInSeconds < 1) { setPointInSeconds = 1; }
          else if (setPointInSeconds <=0 && setPointInSeconds > -1) setPointInSeconds = -1;
          outputD = -kd * direction * (deadTime / setPointInSeconds) ;
        } else if ( fabs(setPointInSeconds) > 99) {
          setPointInSeconds = 100;
        }
      }
      double output =
        lastOutput
        + outputK
        + outputI
        + outputD;

      //if (outputD > 0 && error > 0) {
      //  output += outputD;
      //}

      //forget previous POSITIVE OutputD if temperature direction has changed / is changing
      if ( outputD > 0 ) sumOutputD += outputD;  
      if ( (sumOutputD >0 && outputD <0) || (error <= 0 && sumOutputD >0) ) {
        DEBUG_printLib("Input=%5.2f error=%0.2f sumOutputD=%0.2f\n", input, error, convertOutputToUtilisation(sumOutputD) );
        output -= sumOutputD;
        sumOutputD = 0;
      }

      double steadyPower = 4.2;
      if ( (output < convertUtilisationToOutput(steadyPower)) && (error >= -0.5) ) {
        output = convertUtilisationToOutput(steadyPower);
      }
      
      if (output > outMax) output = outMax;
      else if(output < outMin) output = outMin;
      //DEBUG_printLib("Input=%5.2f error=%0.2f SetPoint=%0.2f\n", input, error, (double)*mySetpoint);
      //DEBUG_printLib("Input=%6.2f | DiffTemp=%5.2f | SetInSecs=%0.2f | Output=%6.2f = %6.2f + k:%5.2f + i:%5.2f + d:%5.2f ***\n", 
      //  input,
      //  error,
      //  setPointInSeconds,
      //  convertOutputToUtilisation(output), 
      //  convertOutputToUtilisation(lastOutput), 
      //  convertOutputToUtilisation(outputK), 
      //  convertOutputToUtilisation(outputI),
      //  convertOutputToUtilisation(outputD)    
      //  );    
      
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
   sumOutputD = 0;
   if(*myOutput > outMax) *myOutput = outMax;
   else if(*myOutput < outMin) *myOutput = outMin;
   if(lastOutput > outMax) lastOutput = outMax;
   else if(lastOutput < outMin) lastOutput = outMin;
}

double VelocityControllerTypeC::GetKp() { return dispKp;}
double VelocityControllerTypeC::GetKi() { return dispKi;}
double VelocityControllerTypeC::GetKd() { return dispKd;}
double VelocityControllerTypeC::GetOutputK() { return outputK;}
double VelocityControllerTypeC::GetOutputI() { return outputI;}
double VelocityControllerTypeC::GetOutputD() { return outputD;} 
double VelocityControllerTypeC::GetLastOutput() { return lastOutput;} 
double VelocityControllerTypeC::GetSetPointInSeconds() { return setPointInSeconds;} 
int VelocityControllerTypeC::GetMode() { return  inAuto ? AUTOMATIC : MANUAL;}
