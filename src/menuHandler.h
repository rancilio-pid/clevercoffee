
#pragma once

#include <Menu.h>
#include <hardware/pinmapping.h>
#include <icons/menuIcons.h>
#include <button.h>

enum MENUINPUT {
    BUTTONS,
    ROTARY,
};

Menu* menu;
GPIOPin* menuEnterPin;
GPIOPin* menuUpPin;
GPIOPin* menuDownPin;
QueueHandle_t button_events;
button_event_t ev;


bool showMaintenance() {
    return BREWCONTROL_TYPE > 0;
}

bool showBrewPIDSettings() {
    return FEATURE_BREWDETECTION > 0;
}


void menuInputInit() {
    if (MENU_INPUT == MENUINPUT::BUTTONS) {
        menuEnterPin = new GPIOPin(PIN_MENU_ENTER, GPIOPin::IN_PULLUP);
        menuUpPin = new GPIOPin(PIN_MENU_OUT_A, GPIOPin::IN_PULLUP);
        menuDownPin = new GPIOPin(PIN_MENU_OUT_B, GPIOPin::IN_PULLUP);

        button_events = pulled_button_init(
            PIN_BIT(menuEnterPin->getPinNumber()) |
                PIN_BIT(menuUpPin->getPinNumber()) |
                PIN_BIT(menuDownPin->getPinNumber()), GPIO_PULLUP_ONLY
        );
    }
}

void initMenu(U8G2& display) {
    menu = new Menu(display);

    menuInputInit();

    /* Main Menu */
    menu->AddInputItem("Brew Temp.", "Brew Temperature", "", "°C", BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, []() { sysParaPidOn.setStorage(); }, brewSetpoint, bitmap_icon_temp, 0.1, 0.5, true);
    menu->AddInputItem("Steam Temp.", "Steam Temperature", "", "°C", STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, []() { sysParaSteamSetpoint.setStorage(); }, steamSetpoint, bitmap_icon_steam);

    menu->AddToggleItem("PID", []() { sysParaPidOn.setStorage(); }, reinterpret_cast<bool&>(pidON), bitmap_icon_pid);

    menu->SetEventHandler([&]() {
                if (xQueueReceive(button_events, &ev, 1 / portTICK_PERIOD_MS)) {
                    if (ev.pin == menuEnterPin->getPinNumber()) {
                        menu->Event(EVENT_ENTER, EventState(ev.event));
                    } else if (ev.pin == menuUpPin->getPinNumber()) {
                        menu->Event(EVENT_UP, EventState(ev.event));
                    } else if (ev.pin == menuDownPin->getPinNumber()) {
                        menu->Event(EVENT_DOWN, EventState(ev.event));
                    }
                }
    });

    /* Brew Weight & Time */
    Menu* weightNTime = new Menu(display);
    weightNTime->AddInputItem("Brew by Time", "Brew Time", "", " s", BREW_TIME_MIN, BREW_TIME_MAX, []() { sysParaBrewTime.setStorage(); }, brewTime, bitmap_icon_clock);
    weightNTime->AddInputItem("Brew by Weight", "Brew Weight", "", "g", WEIGHTSETPOINT_MIN, WEIGHTSETPOINT_MAX, []() { sysParaWeightSetpoint.setStorage(); }, weightSetpoint, bitmap_icon_scale);
    weightNTime->AddBackItem("Back", bitmap_icon_back);
    menu->AddSubMenu("Brew Time & Weight", *weightNTime, false);

    /* Preinfusion */
    Menu* preInfusion = new Menu(display);
    preInfusion->AddInputItem("Preinfusion Pause", "Pause", "", "s", PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, []() { sysParaPreInfPause.setStorage(); }, preinfusionPause, 1.0, 2.0, true);
    preInfusion->AddInputItem("Preinfusion", "Time", "", "s", PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, []() { sysParaPreInfTime.setStorage(); }, preinfusion, 1.0, 2.0, true);
    preInfusion->AddBackItem("Back", bitmap_icon_back);
    menu->AddSubMenu("Preinfusion", *preInfusion, false);
    /*
     * Maintenance Menu
     * */
    Menu* maintenanceMenu = new Menu(display);
    maintenanceMenu->AddToggleItem("Backflush", reinterpret_cast<bool&>(backflushOn), bitmap_icon_refresh);
    maintenanceMenu->AddBackItem("Back", bitmap_icon_back);

    menu->AddSubMenu("Maintenance", *maintenanceMenu, bitmap_icon_tools, showMaintenance());

    /*
     * Advanced Menu
     */

    Menu* advancedMenu = new Menu(display);
    advancedMenu->AddInputItem("Brew Temp. Offset", "Brew temp. offset", "", "°C", BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, []() { sysParaTempOffset.setStorage(); }, brewTempOffset, bitmap_icon_temp);
    /*
     * Standby Menu
     */
    Menu* standbyMenu = new Menu(display);
    standbyMenu->AddToggleItem("Standby", []() { sysParaStandbyModeOn.setStorage(); }, reinterpret_cast<bool&>(standbyModeOn), true);
    standbyMenu->AddInputItem("Standby Time", "Standby Time", "", " m", STANDBY_MODE_TIME_MIN, STANDBY_MODE_TIME_MAX, []() { sysParaStandbyModeTime.setStorage(); }, standbyModeTime, bitmap_icon_clock, 1.0, 2.0, true);

    standbyMenu->AddBackItem("Back", bitmap_icon_back);
    advancedMenu->AddSubMenu("Standby", *standbyMenu, bitmap_icon_sleep_mode);

    /* PID Settings */
    Menu* pidSettings = new Menu(display);
    pidSettings->AddToggleItem("Enable PonM", []() { sysParaUsePonM.setStorage(); }, reinterpret_cast<bool&>(usePonM));
    pidSettings->AddInputItem("Start Kp", "Start Kp", "", "", PID_KP_START_MIN, PID_KP_START_MAX, []() { (sysParaPidKpStart.setStorage()); }, startKp);
    pidSettings->AddInputItem("Start Tn", "Start Tn", "", "", PID_TN_START_MIN, PID_TN_START_MAX, []() { sysParaPidTnStart.setStorage(); }, startTn);
    pidSettings->AddInputItem("Kp", "Kp", "", "", PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, []() { sysParaPidKpReg.setStorage(); }, aggKp);
    pidSettings->AddInputItem("Tn", "Tn (=Kp/Ki)", "", "", PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, []() { sysParaPidTnReg.setStorage(); }, aggTn);
    pidSettings->AddInputItem("Tv", "Tv (=Kd/Kp)", "", "", PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, []() { sysParaPidTvReg.setStorage(); }, aggTv);
    pidSettings->AddInputItem("Integrator Max", "Integrator Max", "", "", PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, []() { sysParaPidIMaxReg.setStorage(); }, aggIMax);
    pidSettings->AddInputItem("Steam Kp", "Steam Kp", "", "", PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, []() { sysParaPidKpSteam.setStorage(); }, steamKp);
    pidSettings->AddBackItem("Back", bitmap_icon_back);

    /* Brew PID Settings */


    advancedMenu->AddSubMenu("PID Settings", *pidSettings, bitmap_icon_pid);
    advancedMenu->AddBackItem("Back", bitmap_icon_back);
    menu->AddSubMenu("Advanced", *advancedMenu, bitmap_icon_settings);

    menu->AddBackItem("Close Menu", bitmap_icon_back);
    menu->Init();
}

void menuLoop() {
    menu->Loop();
}