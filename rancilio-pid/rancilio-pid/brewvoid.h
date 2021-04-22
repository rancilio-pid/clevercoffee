

/********************************************************
    Digtalswitch OR Read analog input pin for BREW SWITCH
******************************************************/

void checkbrewswitch()
{
  #if (PINBREWSWITCH > 0)
    // Digital GIPO
    brewswitch = digitalRead(PINBREWSWITCH);
  #endif
  // Digital Analog
  #if (PINBREWSWITCH == 0)
    unsigned long currentMillistemp = millis();
    if (currentMillistemp - previousMillistempanalogreading >= analogreadingtimeinterval)
    {
      previousMillistempanalogreading = currentMillistemp;
      if (filter(analogRead(analogPin)) > 1000 )
      {
        brewswitch = HIGH ; 
      }
      if (filter(analogRead(analogPin)) < 1000 )
      {
        brewswitch = LOW ;
      }
    }
  #endif
}
/********************************************************
   BACKFLUSH
******************************************************/


void backflush() {
  if (backflushState != 10 && backflushON == 0) {
    backflushState = 43;    // force reset in case backflushON is reset during backflush!
  } else if ( Offlinemodus == 1 || brewcounter > 10 || maxflushCycles <= 0 || backflushON == 0) {
    return;
  }

  if (pidMode == 1) { //Deactivate PID
    pidMode = 0;
    bPID.SetMode(pidMode);
    Output = 0 ;
  }
  digitalWrite(pinRelayHeater, LOW); //Stop heating
  
  checkbrewswitch() ;
  unsigned long currentMillistemp = millis();

  if (brewswitch == LOW && backflushState > 10) {   //abort function for state machine from every state
    backflushState = 43;
  }

  // state machine for brew
  switch (backflushState) {
    case 10:    // waiting step for brew switch turning on
      if (brewswitch == HIGH && backflushON) {
        startZeit = millis();
        backflushState = 20;
      }
      break;
    case 20:    //portafilter filling
      DEBUG_println("portafilter filling");
      digitalWrite(pinRelayVentil, relayON);
      digitalWrite(pinRelayPumpe, relayON);
      backflushState = 21;
      break;
    case 21:    //waiting time for portafilter filling
      if (millis() - startZeit > FILLTIME) {
        startZeit = millis();
        backflushState = 30;
      }
      break;
    case 30:    //flushing
      DEBUG_println("flushing");
      digitalWrite(pinRelayVentil, relayOFF);
      digitalWrite(pinRelayPumpe, relayOFF);
      flushCycles++;
      backflushState = 31;
      break;
    case 31:    //waiting time for flushing
      if (millis() - startZeit > flushTime && flushCycles < maxflushCycles) {
        startZeit = millis();
        backflushState = 20;
      } else if (flushCycles >= maxflushCycles) {
        backflushState = 43;
      }
      break;
    case 43:    // waiting for brewswitch off position
      if (brewswitch == LOW) {
        DEBUG_println("backflush finished");
        digitalWrite(pinRelayVentil, relayOFF);
        digitalWrite(pinRelayPumpe, relayOFF);
        currentMillistemp = 0;
        flushCycles = 0;
        backflushState = 10;
      }
      break;
  }
}



/********************************************************
    PreInfusion, Brew Normal
******************************************************/
#if (BREWMODE == 1) // old Brew MODE 
void brew() 
{
  if (OnlyPID == 0) 
  {    
    unsigned long currentMillistemp = millis();
    checkbrewswitch() ;

    if (brewswitch == LOW && brewcounter > 10)
    {
      //abort function for state machine from every state
      brewcounter = 43;
    }

    if (brewcounter > 10 && brewcounter < 43 ) {
      bezugsZeit = currentMillistemp - startZeit;
    }
    if (brewswitch == LOW && firstreading == 0 ) 
    {   //check if brewswitch was turned off at least once, last time,
      brewswitchWasOFF = true;
      //DEBUG_println("brewswitch value")
      //DEBUG_println(brewswitch)
    }

    totalbrewtime = preinfusion + preinfusionpause + brewtime;    // running every cycle, in case changes are done during brew

    // state machine for brew
    switch (brewcounter) {
    case 10:    // waiting step for brew switch turning on
        if (brewswitch == HIGH && backflushState == 10 && backflushON == 0 && brewswitchWasOFF) {
          startZeit = millis();
          if (preinfusionpause == 0 || preinfusion == 0){
          brewcounter = 40;
          } else {
          brewcounter = 20;
          }
          kaltstart = false;    // force reset kaltstart if shot is pulled
          
        } else {
          backflush();
        }
        break;
      case 20:    //preinfusioon
        DEBUG_println("Preinfusion");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayON);
        brewcounter = 21;
        break;
      case 21:    //waiting time preinfusion
        if (bezugsZeit > preinfusion) {
          brewcounter = 30;
        }
        break;
      case 30:    //preinfusion pause
        DEBUG_println("preinfusion pause");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayOFF);
        brewcounter = 31;
        break;
      case 31:    //waiting time preinfusion pause
        if (bezugsZeit > preinfusion + preinfusionpause) {
          brewcounter = 40;
        }
        break;
      case 40:    //brew running
        DEBUG_println("Brew started");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayON);
        brewcounter = 41;
        break;
      case 41:    //waiting time brew
        lastbezugszeit = bezugsZeit;
        if (bezugsZeit > totalbrewtime) {
          brewcounter = 42;
        }
        break;
      case 42:    //brew finished
        DEBUG_println("Brew stopped");
        digitalWrite(pinRelayVentil, relayOFF);
        digitalWrite(pinRelayPumpe, relayOFF);
        brewcounter = 43;
        bezugsZeit = 0;
        break;
      case 43:    // waiting for brewswitch off position
        if (brewswitch == LOW) {
          digitalWrite(pinRelayVentil, relayOFF);
          digitalWrite(pinRelayPumpe, relayOFF);
          // lastbezugszeitMillis = millis();  // for shottimer delay after disarmed button
          currentMillistemp = 0;
          brewDetected = 0; //rearm brewdetection
          brewcounter = 10;
          bezugsZeit = 0;
        }
        break;
    }
  }
}
#endif

