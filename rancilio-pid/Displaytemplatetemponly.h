/********************************************************
    send data to display
******************************************************/
void printScreen() 
{
  // Abbruch der Anzeige, Return, wenn Shottimer läuft. 
  if ((SHOTTIMER == 1 && bezugsZeit > 0) || 
  (SHOTTIMER == 1 && millis() >= bezugszeit_last_Millis && bezugszeit_last_Millis+brewswitchDelay >= millis())) // sobald der Brühschalter umgelegt wird, brewswitchDelay abgelaufen
  return;
  unsigned long currentMillisDisplay = millis();
  if (currentMillisDisplay - previousMillisDisplay >= intervalDisplay) {
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
        u8g2.print(Input);
      }
      else {
        u8g2.setCursor(-1, 12);
        u8g2.setFont(u8g2_font_fub35_tf);
        u8g2.print(Input);
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
