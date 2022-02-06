
/********************************************************
  CheckWeight
******************************************************/
#if (BREWMODE == 2 || ONLYPIDSCALE == 1)
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
    #if (DISPLAY != 0)
      u8g2.clearBuffer();
      u8g2.drawStr(0, 2, "Taring scale,");
      u8g2.drawStr(0, 12, "remove any load!");
      u8g2.drawStr(0, 22, "....");
      delay(2000);
      u8g2.sendBuffer();
    #endif    
    LoadCell.start(stabilizingtime, _tare);
    if (LoadCell.getTareTimeoutFlag()) {
      DEBUG_println(F("Timeout, check MCU>HX711 wiring and pin designations"));
      #if (DISPLAY != 0)
        u8g2.drawStr(0, 32, "failed!");
        u8g2.drawStr(0, 42, "Scale not working...");    // scale timeout will most likely trigger after OTA update, but will still work after boot
        delay(5000);
        u8g2.sendBuffer();
      #endif
    }
    else {
      DEBUG_println(F("done"));
      #if (DISPLAY != 0)
        u8g2.drawStr(0, 32, "done.");
        u8g2.sendBuffer();
      #endif
    }
    LoadCell.setCalFactor(calibrationValue); // set calibration factor (float)
  }

  void shottimerscale()
  {
      switch (shottimercounter) 
      {
        case 10:    // waiting step for brew switch turning on
          if (bezugsZeit > 0) 
          {
            weightPreBrew = weight;
            shottimercounter = 20  ;
          }
        break;
        case 20:   
        weightBrew = weight - weightPreBrew;
        if (bezugsZeit == 0) 
        {
          shottimercounter = 10;
        }
        break;
      }
  }
#endif
