/********************************************************
    PreInfusion, Brew , if not Only PID
******************************************************/

#if (BREWMODE == 1) // old Brew MODE 
void brew() 
{
  if (OnlyPID == 0) 
  {
    readAnalogInput();
    unsigned long currentMillistemp = millis();

    if (brewswitch < 1000 && brewcounter > 10)
    {
      //abort function for state machine from every state
      brewcounter = 43;
    }

    if (brewcounter > 10 && brewcounter < 43 ) {
      bezugsZeit = currentMillistemp - startZeit;
    }
    if (brewswitch < 1000 && firstreading == 0 ) 
    {   //check if brewswitch was turned off at least once, last time,
      brewswitchWasOFF = true;
      //DEBUG_println("brewswitch value")
      //DEBUG_println(brewswitch)
    }

    totalbrewtime = preinfusion + preinfusionpause + brewtime;    // running every cycle, in case changes are done during brew

    // state machine for brew
    switch (brewcounter) {
      case 10:    // waiting step for brew switch turning on
        if (brewswitch > 1000 && backflushState == 10 && backflushON == 0 && brewswitchWasOFF) {
          startZeit = millis();
          brewcounter = 20;
          lastbezugszeit = 0;
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
        if (brewswitch < 1000) {
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

/********************************************************
  CheckWeight
******************************************************/

void checkWeight() {
  static boolean newDataReady = 0;
  unsigned long currentMillisScale = millis();
  if (scaleFailure) {   // abort if scale is not working
    return;
  }
  if (currentMillisScale - previousMillisScale >= intervalWeight)
  {
    previousMillisScale = currentMillisScale;

    // check for new data/start next conversion:
    if (LoadCell.update()) {
      newDataReady = true;
    }

    // get smoothed value from the dataset:
    if (newDataReady) {
      weight = LoadCell.getData();
      newDataReady = 0;
    }
  }
}

/********************************************************
   Initialize scale
******************************************************/
void initScale() {
  LoadCell.begin();
  long stabilizingtime = 2000; // tare preciscion can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  DEBUG_print(F("INIT: Initializing scale ... "));
  u8g2.clearBuffer();
  u8g2.drawStr(0, 2, "Taring scale,");
  u8g2.drawStr(0, 12, "remove any load!");
  u8g2.drawStr(0, 22, "....");
  u8g2.sendBuffer();
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    DEBUG_println(F("Timeout, check MCU>HX711 wiring and pin designations"));
    u8g2.drawStr(0, 32, "failed!");
    u8g2.drawStr(0, 42, "Scale not working...");    // scale timeout will most likely trigger after OTA update, but will still work after boot
    delay(5000);
    u8g2.sendBuffer();
  }
  else {
    DEBUG_println(F("done"));
    u8g2.drawStr(0, 32, "done.");
    u8g2.sendBuffer();
  }
  LoadCell.setCalFactor(calibrationValue); // set calibration factor (float)
}


void brew()
{
  if (OnlyPID == 0)
  {
    readAnalogInput();
    unsigned long currentMillistemp = millis();

    if (brewswitch < 1000 && brewcounter > 10)
    {
      //abort function for state machine from every state
      brewcounter = 43;
    }

    if (brewcounter > 10) {
      bezugsZeit = currentMillistemp - startZeit;
      weightBrew = weight - weightPreBrew;

    }
    if (brewswitch < 1000 && firstreading == 0 )
    { //check if brewswitch was turned off at least once, last time,
      brewswitchWasOFF = true;
      //DEBUG_println("brewswitch value")
      //DEBUG_println(brewswitch)
    }

    totalbrewtime = preinfusion + preinfusionpause + brewtime;    // running every cycle, in case changes are done during brew

    // state machine for brew
    switch (brewcounter) {
      case 10:    // waiting step for brew switch turning on
        if (brewswitch > 1000 && backflushState == 10 && backflushON == 0 && brewswitchWasOFF) {
          startZeit = millis();
          brewcounter = 20;
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
        if (brewswitch < 1000) {
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