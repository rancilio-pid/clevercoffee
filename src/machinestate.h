
/********************************************************
   machinestatevoid
******************************************************/

void machinestatevoid()
{
  switch (machinestate)
  {
    // init
    case kInit:
      if (Input < (BrewSetPoint-1) || Input < 150 ) // Prevent coldstart leave by Input 222
      {
        machinestate = kColdStart;
        Serial.println(Input);
        Serial.println(machinestate);
        // some user have 100 % Output in kInit / Koldstart, reset PID 
        pidMode = 0;
        bPID.SetMode(pidMode);
        Output = 0 ;
        digitalWrite(pinRelayHeater, LOW); //Stop heating
        // start PID
        pidMode = 1;
        bPID.SetMode(pidMode);
      }

      if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
     if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kColdStart:
      switch (machinestatecold)
      // one high Input let the state jump to 19.
      // switch (machinestatecold) prevent it, we wait 10 sec with new state.
      // during the 10 sec the Input has to be Input >= (BrewSetPoint-1),
      // If not, reset machinestatecold
      {
        case 0:
          if (Input >= (BrewSetPoint-1) && Input < 150 )
          {
            machinestatecoldmillis = millis(); // get millis for interval calc
            machinestatecold = 10 ; // new state
            Serial.println("Input >= (BrewSetPoint-1), wait 10 sec before machinestate 19");

          }
          break;
        case 10:
          if (Input < (BrewSetPoint-1))
          {
            machinestatecold = 0 ;//  Input was only one time above BrewSetPoint, reset machinestatecold
            Serial.println("Reset timer for machinestate 19: Input < (BrewSetPoint-1)");
          }
          if (machinestatecoldmillis+10*1000 < millis() ) // 10 sec Input above BrewSetPoint, no set new state
          {
            machinestate = kSetPointNegative ;
            Serial.println("10 sec Input >= (BrewSetPoint-1) finished, switch to state 19");
          }
          break;
      }
      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

      if
      (
       (bezugsZeit > 0 && ONLYPID == 1) || // Bezugszeit bei Only PID
       (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)
      )

      {
        machinestate = kBrew;
      }

      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

      if (backflushON || backflushState > 10)
      {
        machinestate = kBackflush;
      }

     if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
     if(sensorError)
      {
        machinestate = kSensorError;
      }
      break;
      // Setpoint -1 Celsius
      case kSetPointNegative:
      brewdetection();  //if brew detected, set PID values
      if (Input >= (BrewSetPoint))
      {
        machinestate = kPidNormal;
      }
      if
      (
       (bezugsZeit > 0 && ONLYPID == 1) || // Bezugszeit bei Only PID
       (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)
      )
      {
        machinestate = kBrew;
      }
      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

      if (backflushON || backflushState > 10)
      {
        machinestate = kBackflush;
      }

      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

     if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
     if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kPidNormal:
      brewdetection();  //if brew detected, set PID values
      if
      (
       (bezugsZeit > 0 && ONLYPID == 1) || // Bezugszeit bei Only PID
       (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)
      )
      {
        machinestate = kBrew;
      }
      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

      if (backflushON || backflushState > 10)
      {
        machinestate = kBackflush;
      }
      if (emergencyStop)
      {
        machinestate = kEmergencyStop;
      }
     if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
     if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kBrew:
      brewdetection();
      // Ausgabe waehrend des Bezugs von Bruehzeit, Temp und heatrateaverage
      if (logbrew.check())
          Serial.printf("(tB,T,hra) --> %5.2f %6.2f %8.2f\n",(double)(millis() - startZeit)/1000,Input,heatrateaverage);
      if
      (
       (bezugsZeit > 35*1000 && Brewdetection == 1 && ONLYPID == 1  ) ||  // 35 sec later and BD PID active SW Solution
       (bezugsZeit == 0      && Brewdetection == 3 && ONLYPID == 1  ) ||  // Voltagesensor reset bezugsZeit == 0
       ((brewcounter == 10 || brewcounter == 43)   && ONLYPID == 0  ) // After brew
      )
      {
       if ((ONLYPID == 1 && Brewdetection == 3) || ONLYPID == 0 ) // only delay of shotimer for voltagesensor or brewcounter
       {
         machinestate = kShotTimerAfterBrew ;
         lastbezugszeitMillis = millis() ; // for delay

       }
       if (ONLYPID == 1 && Brewdetection == 1 && timerBrewdetection == 1) //direct to PID BD
       {
         machinestate = kBrewDetectionTrailing ;
       }
      }
      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

      if (emergencyStop)
      {
        machinestate = kEmergencyStop;
      }
     if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
     if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kShotTimerAfterBrew: //lastbezugszeitMillis
    brewdetection();
      if ( millis()-lastbezugszeitMillis > BREWSWITCHDELAY )
      {
       Serial.printf("Bezugsdauer: %4.1f s\n",lastbezugszeit/1000);
       machinestate = kBrewDetectionTrailing ;
       lastbezugszeit = 0 ;
      }

      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

     if (backflushON || backflushState > 10)
      {
        machinestate = kBackflush;
      }

      if (emergencyStop)
      {
        machinestate = kEmergencyStop;
      }

     if (pidON == 0)
      {
        machinestate = kPidOffline;
      }

     if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kBrewDetectionTrailing:
      brewdetection();
      if (timerBrewdetection == 0)
      {
        machinestate = kPidNormal;
      }
      if
      (
       (bezugsZeit > 0 && ONLYPID == 1  && Brewdetection == 3) || // New Brew inner BD only by Only PID AND Voltage Sensor
       (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42)
      )
      {
        machinestate = kBrew;
      }

      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

      if (backflushON || backflushState > 10)
      {
        machinestate = kBackflush;
      }

      if (emergencyStop)
      {
        machinestate = kEmergencyStop;
      }
      if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
     if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kSteam:
      if (SteamON == 0)
      {
        machinestate = kCoolDown;
      }

       if (emergencyStop)
      {
        machinestate = kEmergencyStop;
      }

      if (backflushON || backflushState > 10)
      {
        machinestate = kBackflush;
      }

      if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
      if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kCoolDown:
    if (Brewdetection == 2 || Brewdetection == 3)
      {
        /*
          Bei QuickMill Dampferkennung nur ueber Bezugsschalter moeglich, durch Aufruf von
          brewdetection() kann neuer Dampfbezug erkannt werden
          */
        brewdetection();
      }
      if (Brewdetection == 1 && ONLYPID == 1)
      {
        // Ab lokalen Minumum wieder freigeben für state 20, dann wird bist Solltemp geheizt.
         if (heatrateaverage > 0 && Input < BrewSetPoint + 2)
         {
            machinestate = kPidNormal;
         }
      }
      if ((Brewdetection == 3 || Brewdetection == 2) && Input < BrewSetPoint + 2)
      {
        machinestate = kPidNormal;
      }

      if (SteamON == 1)
      {
        machinestate = kSteam;
      }

      if (backflushON || backflushState > 10)
      {
        machinestate = kBackflush;
      }

      if (emergencyStop)
      {
        machinestate = kEmergencyStop;
      }
      if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
      if(sensorError)
      {
        machinestate = kSensorError;
      }
    break;

    case kBackflush:
      if (backflushON == 0)
       {
         machinestate = kPidNormal;
       }

      if (emergencyStop)
       {
         machinestate = kEmergencyStop;
       }
      if (pidON == 0)
       {
         machinestate = kPidOffline;
       }
      if(sensorError)
       {
         machinestate = kSensorError;
       }
    break;

    case kEmergencyStop:
      if (!emergencyStop)
      {
        machinestate = kPidNormal;
      }
      if (pidON == 0)
      {
        machinestate = kPidOffline;
      }
      if(sensorError)
      {
        machinestate = kSensorError ;
      }
    break;

    case kPidOffline:
      if (pidON == 1)
      {
        if(kaltstart)
        {
          machinestate = kColdStart;
        }
        else if(!kaltstart && (Input > (BrewSetPoint-10) )) // Input higher BrewSetPoint-10, normal PID
        {
          machinestate = kPidNormal;
        }
        else if (Input <= (BrewSetPoint-10) )
        {
          machinestate = kColdStart; // Input 10C below set point, enter cold start
          kaltstart = true;
        }
      }

      if(sensorError)
      {
        machinestate = kSensorError ;
      }
    break;

    case kSensorError:
      machinestate = kSensorError ;
    break;

    case keepromError:
      machinestate = keepromError ;

    break;

  } // switch case

  if (machinestate != lastmachinestate) {
    Serial.printf("new machinestate: %i -> %i\n", lastmachinestate, machinestate);
    lastmachinestate = machinestate;
  }
} // end void