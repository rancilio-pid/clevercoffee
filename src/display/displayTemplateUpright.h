/**
 * @file displayTemplateUpright.h
 *
 * @brief Vertical display template
 *
 */

#pragma once

/**
 * @brief Send data to display
 */
inline void printScreen() {
    const bool scaleEnabled = config.get<bool>("hardware.sensors.scale.enabled");
    const bool pressureEnabled = config.get<bool>("hardware.sensors.pressure.enabled");
    const bool brewEnabled = config.get<bool>("hardware.switches.brew.enabled");

    if (displayFullscreenBrewTimer()) {
        // Display was updated, end here
        return;
    }

    if (displayFullscreenManualFlushTimer()) {
        // Display was updated, end here
        return;
    }

    // Show fullscreen hot water timer:
    if (displayFullscreenHotWaterTimer()) {
        // Display was updated, end here
        return;
    }

    if (displayOfflineMode()) {
        // Display was updated, end here
        return;
    }

    u8g2->clearBuffer();
    if (machineState == kWaterTankEmpty) {
        u8g2->drawXBMP(8, 50, Water_Tank_Empty_Logo_width, Water_Tank_Empty_Logo_height, Water_Tank_Empty_Logo);
    }
    else if (machineState == kSensorError) {
        displayWrappedMessage(String(langstring_error_tsensor[0]) + String(temperature) + '\n' + String(langstring_error_tsensor[1]));
    }
    else if (machineState == kStandby) {
        u8g2->drawXBMP(6, 50, Off_Logo_width, Off_Logo_height, Off_Logo);
        u8g2->setCursor(1, 110);
        u8g2->setFont(u8g2_font_profont10_tf);
        u8g2->print("Standby mode");
    }
    else {
        // no fullscreen states
        u8g2->setFont(u8g2_font_profont11_tf);
        u8g2->setCursor(1, 14);
        u8g2->print(langstring_current_temp_ur);
        u8g2->print(temperature, 1);
        u8g2->print(" ");
        u8g2->print(static_cast<char>(176));
        u8g2->print("C");
        u8g2->setCursor(1, 24);
        u8g2->print(langstring_set_temp_ur);
        u8g2->print(setpoint, 1);
        u8g2->print(" ");
        u8g2->print(static_cast<char>(176));
        u8g2->print("C");

        // Draw heat bar
        u8g2->drawFrame(0, 124, 64, 4);
        u8g2->drawLine(1, 125, pidOutput / 16.13 + 1, 125);
        u8g2->drawLine(1, 126, pidOutput / 16.13 + 1, 126);

        // logos that only fill the lower half leaving temperatures, top and bottom boxes
        if (machineState == kPidDisabled) {
            u8g2->drawXBMP(6, 50, Off_Logo_width, Off_Logo_height, Off_Logo);
            u8g2->setCursor(1, 110);
            u8g2->setFont(u8g2_font_profont10_tf);
            u8g2->print("PID disabled");
        }

        // Steam
        else if (machineState == kSteam) {
            u8g2->drawXBMP(12, 50, Steam_Logo_width, Steam_Logo_height, Steam_Logo);
        }

        // Show the heating logo when we are in regular PID mode and more than 5degC below the set point
        else if (config.get<bool>("display.heating_logo") && machineState == kPidNormal && setpoint - temperature > 5.0) {
            // For status info
            u8g2->drawXBMP(12, 50, Heating_Logo_width, Heating_Logo_height, Heating_Logo);
            u8g2->setFont(u8g2_font_fub17_tr);
            u8g2->setCursor(8, 90);
            u8g2->print(temperature, 1);
        }
        else {
            // print status
            if (scaleEnabled && pressureEnabled) {
                u8g2->setCursor(1, 65);
            }
            else if (scaleEnabled || pressureEnabled) {
                u8g2->setCursor(1, 60);
            }
            else {
                u8g2->setCursor(1, 55);
            }

            u8g2->setFont(u8g2_font_profont22_tr);

            if (machineState == kManualFlush) {
                u8g2->print("FLUSH");
            }
            else if (machineState == kBackflush) {
                u8g2->setFont(u8g2_font_profont15_tr);
                u8g2->print("BACKFLUSH");
            }
            else if (shouldDisplayBrewTimer()) {
                u8g2->print("BREW");
            }
            else if (fabs(temperature - setpoint) < 0.3) {

                if (isrCounter < 500) {
                    u8g2->print("OK");
                }
                else {
                    u8g2->print("");
                }
            }
            else {
                u8g2->print("WAIT");
            }

            u8g2->setFont(u8g2_font_profont11_tf);

            // PID values above heater output bar
            u8g2->setCursor(1, 84);
            u8g2->print("P: ");
            u8g2->print(bPID.GetKp(), 0);

            u8g2->setCursor(1, 93);
            u8g2->print("I: ");

            if (bPID.GetKi() != 0) {
                u8g2->print(bPID.GetKp() / bPID.GetKi(), 0);
            }
            else {
                u8g2->print("0");
            }
            u8g2->setCursor(1, 102);
            u8g2->print("D: ");
            u8g2->print(bPID.GetKd() / bPID.GetKp(), 0);
            u8g2->setCursor(1, 111);

            if (pidOutput < 99) {
                u8g2->print(pidOutput / 10, 1);
            }
            else {
                u8g2->print(pidOutput / 10, 0);
            }

            u8g2->print("%");

            // Brew
            if (scaleEnabled) {
                displayBrewWeight(1, 44, currReadingWeight, -1, scaleFailure);
            }

            if (pressureEnabled) {
                u8g2->setFont(u8g2_font_profont11_tf);

                if (scaleEnabled) {
                    u8g2->setCursor(1, 54);
                }
                else {
                    u8g2->setCursor(1, 44);
                }

                u8g2->print(langstring_pressure_ur);
                u8g2->print(inputPressure, 1);
                u8g2->print(" bar");
            }

            // Brew time
            if (brewEnabled) {
                // Show flush time
                if (machineState == kManualFlush) {
                    displayBrewTime(1, 34, langstring_manual_flush_ur, currBrewTime);
                }
                // Show hot water time
                else if (machineState == kHotWater) {
                    displayBrewTime(1, 34, langstring_hot_water_ur, currPumpOnTime);
                }
                else {
                    const bool automaticBrewingEnabled = config.get<bool>("brew.mode") == 1;

                    // Show brew time
                    if (shouldDisplayBrewTimer()) {
                        if (automaticBrewingEnabled && config.get<bool>("brew.by_time.enabled")) {
                            displayBrewTime(1, 34, langstring_brew_ur, currBrewTime, totalTargetBrewTime);
                        }
                        else {
                            displayBrewTime(1, 34, langstring_brew_ur, currBrewTime);
                        }

                        if (scaleEnabled) {
                            if (automaticBrewingEnabled && config.get<bool>("brew.by_weight.enabled")) {
                                const auto targetBrewWeight = ParameterRegistry::getInstance().getParameterById("brew.by_weight.target_weight")->getValueAs<float>();
                                displayBrewWeight(1, 44, currBrewWeight, targetBrewWeight, scaleFailure);
                            }
                            else {
                                displayBrewWeight(1, 44, currBrewWeight, -1, scaleFailure);
                            }
                        }
                    }
                }
            }
        }

        // Status info in top bar
        u8g2->drawLine(0, 12, 64, 12);
        if (!offlineMode) {
            displayWiFiStatus(4, 2);
            displayMQTTStatus(21, 0);
        }
        else {
            u8g2->setCursor(4, 1);
            u8g2->setFont(u8g2_font_profont11_tf);
            u8g2->print(langstring_offlinemode);
        }

        if (config.get<bool>("hardware.sensors.scale.enabled") && config.get<int>("hardware.sensors.scale.type") == 2) {
            displayBluetoothStatus(54, 1);
        }
    }

    displayBufferReady = true;
}
