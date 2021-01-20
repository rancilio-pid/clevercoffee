
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
        u8g2.setDisplayRotation(DISPALYROTATE);
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

    /********************************************************
     DISPLAY - print logo and message at boot
    *****************************************************/
    void displayLogo(String displaymessagetext, String displaymessagetext2) 
    {
        u8g2.clearBuffer();
        u8g2.drawStr(0, 47, displaymessagetext.c_str());
        u8g2.drawStr(0, 55, displaymessagetext2.c_str());
        //Rancilio startup logo
        if (machineLogo == 1) {
            u8g2.drawXBMP(41, 2, startLogoRancilio_width, startLogoRancilio_height, startLogoRancilio_bits);
        } else if (machineLogo == 2) {
            u8g2.drawXBMP(0, 2, startLogoGaggia_width, startLogoGaggia_height, startLogoGaggia_bits);
        }
        u8g2.sendBuffer();
    }

    /********************************************************
     DISPLAY - EmergencyStop
    *****************************************************/
    void displayEmergencyStop(void) 
    {
        u8g2.clearBuffer();
        u8g2.drawXBMP(0, 0, logo_width, logo_height, logo_bits_u8g2);   //draw temp icon
        u8g2.setCursor(32, 24);
        u8g2.print("Ist :  ");
        u8g2.print(Input, 1);
        u8g2.print(" ");
        u8g2.print((char)176);
        u8g2.print("C");
        u8g2.setCursor(32, 34);
        u8g2.print("Soll:  ");
        u8g2.print(setPoint, 1);
        u8g2.print(" ");
        u8g2.print((char)176);
        u8g2.print("C");

    //draw current temp in icon
        if (isrCounter < 500)
        {
                u8g2.drawLine(9, 48, 9, 5);
                u8g2.drawLine(10, 48, 10, 4);
                u8g2.drawLine(11, 48, 11, 3);
                u8g2.drawLine(12, 48, 12, 4);
                u8g2.drawLine(13, 48, 13, 5);
                u8g2.setCursor(32, 4);
                u8g2.print("HEATING STOPPED");
        }
        u8g2.sendBuffer();
    }
    void displayShottimer(void) 
     {
        if ((bezugsZeit > 0) && SHOTTIMER == 1) // Shotimer muss 1 = True sein und Bezug vorliegen
        {
            // Dann Zeit anzeigen
            u8g2.clearBuffer();
           // u8g2.drawXBMP(0, 0, logo_width, logo_height, logo_bits_u8g2);   //draw temp icon
            u8g2.drawXBMP(0, 0, brewlogo_width, brewlogo_height, brewlogo_bits_u8g2);
            u8g2.setFont(u8g2_font_profont22_tf);
            u8g2.setCursor(64, 25);
            u8g2.print(bezugsZeit / 1000, 1);
            u8g2.setFont(u8g2_font_profont11_tf);
            u8g2.sendBuffer();
        }
    }

#endif
