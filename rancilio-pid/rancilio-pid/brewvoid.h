

/********************************************************
    Digtalswitch OR Read analog input pin for BREW SWITCH
******************************************************/

void checkbrewswitch()
{
  #if BREWSWITCHTYPE == 1
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
  #endif
  #if BREWSWITCHTYPE == 2 // TRIGGER
    
    #if (PINBREWSWITCH > 0) 

        int reading = digitalRead(PINBREWSWITCH);

        if (reading != brewswitchTrigger) {
          // reset the debouncing timer
          lastDebounceTimeBrewTrigger = millis();
        }
        if ((millis() - lastDebounceTimeBrewTrigger) > debounceDelayBrewTrigger) {
          // whatever the reading is at, it's been there for longer than the debounce
          // delay, so take it as the actual current state:

          // if the button state has changed:
          if (reading != buttonStateBrewTrigger) {
            buttonStateBrewTrigger = reading;
          }
        }
      brewswitchTrigger = reading;  
      Serial.println(brewswitchTrigger);
    #endif
      // Digital Analog
     #if (PINBREWSWITCH == 0)
        unsigned long currentMillistemp = millis();
        if (currentMillistemp - previousMillistempanalogreading >= analogreadingtimeinterval)
        {
          //DEBUG_println(analogRead(analogPin));
          previousMillistempanalogreading = currentMillistemp;
          if (filter(analogRead(analogPin)) > 1000 )
          {
            brewswitchTrigger = HIGH ; 
          }
          if (filter(analogRead(analogPin)) < 1000 )
          {
            brewswitchTrigger = LOW ;
          }
        }
    #endif
        // Triggersignal umsetzen in brewswitch
    switch(brewswitchTriggerCase) 
    {
      case 10:
        if (brewswitchTrigger == HIGH)
        {
          brewswitchTriggermillis = millis() ; 
          brewswitchTriggerCase = 20 ; 
          DEBUG_println("brewswitchTriggerCase 10:  HIGH");
        }
      break;
      case 20: 
        // only one push, brew
        if (brewswitchTrigger == LOW)
        { 
          // Brew 
          brewswitch = HIGH  ;
          brewswitchTriggerCase = 30 ;
          DEBUG_println("brewswitchTriggerCase 20: Brew Trigger");
        }
        // Button one 1sec pushed
        if (brewswitchTrigger == HIGH && (brewswitchTriggermillis+1000 <= millis() ))
        {
          // DO something
           DEBUG_println("brewswitchTriggerCase 20: XXX Trigger");
          brewswitchTriggerCase = 30 ;
        }
      break ;
      case 30:
        // Stop brewing
        if (brewswitchTrigger == HIGH && brewswitch == LOW)
        {
          brewswitchTriggerCase = 40 ; 
          brewswitchTriggermillis = millis() ;     
          DEBUG_println("brewswitchTriggerCase 30: XXX Trigger LOW");
        }
        if (brewswitchTrigger == HIGH && brewswitch == HIGH)
        {
          brewswitch = LOW  ;
          brewswitchTriggerCase = 40 ; 
          brewswitchTriggermillis = millis() ; 
          DEBUG_println("brewswitchTriggerCase 30: Brew Trigger LOW");
        }
      break ;
      case 40:
        // wait 5 Sec until next brew, detection  
        if (brewswitchTriggermillis+5000 <= millis() )
        {
          brewswitchTriggerCase = 10 ; 
           DEBUG_println("brewswitchTriggerCase 40: Brew Trigger Next Loop");
        }
      break ;
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
if (backflushON || backflushState > 10) 
  {
    if (backflushState == 43) {
      #if DISPLAY != 0
        displayMessage(langstring_bckffinished[0], langstring_bckffinished[1], "", "", "", "");
      #endif 
    } else if (backflushState == 10) {
      #if DISPLAY != 0
        displayMessage(langstring_bckfactivated[0], langstring_bckfactivated[1], "", "", "", "");
      #endif
    } else if ( backflushState > 10) {
      #if DISPLAY != 0
        displayMessage(langstring_bckfrunning[0], String(flushCycles), langstring_bckfrunning[1], String(maxflushCycles), "", "");
      #endif
    }
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
      DEBUG_println("Brew stopped manually");
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
