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

int PIDBias::signnum_c(double x) {
  if (x >= 0.0) return 1;
  if (x < 0.0) return -1;
}

PIDBias::PIDBias(double* Input, double* Output, double* steadyPower, double* steadyPowerOffset, unsigned long* steadyPowerOffset_Activated, int* steadyPowerOffset_Time, double* Setpoint,
        double Kp, double Ki, double Kd)
{
    myOutput = Output;
    myInput = Input;
    mySetpoint = Setpoint;
    mySteadyPower = steadyPower;
    mySteadyPowerOffset = steadyPowerOffset;
    mySteadyPowerOffset_Activated = steadyPowerOffset_Activated;
    mySteadyPowerOffset_Time = steadyPowerOffset_Time;
    inAuto = MANUAL;
    lastOutput = *myOutput;
    steadyPowerDefault = *mySteadyPower;
    //steadyPowerOffset = 0;
    PIDBias::SetOutputLimits(0, 5000);
    SampleTime = 5000;
    PIDBias::SetTunings(Kp, Ki, Kd);
    lastTime = millis();
}

int PIDBias::Compute()
{
   unsigned long now = millis();
   unsigned long timeChange = (now - lastTime);
   if(timeChange >= SampleTime) {
      if(!inAuto) {
        lastTime = now;
        return 2;  // compute should run, but PID is disabled
      }
      steadyPowerOffsetCalculated = PIDBias::CalculateSteadyPowerOffset();
      const double setPointBand = 0.1;
      
      lastOutput = *myOutput;
      double input = *myInput;
      double error = *mySetpoint - input;
      double pastChange = pastTemperatureChange(10) / 2;  // difference of the last 10 seconds scaled down to one compute() cycle (=5 seconds).

      outputP = kp * error;
      outputI = ki * error;
      // outputD = -kd * (input - 2*lastInput + lastLastInput);
      outputD = -kd * pastChange;

      sumOutputI += outputI;

      //Filter too high sumOutputI
      if ( sumOutputI > filterSumOutputI ) sumOutputI = filterSumOutputI;

      //reset sumI when at setPoint. This improves stabilization.
      if ( signnum_c(error) * signnum_c(lastError) < 0 ) { //temperature curve crosses setPoint
        //DEBUG_print("Crossing setPoint\n");
        
        //moving upwards
        if ( sumOutputI > 0 ) {
          //steadyPower auto-tuning
           if ( steadyPowerAutoTune && pastChange < 0.06 && sumOutputI < filterSumOutputI ) {  //TODO: <= 0.03 better?
            if (steadyPowerOffsetCalculated >= 0.2) {
              DEBUG_print("Attention: steadyPowerOffset is probably too high (%0.2f -= %0.2f) (moving up at setPoint)\n", steadyPowerOffsetCalculated, 0.2);
              *mySteadyPowerOffset -= 0.2;
              //TODO: perhaps we need to reduce steadyPowerOffset in eeprom (permanently)
           } else if ( convertOutputToUtilisation(sumOutputI) >= 0.5 && pastTemperatureChange(20) >=0 && pastTemperatureChange(20) <= 0.1) {
              DEBUG_print("Auto-Tune steadyPower(%0.2f += %0.2f) (moving up with too much outputI)\n", *mySteadyPower, 0.15); //convertOutputToUtilisation(sumOutputI / 2));
              *mySteadyPower += 0.15; // convertOutputToUtilisation(sumOutputI / 2);
              //TODO: remember sumOutputI and restore it back to 1/2 when moving down later. only when we are moving up <0.2
           } else if ( convertOutputToUtilisation(sumOutputI) >= 0.3 && pastTemperatureChange(20) >=0 && pastTemperatureChange(20) <= 0.1) {
              DEBUG_print("Auto-Tune steadyPower(%0.2f += %0.2f) (moving up with too much outputI)\n", *mySteadyPower, 0.1); //convertOutputToUtilisation(sumOutputI / 2));
              *mySteadyPower += 0.1; // convertOutputToUtilisation(sumOutputI / 2);
              //TODO: remember sumOutputI and restore it back to 1/2 when moving down later. only when we are moving up <0.2
            }
           }

          /*
          if ( steadyPowerAutoTune && pastChange <= 0.3 && convertOutputToUtilisation(sumOutputI) >= 0.3 && sumOutputI < filterSumOutputI ) {
            DEBUG_print("Auto-Tune steadyPower(%0.2f += %0.2f) (moving up with too much outputI)\n", *mySteadyPower, 0.1); //convertOutputToUtilisation(sumOutputI / 2));
            *mySteadyPower += 0.1; // convertOutputToUtilisation(sumOutputI / 2);
            //TODO: remember sumOutputI and restore it back to 1/2 when moving down later. only when we are moving up <0.2
          }
          */
        }
        
        //moving downwards
        if ( steadyPowerAutoTune && pastChange <= -0.01 && steadyPowerOffsetCalculated >= 0.3 ) {
          DEBUG_print("Auto-Tune steadyPower(%0.2f += %0.2f) (moving down too fast, steadyPowerOffset=%0.2f)\n", *mySteadyPower, 0.2, steadyPowerOffsetCalculated); //convertOutputToUtilisation(sumOutputI / 2));
          *mySteadyPower += 0.2;
        } else if ( steadyPowerAutoTune && pastChange <= -0.01 && pastChange > -0.4 ) {
          //steadyPower auto-tuning
          DEBUG_print("Auto-Tune steadyPower(%0.2f += %0.2f) (moving down too fast)\n", *mySteadyPower, 0.1); //convertOutputToUtilisation(sumOutputI / 2));
          *mySteadyPower += 0.1;
        }
        
        //DEBUG_print("Set sumOutputI=0 to stabilize(%0.2f)\n", convertOutputToUtilisation(sumOutputI));
        sumOutputI = 0;
      }
      
      //above target band (setPoint_band) and temp_changes too fast
      if ( steadyPowerAutoTune && error < -setPointBand && (millis() - lastTrigger >30000) ) {
        //steady temp
        if ( fabs(pastTemperatureChange(20)) <= 0.01 ) {
          if (steadyPowerOffsetCalculated >= 0.1) {
            //DEBUG_print("Attention: steadyPowerOffset is probably too high (%0.2f -= %0.2f) (barely moving)\n", steadyPowerOffsetCalculated, 0.1);
            //*mySteadyPowerOffset -= 0.1;
          } else {
            //steadyPower auto-tuning
            DEBUG_print("Auto-Tune steadyPower(%0.2f -= %0.2f) (barely moving)\n", *mySteadyPower, 0.4);
            *mySteadyPower -= 0.4;
          }
          lastTrigger = millis();
        }
        //going up
        else if ( pastTemperatureChange(20) > 0 && pastTemperatureChange(20) <= 0.1 ) {
          if (steadyPowerOffsetCalculated >= 0.2) {
            DEBUG_print("Attention: steadyPowerOffset is probably too high (%0.2f -= %0.2f) (moving up at setPointBand)\n", steadyPowerOffsetCalculated, 0.3);
            *mySteadyPowerOffset -= 0.3;
            //TODO: perhaps we need to reduce steadyPowerOffset in eeprom (permanently)
          } else {
            //steadyPower auto-tuning
            DEBUG_print("Auto-Tune steadyPower(%0.2f -= %0.2f) (moving up too much)\n", *mySteadyPower, 0.3);
            *mySteadyPower -= 0.3;
          }
          lastTrigger = millis();
        } 
      }
      
      //below target band and temp not going up fast enough 
      if ( steadyPowerAutoTune && error > setPointBand && pastTemperatureChange(20) <= 0.1 && pastTemperatureChange(20) >= 0 &&
                sumOutputI == filterSumOutputI && (millis() - lastTrigger >20000) ) {
        //steadyPower auto-tuning
        double offset = 0.1;
        if (error > 1) offset = 0.15;
        if (steadyPowerOffsetCalculated >= 0.2) {
          offset *= 2;
        }
        DEBUG_print("Auto-Tune steadyPower(%0.2f += %0.2f) (not moving. either Kp, steadyPowerOffset or steadyPower too low)\n", *mySteadyPower, offset);
        *mySteadyPower += offset;
        lastTrigger = millis();
      }

      //Auto-tune should never increase steadyPower too much (this prevents bugs in not thought of cases)
      if ( steadyPowerAutoTune && (*mySteadyPower > steadyPowerDefault * 1.5 || *mySteadyPower < steadyPowerDefault * 0.5) ) {
        DEBUG_print("Auto-Tune steadyPower(%0.2f) is off too far. Set back to %0.2f\n", *mySteadyPower, steadyPowerDefault);
        *mySteadyPower = steadyPowerDefault; 
      }
      if ( steadyPowerAutoTune && *mySteadyPower > 10 ) {
        DEBUG_print("Auto-Tune steadyPower(%0.2f) is by far too high. Set back to %0.2f\n", *mySteadyPower, 4.8);
        *mySteadyPower = 4.8; 
      }

      //TODO safe-guard against always moving up because of steadyPower (eg no steadyPower above error < 5)

      //If we are above setpoint, we dont want to overly reduce output when temperature is moving downwards
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
        + convertUtilisationToOutput(*mySteadyPower) + convertUtilisationToOutput(steadyPowerOffsetCalculated) // convertUtilisationToOutput(*mySteadyPowerOffset) // add steadyPower (bias) always to output
        + outputP
        + sumOutputI
        + outputD;

      // if we in upper-side of side-band, (nearly) not moving at all due to stable temp but yet not near setpoint, manipulate output once
      if ( error <= -setPointBand && 
           fabs(pastTemperatureChange(20)) <= 0.1 &&
           millis() - lastTrigger2 >30000 ) {
        double factor = 1;
        if ( error <= -0.2 ) {
          factor = 0;
        } else if ( error <= -0.1 ) {
          factor = 0.1;
        }
        if (factor != 1) {
          DEBUG_print("Overwrite output (%0.2f *= %0.2f) (not moving)\n", convertOutputToUtilisation(output), factor);
          output *= factor;
          lastTrigger2 = millis();
        }
      }
        
      //temporary overwrite output when burst is triggered OR overwrite should happen
      if ( burstOutput > 0 ) {
        output = burstOutput;
        burstOutput = 0;
      }

      if (output > outMax) output = outMax;
      else if(output < outMin) output = outMin;
      *myOutput = output;
      lastError = error;
      lastTime = now;
      /*
      DEBUG_print("Input=%6.2f | error=%5.2f delta=%5.2f | Output=%6.2f = b:%5.2f + p:%5.2f + i:%5.2f(%5.2f) + d:%5.2f -\n", 
        *myInput,
        (*mySetpoint - *myInput),
        pastTemperatureChange(10)/2,
        convertOutputToUtilisation(output),
        *mySteadyPower + GetSteadyPowerOffsetCalculated(),
        convertOutputToUtilisation(GetOutputP()),
        convertOutputToUtilisation(GetSumOutputI()),
        convertOutputToUtilisation(GetOutputI()),
        convertOutputToUtilisation(GetOutputD())
        );
      */
      return 1;  //compute did run
   } else {
    return 0;  //compute shall not run yet
   }
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
   lastOutput = *myOutput;
   burstOutput = 0;
   sumOutputD = 0;
   sumOutputI = 0;
   lastTrigger = 0;
   lastTrigger2 = 0;
   steadyPowerAutoTune = true;
   steadyPowerDefault = *mySteadyPower;
   steadyPowerOffsetCalculated = *mySteadyPowerOffset;
   filterSumOutputI = outMax;
   lastTime = millis();
   if(*myOutput > outMax) *myOutput = outMax;
   else if(*myOutput < outMin) *myOutput = outMin;
   if(lastOutput > outMax) lastOutput = outMax;
   else if(lastOutput < outMin) lastOutput = outMin;
}

