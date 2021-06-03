/********************************************************
    send data to display - Senkrechtes Template
******************************************************/
void printScreen() 
{
  
  unsigned long currentMillisDisplay = millis();
  if  
  (
   (machinestate == 19 || machinestate == 20 || machinestate == 35) ||
   ((machinestate == 30 || machinestate == 31)  && SHOTTIMER == 0) ||// shottimer == 0, auch Bezug anzeigen
   machinestate == 45 ||
   ((machinestate == 10)  && HEATINGLOGO == 0) ||
   ((machinestate == 90)  && OFFLINEGLOGO == 0) 
  ) 
    {

    if (!sensorError) {
      u8g2.clearBuffer();
      u8g2.setCursor(1, 14);
      u8g2.print(langstring_current_temp_rot_ur);
      u8g2.print(Input, 1);
      u8g2.print(" ");
      u8g2.print((char)176);
      u8g2.print("C");
      u8g2.setCursor(1, 24);
      u8g2.print(langstring_set_temp_rot_ur);
      u8g2.print(setPoint, 1);
      u8g2.print(" ");
      u8g2.print((char)176);
      u8g2.print("C");

      // Draw heat bar
      u8g2.drawLine(0, 124, 63, 124);
      u8g2.drawLine(0, 124, 0, 127);
      u8g2.drawLine(64, 124, 63, 127);
      u8g2.drawLine(0, 127, 63, 127);
      u8g2.drawLine(1, 125, (Output / 16.13) + 1, 125);
      u8g2.drawLine(1, 126, (Output / 16.13) + 1, 126);

      //print heating status
      u8g2.setCursor(1, 50);
      u8g2.setFont(u8g2_font_profont22_tf);
      if (fabs(Input  - setPoint) < 0.3) {
        if (isrCounter < 500) {
          u8g2.print("OK");
        }
      } else {
        u8g2.print("WAIT");
      }
      u8g2.setFont(u8g2_font_profont11_tf);

      //print brewdetection
      if (timerBrewdetection == 1) {
        u8g2.setCursor(1, 75);
        u8g2.print("BD ");
        u8g2.print((millis() - timeBrewdetection) / 1000, 1);
        u8g2.print("/");
        u8g2.print(brewtimersoftware, 0);
      }
      
      // PID Werte ueber heatbar
      u8g2.setCursor(1, 84);
      u8g2.print("P: ");
      u8g2.print(bPID.GetKp(), 0); // P

      u8g2.setCursor(1, 93);
      u8g2.print("I: ");
      if (bPID.GetKi() != 0) {
        u8g2.print(bPID.GetKp() / bPID.GetKi(), 0); // I
      }
      else
      {
        u8g2.print("0");
      }
      
      u8g2.setCursor(1, 102);
      u8g2.print("D: ");
      u8g2.print(bPID.GetKd() / bPID.GetKp(), 0); // D
      
      u8g2.setCursor(1, 111);
      if (Output < 99) {
        u8g2.print(Output / 10, 1);
      } else {
        u8g2.print(Output / 10, 0);
      }
      u8g2.print("%");

      // Brew
      u8g2.setCursor(1, 34);
      u8g2.print(langstring_brew_rot_ur);
      u8g2.print(bezugsZeit / 1000, 0);
      u8g2.print("/");
      if (ONLYPID == 1) {
        u8g2.print(brewtimersoftware, 0);             // deaktivieren wenn Preinfusion ( // voransetzen )
      }
      else
      {
        u8g2.print(totalbrewtime / 1000, 0);            // aktivieren wenn Preinfusion
      }
      u8g2.print(" s");

      // Für Statusinfos
      u8g2.drawFrame(0, 0, 64, 12);
      if (Offlinemodus == 0) {
        getSignalStrength();
        if (WiFi.status() == WL_CONNECTED) {
          u8g2.drawXBMP(4, 2, 8, 8, antenna_OK_u8g2);
          for (int b = 0; b <= bars; b++) {
            u8g2.drawVLine(13 + (b * 2), 10 - (b * 2), b * 2);
          }
        } else {
          u8g2.drawXBMP(4, 2, 8, 8, antenna_NOK_u8g2);
          u8g2.setCursor(56, 2);
          u8g2.print("RC: ");
          u8g2.print(wifiReconnects);
        }
        if (Blynk.connected()) {
          u8g2.drawXBMP(24, 2, 11, 8, blynk_OK_u8g2);
        } else {
          u8g2.drawXBMP(24, 2, 8, 8, blynk_NOK_u8g2);
        }
        if (MQTT == 1) {
          if (mqtt.connect(hostname, mqtt_username, mqtt_password)) { 
            u8g2.setCursor(41, 2);
            u8g2.print("MQTT");
          } else {
            u8g2.setCursor(41, 2);
            u8g2.print("");
          }
        }
      } else {
        u8g2.setCursor(4, 1);
        u8g2.print("Offline");
      }
      u8g2.sendBuffer();
    }
  }
}
