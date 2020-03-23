/**********************************************************************************************
 * This is another PID test implementation by <medlor@web.de> based on the great work of
 * 
 * * Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 * * Arduino PID Library - Version 1.2.1
 * * This Library is licensed under the MIT License
 *
 * credits go to him.
 * 
 * Tobias <medlor@web.de>, Code changes done by me are released under GPLv3.
 *
 **********************************************************************************************/

#include <stdio.h>
#include <math.h>
#include <float.h>

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "PIDBias.h"

double PIDBias::signnum_c(double x) {
  if (x >= 0.0) return 1.0;
  if (x < 0.0) return -1.0;
}

PIDBias::PIDBias(double* Input, double* Output, double* steadyPower, double* Setpoint,
        double Kp, double Ki, double Kd, RemoteDebug* Debug)
{
    myOutput = Output;
    myInput = Input;
    mySetpoint = Setpoint;
    mySteadyPower = steadyPower;
    inAuto = MANUAL;
    lastInput = *myInput;
    lastLastInput = *myInput;
    lastOutput = *myOutput;
    myDebug = Debug;
    steadyPowerDefault = *mySteadyPower;
    steadyPowerOffset = 0;

    PIDBias::SetOutputLimits(0, 5000);
    SampleTime = 5000;

    PIDBias::SetTunings(Kp, Ki, Kd);

    lastTime = millis()-SampleTime;
}

bool PIDBias::Compute()
{
   if(!inAuto) return false;
   unsigned long now = millis();
   unsigned long timeChange = (now - lastTime);
   double steadyPowerOutput = convertUtilisationToOutput(*mySteadyPower);
   double setPointBand = 0.1;
   
   if(timeChange>=SampleTime)
   {
      lastOutput = *myOutput;
      double input = *myInput;
      double error = *mySetpoint - input;
      double pastChange = pastTemperatureChange(10) / 2;  // in seconds

      outputP = kp * error;
      outputI = ki * error;
      // outputD = -kd * (input - 2*lastInput + lastLastInput);
      outputD = -kd * pastChange;

      sumOutputI += outputI;

      //Filter too high sumOutputI
      if ( sumOutputI > filterSumOutputI ) sumOutputI = filterSumOutputI;
      
      //reset sumI when at setPoint. This improves stabilization.
      if ( signnum_c(error) * signnum_c(lastError) < 0 ) { //temperature curve crosses setPoint
        DEBUG_printLib("Crossing setPoint\n");
        //moving upwards
        if ( sumOutputI > 0 ) {
          //steadyPower auto-tuning
          if ( steadyPowerAutoTune && pastChange <= 0.3 && convertOutputToUtilisation(sumOutputI) >= 0.3 && sumOutputI < filterSumOutputI ) {
            DEBUG_printLib("Auto-Tune steadyPower(%0.2f += %0.2f) (moving up with too much outputI)\n", *mySteadyPower, 0.1); //convertOutputToUtilisation(sumOutputI / 2));
            *mySteadyPower += 0.1; // convertOutputToUtilisation(sumOutputI / 2);  //TODO write to epprom?
            //TODO: remember sumOutputI and restore it back to 1/2 when moving down later. only when we are moving up <0.2
          }
        }
        //moving downwards
        if ( steadyPowerAutoTune && pastChange <= -0.2 && pastChange > -0.4 ) { //we only detect major issues when going downwards
          //steadyPower auto-tuning
          DEBUG_printLib("Auto-Tune steadyPower(%0.2f += %0.2f) (moving down too fast)\n", *mySteadyPower, 0.1); //convertOutputToUtilisation(sumOutputI / 2));
          *mySteadyPower += 0.1;
        }
        DEBUG_printLib("Set sumOutputI=0 to stabilize(%0.2f)\n", convertOutputToUtilisation(sumOutputI));
        sumOutputI = 0;
      }
      
      //above target band (setPoint_band) and temp_changes too fast
      if ( steadyPowerAutoTune && error < -setPointBand && pastTemperatureChange(20) <= 0.1 && pastTemperatureChange(20) > 0 &&
           (millis() - lastTrigger >30000) ) {
        //steadyPower auto-tuning
        DEBUG_printLib("Auto-Tune steadyPower(%0.2f -= %0.2f) (moving up too much)\n", *mySteadyPower, 0.2);
        *mySteadyPower -= 0.2; //TODO write to epprom?
        lastTrigger = millis();
      }
      //below target band and temp not going up fast enough 
      else if ( steadyPowerAutoTune && error >= (setPointBand+0.1) && pastTemperatureChange(30) <= 0.1 && pastTemperatureChange(30) >= 0 &&
                sumOutputI == filterSumOutputI && (millis() - lastTrigger >20000) ) {
        //steadyPower auto-tuning
        DEBUG_printLib("Auto-Tune steadyPower(%0.2f += %0.2f) (not moving. either Kp or steadyPower too low)\n", *mySteadyPower, 0.1);
        *mySteadyPower += 0.1; //TODO write to epprom?
        lastTrigger = millis();
      }

      //Auto-tune should never increase steadyPower too much (this prevents bugs due to not thought off uses)
      if ( steadyPowerAutoTune && (*mySteadyPower > steadyPowerDefault * 1.5 || *mySteadyPower < steadyPowerDefault * 0.5) ) {
        DEBUG_printLib("Auto-Tune steadyPower(%0.2f) is off too far. Set back to %0.2f\n", *mySteadyPower, steadyPowerDefault);
        *mySteadyPower = steadyPowerDefault; 
      }
      if ( steadyPowerAutoTune && *mySteadyPower > 10 ) {
        DEBUG_printLib("Auto-Tune steadyPower(%0.2f) is by far too high. Set back to %0.2f\n", *mySteadyPower, 4.7);
        *mySteadyPower = 4.7; 
      }

      //TODO safe-guard against always moving up because of steadyPower (eg no steadyPower above error < 5)

      //If we are above setpoint, we dont want to overly reduce output when temperature is moving downwars
      if ( error < 0 ) {

        //negative outputI shall not be accumulated when above setpoint
        if ( sumOutputI < 0 ) sumOutputI = outputI;  
        
        //reduce impact of outputD
        if ( outputD >= 0 ) {
          //moving downwards: reduce impact of outputD
          outputD /= 1;
        } else if ( outputD < 0 ){
          //moving upwards
          outputD /= 4;
        }
      }

      double output =
        + steadyPowerOutput + steadyPowerOffset // add steadyPower (bias) always to output
        + outputP
        + sumOutputI
        + outputD;

      //temporary overwrite output when bust is triggered
      if ( burstOutput > 0 ) {
        output = burstOutput;
        burstOutput = 0;
      }

      if (output > outMax) output = outMax;
      else if(output < outMin) output = outMin;
      
      //DEBUG_printLib("Input=%5.2f error=%0.2f SetPoint=%0.2f\n", input, error, (double)*mySetpoint);
      //DEBUG_printLib("Input=%6.2f | DiffTemp=%5.2f | SetInSecs=%0.2f | Output=%6.2f = %6.2f + p:%5.2f + i:%5.2f + d:%5.2f ***\n", 
      //  input,
      //  error,
      //  setPointInSeconds,
      //  convertOutputToUtilisation(output), 
      //  convertOutputToUtilisation(lastOutput), 
      //  convertOutputToUtilisation(outputP), 
      //  convertOutputToUtilisation(outputI),
      //  convertOutputToUtilisation(outputD)    
      //  );    
      
      *myOutput = output;
      lastLastInput = lastInput;
      lastError = error;
      lastInput = input;
      lastTime = now;
      return true;
   }
   else return false;
}