void PIDBias::SetBurst(double output)
{
    burstOutput = convertUtilisationToOutput(output);
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

//void PIDBias::SetSteadyPowerOffset(double steadyPowerOffset_set)
//{
//    steadyPowerOffset = convertUtilisationToOutput(steadyPowerOffset_set);
//}

void PIDBias::SetAutoTune(boolean steadyPowerAutoTune_set)
{
    steadyPowerAutoTune = steadyPowerAutoTune_set;
}

/*
void PIDBias::UpdateSteadyPowerOffset(unsigned long steadyPowerOffset_Activated_in, unsigned long steadyPowerOffset_Time) {
  if (steadyPowerOffset_Activated_in == 0 || steadyPowerOffset_Time <= 0) {
    //bPID.SetSteadyPowerOffset(0);
    *mySteadyPowerOffset = 0;
    return;
  }
  unsigned long diff = millis() - steadyPowerOffset_Activated_in;
  double steadyPowerOffsetPerMillisecond = *mySteadyPowerOffset / steadyPowerOffset_Time;
  *mySteadyPowerOffset -= (diff * steadyPowerOffsetPerMillisecond);
  if (*mySteadyPowerOffset < 0) {
    *mySteadyPowerOffset = 0;
  }
}
*/

double PIDBias::CalculateSteadyPowerOffset() {
  unsigned long diff = millis() - *mySteadyPowerOffset_Activated;
  if (*mySteadyPowerOffset_Activated == 0 || *mySteadyPowerOffset_Time <= 0 || 
     (*mySteadyPowerOffset_Activated > 0) && (diff >= *mySteadyPowerOffset_Time*1000)) {
    return 0;
  }
  double steadyPowerOffsetPerMillisecond = *mySteadyPowerOffset / (*mySteadyPowerOffset_Time*1000);
  double steadyPowerOffsetCalculated = *mySteadyPowerOffset - (diff * steadyPowerOffsetPerMillisecond);
  if (steadyPowerOffsetCalculated < 0) {
    return 0;
  } else {
    //DEBUG_print("steadyPowerOffset=%0.2f\n", steadyPowerOffsetCalculated);  //TODO remove
    return steadyPowerOffsetCalculated;
  }
}


double PIDBias::GetKp() { return dispKp;}
double PIDBias::GetKi() { return dispKi;}
double PIDBias::GetKd() { return dispKd;}
double PIDBias::GetOutputP() { return outputP;}
double PIDBias::GetOutputI() { return outputI;}
double PIDBias::GetSumOutputI() { return sumOutputI;}
double PIDBias::GetFilterSumOutputI() { return filterSumOutputI; }
double PIDBias::GetOutputD() { return outputD;} 
double PIDBias::GetLastOutput() { return lastOutput;} 
int PIDBias::GetMode() { return  inAuto ? AUTOMATIC : MANUAL;}
double PIDBias::GetSteadyPowerOffset() { return *mySteadyPowerOffset;}
double PIDBias::GetSteadyPowerOffsetCalculated() { return steadyPowerOffsetCalculated; }
