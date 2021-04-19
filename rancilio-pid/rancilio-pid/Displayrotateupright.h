#if (DISPLAY == 1 || DISPLAY == 2)  

    /********************************************************
     initialize u8g2 display
    *****************************************************/
    void u8g2_prepare(void) 
    {
        //u8g2.setFont(u8g2_font_6x12_tf);
        u8g2.setFont(u8g2_font_profont11_tf);
        //u8g2.setFont(u8g2_font_IPAandRUSLCD_tf);
        u8g2.setFontRefHeightExtendedText();
        u8g2.setDrawColor(1);
        u8g2.setFontPosTop();
        u8g2.setFontDirection(0);
        u8g2.setDisplayRotation(U8G2_R1); // fix setting for veritcal 
    }

    /********************************************************
     DISPLAY - print message
    *****************************************************/
    void displayMessage(String text1, String text2, String text3, String text4, String text5, String text6) 
    {
        u8g2.clearBuffer();
        u8g2.setCursor(0, 0);
        u8g2.print(text1);
        u8g2.setCursor(0, 10);
        u8g2.print(text2);
        u8g2.setCursor(0, 20);
        u8g2.print(text3);
        u8g2.setCursor(0, 30);
        u8g2.print(text4);
        u8g2.setCursor(0, 40);
        u8g2.print(text5);
        u8g2.setCursor(0, 50);
        u8g2.print(text6);
        u8g2.sendBuffer();
    }

#endif




/********************************************************
 DISPLAY - print logo and message at boot
*****************************************************/
void displayLogo(String displaymessagetext, String displaymessagetext2) 
{
    u8g2.clearBuffer();
    u8g2.drawStr(0, 47, displaymessagetext.c_str());
    u8g2.drawStr(0, 55, displaymessagetext2.c_str());
    //Rancilio startup logo
        switch (machineLogo) {
          case 1:
          u8g2.drawXBMP(9, 2, startLogoRancilio_width, startLogoRancilio_height, startLogoRancilio_bits);
          break;

          case 2: 
          u8g2.drawXBMP(0, 2, startLogoGaggia_width, startLogoGaggia_height, startLogoGaggia_bits);
          break;

          case 3:
          u8g2.drawXBMP(22, 0, startLogoQuickMill_width, startLogoQuickMill_height, startLogoQuickMill_bits);
          break;         
        }
    // if (machineLogo == 1) {
    //   u8g2.drawXBMP(9, 2, startLogoRancilio_width, startLogoRancilio_height, startLogoRancilio_bits);
    // } else if (machineLogo == 2) {
    //   u8g2.drawXBMP(0, 2, startLogoGaggia_width, startLogoGaggia_height, startLogoGaggia_bits);
    // }
    u8g2.sendBuffer();
}

/********************************************************
 DISPLAY - EmergencyStop
*****************************************************/
void displayEmergencyStop(void) 
{
    u8g2.clearBuffer();
    u8g2.setCursor(1, 34);
    u8g2.print(langstring_current_temp_rot_ur);
    u8g2.print(Input, 1);
    u8g2.print(" ");
    u8g2.print((char)176);
    u8g2.print("C");
    u8g2.setCursor(1, 44);
    u8g2.print(langstring_set_temp_rot_ur);
    u8g2.print(setPoint, 1);
    u8g2.print(" ");
    u8g2.print((char)176);
    u8g2.print("C");
    if (isrCounter < 500) {
      u8g2.setCursor(1, 4);
      u8g2.print(langstring_emergencyStop[0]);
      u8g2.setCursor(1, 14);
      u8g2.print(langstring_emergencyStop[1]);
    }
    u8g2.sendBuffer();
}


/********************************************************
 DISPLAY - Shottimer
*****************************************************/
void displayShottimer(void) 
 {
    if (
        (
        (bezugsZeit > 0 && ONLYPID == 1) || // Bezugszeit bei Only PID  
        (ONLYPID == 0 && brewcounter > 10 && brewcounter <= 42) // oder Bezug bei nicht only PID über brewcounter
        ) && SHOTTIMER == 1
        ) // Shotimer muss 1 = True sein und Bezug vorliegen
    {
        // Dann Zeit anzeigen
        u8g2.clearBuffer();
       // u8g2.drawXBMP(0, 0, logo_width, logo_height, logo_bits_u8g2);   //draw temp icon
        u8g2.drawXBMP(0, 0, brewlogo_width, brewlogo_height, brewlogo_bits_u8g2);
        u8g2.setFont(u8g2_font_profont22_tf);
        u8g2.setCursor(5, 70);
        u8g2.print(bezugsZeit / 1000, 1);
        u8g2.setFont(u8g2_font_profont11_tf);
        u8g2.sendBuffer();
        
    }
    if (SHOTTIMER == 1 && millis() >= bezugszeit_last_Millis && // direkt nach Erstellen von bezugszeit_last_mills (passiert beim ausschalten des Brühschalters, case 43 im Code) soll gestartet werden
    bezugszeit_last_Millis+brewswitchDelay >= millis() && // soll solange laufen, bis millis() den brewswitchDelay aufgeholt hat, damit kann die Anzeigedauer gesteuert werden
    bezugszeit_last_Millis < totalbrewtime) // wenn die totalbrewtime automatisch erreicht wird, soll nichts gemacht werden, da sonst falsche Zeit angezeigt wird, da Schalter später betätigt wird als totalbrewtime
    {
        u8g2.clearBuffer();
       u8g2.drawXBMP(0, 0, brewlogo_width, brewlogo_height, brewlogo_bits_u8g2);
       u8g2.setFont(u8g2_font_profont22_tf);
       u8g2.setCursor(5, 70);
       u8g2.print((bezugszeit_last_Millis - startZeit) / 1000, 1);
       u8g2.setFont(u8g2_font_profont11_tf);
       u8g2.sendBuffer();
    }
}    void OFFlogo(void) 
    {
     if (OFFLINEGLOGO == 1 && pidON == 0) 
     {
       u8g2.clearBuffer();
       u8g2.drawXBMP(0,0, OFFLogo_width, OFFLogo_height, OFFLogo); 
       u8g2.setCursor(0, 70);
       u8g2.setFont(u8g2_font_profont10_tf);
       u8g2.print("PID OFFLINE");   
       u8g2.sendBuffer();
     }
    }
