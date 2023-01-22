/**
 * @file Displayrotateupright.h
 *
 * @brief
 */

#if (OLED_DISPLAY == 1 || OLED_DISPLAY == 2)
/**
 * @brief initialize display
 */
void u8g2_prepare(void) {
    u8g2.setFont(u8g2_font_profont11_tf);
    u8g2.setFontRefHeightExtendedText();
    u8g2.setDrawColor(1);
    u8g2.setFontPosTop();
    u8g2.setFontDirection(0);
    u8g2.setDisplayRotation(U8G2_R1); // fix setting for veritcal
}

/**
 * @brief print message
 */
void displayMessage(String text1, String text2, String text3, String text4, String text5, String text6) {
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

/**
 * @brief print logo and message at boot
 */
void displayLogo(String displaymessagetext, String displaymessagetext2) {
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

    u8g2.sendBuffer();
}

#if 0 //not used a.t.m.
/**
 * @brief display emergency stop
 */
void displayEmergencyStop(void)
{
    u8g2.clearBuffer();
    u8g2.setCursor(1, 34);
    u8g2.print(langstring_current_temp_rot_ur);
    u8g2.print(temperature, 1);
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
#endif


/**
 * @brief display shot timer
 */
void displayShottimer(void) {
    if (((timeBrewed > 0 && ONLYPID == 1) || // timeBrewed bei Only PID
        (ONLYPID == 0 && brewcounter > kBrewIdle && brewcounter <= kBrewFinished)) // oder Bezug bei nicht only PID über brewcounter
        && SHOTTIMER == 1) // Shotimer muss 1 = True sein und Bezug vorliegen
    {
        // Dann Zeit anzeigen
        u8g2.clearBuffer();

        // draw temp icon
        u8g2.drawXBMP(0, 0, brewlogo_width, brewlogo_height, brewlogo_bits_u8g2);
        u8g2.setFont(u8g2_font_profont22_tf);
        u8g2.setCursor(5, 70);
        u8g2.print(timeBrewed / 1000, 1);
        u8g2.setFont(u8g2_font_profont11_tf);
        u8g2.sendBuffer();

    }
    if (SHOTTIMER == 1 && millis() >= brewTime_last_Millis && // directly after creating brewTime_last_mills (happens when turning off the brew switch, case 43 in the code) should be started
        brewTime_last_Millis+brewswitchDelay >= millis() && // should run until millis() has caught up with brewswitchDelay, this can be used to control the display duration
        brewTime_last_Millis < totalBrewTime) // if the totalBrewTime is reached automatically, nothing should be done, otherwise wrong time will be displayed because switch is pressed later than totalBrewTime
    {
        u8g2.clearBuffer();
        u8g2.drawXBMP(0, 0, brewlogo_width, brewlogo_height, brewlogo_bits_u8g2);
        u8g2.setFont(u8g2_font_profont22_tf);
        u8g2.setCursor(5, 70);
        u8g2.print((brewTime_last_Millis - startingTime) / 1000, 1);
        u8g2.setFont(u8g2_font_profont11_tf);
        u8g2.sendBuffer();
    }
}
