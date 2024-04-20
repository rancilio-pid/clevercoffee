/**
 * @file displayRotateUpright.h
 *
 * @brief Display template based on the standard template but rotated 90 degrees
 *
 */
 
#pragma once

#if (DISPLAY_HARDWARE == 1 || DISPLAY_HARDWARE == 2)

/**
 * @brief print message
 */
void displayMessage(String text1, String text2, String text3, String text4, String text5, String text6) {
    display.clearBuffer();
    display.setCursor(0, 0);
    display.println(text1);
    display.setCursor(0, 10);
    display.println(text2);
    display.setCursor(0, 20);
    display.println(text3);
    display.setCursor(0, 30);
    display.println(text4);
    display.setCursor(0, 40);
    display.println(text5);
    display.setCursor(0, 50);
    display.println(text6);
    display.sendBuffer();
}
#endif

/**
 * @brief print logo and message at boot
 */
void displayLogo(String displaymessagetext, String displaymessagetext2) {
    display.clearBuffer();
    display.setCursor(0, 47);
    display.print(displaymessagetext.c_str());
    display.setCursor(0, 55);
    display.print(displaymessagetext2.c_str());

    display.drawImage(11, 4, CleverCoffee_Logo_width, CleverCoffee_Logo_height, CleverCoffee_Logo);

    display.sendBuffer();
}

#if 0 // not used a.t.m.
/**
 * @brief display emergency stop
 */
void displayEmergencyStop(void) {
    display.clearBuffer();
    display.setCursor(1, 34);
    display.print(langstring_current_temp_rot_ur);
    display.print(temperature, 1);
    display.print(" ");
    display.print((char)176);
    display.print("C");
    display.setCursor(1, 44);
    display.print(langstring_set_temp_rot_ur);
    display.print(setPoint, 1);
    display.print(" ");
    display.print((char)176);
    display.print("C");

    if (isrCounter < 500) {
        display.setCursor(1, 4);
        display.print(langstring_emergencyStop[0]);
        display.setCursor(1, 14);
        display.print(langstring_emergencyStop[1]);
    }

    display.sendBuffer();
}
#endif

/**
 * @brief display shot timer
 */
bool displayShottimer() {
    if (((timeBrewed > 0 && FEATURE_BREWCONTROL == 0) || (FEATURE_BREWCONTROL > 0 && currBrewState > kBrewIdle && currBrewState <= kBrewFinished)) && FEATURE_SHOTTIMER == 1) {
        display.clearBuffer();

        // draw temp icon
        display.drawImage(0, 0, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);
        display.setFont(FontType::Big);
        display.setCursor(5, 70);
        display.print(timeBrewed / 1000, 1);
        display.setFont(FontType::Normal);
        display.sendBuffer();
        return true;
    }
    else if (FEATURE_SHOTTIMER == 1 && millis() >= lastBrewTimeMillis && // directly after creating lastbrewTimeMillis (happens when turning off the brew switch, case 43 in the code) should be started
             lastBrewTimeMillis + SHOTTIMERDISPLAYDELAY >= millis() &&   // should run until millis() has caught up with SHOTTIMERDISPLAYDELAY, this can be used to control the display duration
             lastBrewTimeMillis < totalBrewTime) // if the totalBrewTime is reached automatically, nothing should be done, otherwise wrong time will be displayed because switch is pressed later than totalBrewTime
    {
        display.clearBuffer();
        display.drawImage(0, 0, Brew_Cup_Logo_width, Brew_Cup_Logo_height, Brew_Cup_Logo);
        display.setFont(FontType::Big);
        display.setCursor(5, 70);
        display.print((lastBrewTimeMillis - startingTime) / 1000, 1);
        display.setFont(FontType::Normal);
        display.sendBuffer();
        return true;
    }

    return false;
}
