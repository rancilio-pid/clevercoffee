/********************************************************
    send data to display
******************************************************/
void printScreen() 
{

  unsigned long currentMillisDisplay = millis();
  if
  (
   (currentMillisDisplay - previousMillisDisplay >= intervalDisplay) &&
   (machinestate == 19 || machinestate == 20 || machinestate == 35) ||
   ((machinestate == 30 || machinestate == 31)  && SHOTTIMER == 0) // shottimer == 0, auch Bezug anzeigen
  ) 
   {
    previousMillisDisplay = currentMillisDisplay;
    if (!sensorError) {
      u8g2.clearBuffer();
      //draw outline frame
      //u8g2.drawFrame(0, 0, 128, 64);
     

      //draw (blinking) temp
    if (fabs(Input - setPoint) < 0.3) {
    if (isrCounter < 500) {
      if (Input < 99.999) {
        u8g2.setCursor(13, 12);
        u8g2.setFont(u8g2_font_fub35_tf);
        u8g2.print(Input,1);
      }
      else {
        u8g2.setCursor(-1, 12);
        u8g2.setFont(u8g2_font_fub35_tf);
        u8g2.print(Input,1);
      }
    }
  } else {
    if (Input < 99.999) {
      u8g2.setCursor(13, 12);
      u8g2.setFont(u8g2_font_fub35_tf);
      u8g2.print(Input, 1);
    }
    else {
      u8g2.setCursor(-1, 12);
      u8g2.setFont(u8g2_font_fub35_tf);
      u8g2.print(Input, 1);
    }
  }
}

      // Für Statusinfos
      if (Offlinemodus == 0) {
        getSignalStrength();
        if (WiFi.status() != WL_CONNECTED) {
          u8g2.drawFrame(116, 28, 12, 12);
          u8g2.drawXBMP(118, 30, 8, 8, antenna_NOK_u8g2);
        } else {
          if (!Blynk.connected()) {
            u8g2.drawFrame(116, 28, 12, 12);
            u8g2.drawXBMP(118, 30, 8, 8, blynk_NOK_u8g2);
          }
        }
      } else {
        u8g2.drawFrame(116, 28, 12, 12);
        u8g2.setCursor(120, 30);
        u8g2.print("O");
      }
      u8g2.sendBuffer();
    }
  }