void PIDBias::SetTunings(double Kp, double Ki, double Kd)
{
   if (Kp<0 || Ki<0 || Kd<0) return;

   dispKp = Kp; dispKi = Ki; dispKd = Kd;

   double SampleTimeInSec = ((double)SampleTime)/1000;
   kp = Kp;
   ki = Ki * SampleTimeInSec;
   kd = Kd / SampleTimeInSec;
}

void PIDBias::SetSampleTime(int NewSampleTime)
{
   if (NewSampleTime > 0)
   {
      double ratio = (double)NewSampleTime / (double)SampleTime;
      ki *= ratio;
      kd /= ratio;
      SampleTime = (unsigned long)NewSampleTime;
   }
}

void PIDBias::SetOutputLimits(double Min, double Max)
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

void PIDBias::SetMode(int Mode)
{
    bool newAuto = (Mode == AUTOMATIC);
    if(newAuto && !inAuto)
    {  /*we just went from manual to auto*/
        PIDBias::Initialize();
    }
    inAuto = newAuto;
}

void PIDBias::Initialize()
{
   lastInput = *myInput;
   lastLastInput = *myInput;
   lastOutput = *myOutput;
   burstOutput = 0;
   sumOutputD = 0;
   sumOutputI = 0;
   lastTrigger = 0;
   steadyPowerAutoTune = true;
   steadyPowerDefault = *mySteadyPower;
   filterSumOutputI = outMax;
   if(*myOutput > outMax) *myOutput = outMax;
   else if(*myOutput < outMin) *myOutput = outMin;
   if(lastOutput > outMax) lastOutput = outMax;
   else if(lastOutput < outMin) lastOutput = outMin;
}

void PIDBias::SetBurst(double output)
{
    burstOutput = output;
}

void PIDBias::SetSumOutputI(double sumOutputI_set)
{
    sumOutputI = convertUtilisationToOutput(sumOutputI_set);
}

void PIDBias::SetFilterSumOutputI(double filterSumOutputI_set)
{
    filterSumOutputI = convertUtilisationToOutput(filterSumOutputI_set);
}

void PIDBias::SetSteadyPowerDefault(double steadyPowerDefault_set)
{
    steadyPowerDefault = convertUtilisationToOutput(steadyPowerDefault_set);
}

void PIDBias::SetSteadyPowerOffset(double steadyPowerOffset_set)
{
    steadyPowerOffset = convertUtilisationToOutput(steadyPowerOffset_set);
}

void PIDBias::SetAutoTune(boolean steadyPowerAutoTune_set)
{
    steadyPowerAutoTune = steadyPowerAutoTune_set;
}

double PIDBias::GetKp() { return dispKp;}
double PIDBias::GetKi() { return dispKi;}
double PIDBias::GetKd() { return dispKd;}
double PIDBias::GetOutputP() { return outputP;}
double PIDBias::GetOutputI() { return outputI;}
double PIDBias::GetSumOutputI() { return sumOutputI;}
double PIDBias::GetOutputD() { return outputD;} 
double PIDBias::GetLastOutput() { return lastOutput;} 
int PIDBias::GetMode() { return  inAuto ? AUTOMATIC : MANUAL;}