#if (BREWMODE == 2) // SCALE Brewmode
void brew()
{
  if (OnlyPID == 0)
  {
    checkbrewswitch() ;
    unsigned long currentMillistemp = millis();

    if (brewswitch == LOW && brewcounter > 10)
    {
      //abort function for state machine from every state
      brewcounter = 43;
    }

    if (brewcounter > 10 && brewcounter < 43 ) {
      bezugsZeit = currentMillistemp - startZeit;
      weightBrew = weight - weightPreBrew;

    }
    if (brewswitch ==  LOW && firstreading == 0 )
    { //check if brewswitch was turned off at least once, last time,
      brewswitchWasOFF = true;
      //DEBUG_println("brewswitch value")
      //DEBUG_println(brewswitch)
    }

    totalbrewtime = preinfusion + preinfusionpause + brewtime;    // running every cycle, in case changes are done during brew

    // state machine for brew
    switch (brewcounter) {
      case 10:    // waiting step for brew switch turning on
        if (brewswitch == HIGH && backflushState == 10 && backflushON == 0 && brewswitchWasOFF) {
          startZeit = millis();
          brewcounter = 20;
          if (preinfusionpause == 0 || preinfusion == 0)
          {
          brewcounter = 40;
          }
          kaltstart = false;    // force reset kaltstart if shot is pulled
          weightPreBrew = weight;
        } else {
          backflush();
        }
        break;
      case 20:    //preinfusioon
        DEBUG_println("Preinfusion");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayON);
        brewcounter = 21;
        break;
      case 21:    //waiting time preinfusion
        if (bezugsZeit > preinfusion) {
          brewcounter = 30;
        }
        break;
      case 30:    //preinfusion pause
        DEBUG_println("preinfusion pause");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayOFF);
        brewcounter = 31;
        break;
      case 31:    //waiting time preinfusion pause
        if (bezugsZeit > preinfusion + preinfusionpause) {
          brewcounter = 40;
        }
        break;
      case 40:    //brew running
        DEBUG_println("Brew started");
        digitalWrite(pinRelayVentil, relayON);
        digitalWrite(pinRelayPumpe, relayON);
        brewcounter = 41;
        break;
      case 41:    //waiting time brew
        if (bezugsZeit > totalbrewtime || (weightBrew > (weightSetpoint - scaleDelayValue))) {
          brewcounter = 42;
        }
        if (bezugsZeit > totalbrewtime) {
          brewcounter = 42;
        }
        break;
        case 42:    //brew finished
        DEBUG_println("Brew stopped");
        digitalWrite(pinRelayVentil, relayOFF);
        digitalWrite(pinRelayPumpe, relayOFF);
        brewcounter = 43;
        break;
        case 43:    // waiting for brewswitch off position
        if (brewswitch == LOW) {
          digitalWrite(pinRelayVentil, relayOFF);
          digitalWrite(pinRelayPumpe, relayOFF);
          //bezugszeit_last_Millis = millis();  // for shottimer delay after disarmed button
          //bezugsZeitAlt = bezugsZeit;
          currentMillistemp = 0;
          bezugsZeit = 0;
          brewDetected = 0; //rearm brewdetection
          brewcounter = 10;
        }
        weightBrew = weight - weightPreBrew;  // always calculate weight to show on display
        break;
    }
  }
}
#endif
