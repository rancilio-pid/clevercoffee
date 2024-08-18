
#pragma once

#include <ESP32Encoder.h>
#include <Menu.h>
#include <button.h>
#include <hardware/pinmapping.h>
#include <icons/menuIcons.h>

enum MENUINPUT {
    BUTTONS,
    ROTARY,
};

Menu* menu;
GPIOPin* menuEnterPin;
GPIOPin* menuUpPin;
GPIOPin* menuDownPin;
ESP32Encoder encoder;
QueueHandle_t button_events;
button_event_t ev;

int last = 0;

void saveBrewTemp() {
    sysParaBrewSetpoint.setStorage(true);
}

void saveSteamTemp() {
    sysParaSteamSetpoint.setStorage(true);
}

void savePIDOn() {
    sysParaPidOn.setStorage(true);
}

void saveStandby() {
    sysParaStandbyModeOn.setStorage(true);
}

void saveStandbyTime() {
    sysParaStandbyModeTime.setStorage(true);
}

bool hasBrewControl() {
    return FEATURE_BREWCONTROL > 0;
}

bool hasScale() {
    return FEATURE_SCALE > 0;
}

bool hasSoftwareDetection() {
    return BREWDETECTION_TYPE == 1;
}

void menuInputInit() {
    switch (MENU_INPUT) {
        case MENUINPUT::BUTTONS:
            menuEnterPin = new GPIOPin(PIN_MENU_ENTER, GPIOPin::IN_PULLUP);
            menuUpPin = new GPIOPin(PIN_MENU_OUT_A, GPIOPin::IN_PULLUP);
            menuDownPin = new GPIOPin(PIN_MENU_OUT_B, GPIOPin::IN_PULLUP);

            button_events = pulled_button_init(PIN_BIT(menuEnterPin->getPinNumber()) | PIN_BIT(menuUpPin->getPinNumber()) | PIN_BIT(menuDownPin->getPinNumber()), GPIO_PULLUP_ONLY);
            break;
        case MENUINPUT::ROTARY:
            menuEnterPin = new GPIOPin(PIN_MENU_ENTER, GPIOPin::IN_PULLUP);
            menuUpPin = new GPIOPin(PIN_MENU_OUT_A, GPIOPin::IN_PULLUP);
            menuDownPin = new GPIOPin(PIN_MENU_OUT_B, GPIOPin::IN_PULLUP);

            button_events = pulled_button_init(PIN_BIT(menuEnterPin->getPinNumber()), GPIO_PULLUP_ONLY);

            encoder.useInternalWeakPullResistors = puType::up;
            encoder.attachFullQuad(PIN_MENU_OUT_A, PIN_MENU_OUT_B);
            encoder.setCount(0);

            break;
        default:
            break;
    }
}

void initMenu(U8G2& display) {
    menu = new Menu(display);

    menuInputInit();

    /* Main Menu */
    menu->AddInputItem("Brew Temp.", "Brew Temperature", "", "°C", BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, saveBrewTemp, brewSetpoint, bitmap_icon_temp, 0.1, 0.5);
    menu->AddInputItem("Steam Temp.", "Steam Temperature", "", "°C", STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, saveSteamTemp, steamSetpoint, bitmap_icon_steam, 0.1, 0.5);

    menu->AddToggleItem("PID", savePIDOn, reinterpret_cast<bool&>(pidON), bitmap_icon_pid);

    menu->SetEventHandler([&]() {
        if (xQueueReceive(button_events, &ev, 1 / portTICK_PERIOD_MS)) {
            if (ev.pin == menuEnterPin->getPinNumber()) {
                if (standbyModeRemainingTimeMillis == 0) {
                    resetStandbyTimer();
                    display.setPowerSave(0);
                    pidON = 1;
                    if (steamON) {
                        machineState = kSteam;
                    }
                    else if (isBrewDetected) {
                        machineState = kBrew;
                    }
                    else {
                        machineState = kPidDisabled;
                    }
                    return;
                }
                if (ev.event == EventState::STATE_DOWN) {
                    resetStandbyTimer();
                }
                menu->Event(EVENT_ENTER, EventState(ev.event));
            }
            else {
                if (MENU_INPUT == MENUINPUT::BUTTONS) {
                    if (ev.pin == menuUpPin->getPinNumber()) {
                        resetStandbyTimer();
                        menu->Event(EVENT_UP, EventState(ev.event));
                    }
                    else if (ev.pin == menuDownPin->getPinNumber()) {
                        resetStandbyTimer();
                        menu->Event(EVENT_DOWN, EventState(ev.event));
                    }
                }
            }
        }
        if (MENU_INPUT == MENUINPUT::ROTARY) {
            int32_t pos = encoder.getCount() / ENCODER_CLICKS_PER_NOTCH;
            if (pos > last) {
                menu->Event(EVENT_UP, EventState(EventState::STATE_DOWN));
                LOG(DEBUG, "Menu: Up\n");
                menu->Event(EVENT_UP, EventState(EventState::STATE_UP));
            }
            else if (pos < last) {
                menu->Event(EVENT_DOWN, EventState(EventState::STATE_DOWN));
                LOG(DEBUG, "Menu: Down\n");
                menu->Event(EVENT_DOWN, EventState(EventState::STATE_UP));
            }

            last = pos;
        }
    });

    /* Brew Weight & Time */
    Menu* weightNTime = new Menu(display);
    weightNTime->AddInputItem("Brew by Time", "Brew Time", "", " s", BREW_TIME_MIN, BREW_TIME_MAX, []() { sysParaBrewTime.setStorage(true); }, brewTime, bitmap_icon_clock);
    weightNTime->AddInputItem("Brew by Weight", "Brew Weight", "", "g", WEIGHTSETPOINT_MIN, WEIGHTSETPOINT_MAX, []() { sysParaWeightSetpoint.setStorage(true); }, weightSetpoint, bitmap_icon_scale, hasScale());
    weightNTime->AddBackItem("Back", bitmap_icon_back);
    menu->AddSubMenu("Brew Time & Weight", *weightNTime, hasBrewControl());

    /* Preinfusion */
    Menu* preInfusion = new Menu(display);
    preInfusion->AddInputItem("Preinfusion Pause", "Pause", "", "s", PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, []() { sysParaPreInfPause.setStorage(true); }, preinfusionPause, 1.0, 2.0, true);
    preInfusion->AddInputItem("Preinfusion", "Time", "", "s", PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, []() { sysParaPreInfTime.setStorage(true); }, preinfusion, 1.0, 2.0, true);
    preInfusion->AddBackItem("Back", bitmap_icon_back);
    menu->AddSubMenu("Preinfusion", *preInfusion, hasBrewControl());
    /*
     * Maintenance Menu
     * */
    Menu* maintenanceMenu = new Menu(display);
    maintenanceMenu->AddToggleItem("Backflush", reinterpret_cast<bool&>(backflushOn), bitmap_icon_refresh);
    maintenanceMenu->AddBackItem("Back", bitmap_icon_back);

    menu->AddSubMenu("Maintenance", *maintenanceMenu, bitmap_icon_tools, hasBrewControl());

    /*
     * Advanced Menu
     */

    Menu* advancedMenu = new Menu(display);
    advancedMenu->AddInputItem("Brew Temp. Offset", "Brew temp. offset", "", "°C", BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, []() { sysParaTempOffset.setStorage(true); }, brewTempOffset, bitmap_icon_temp);
    /*
     * Standby Menu
     */
    Menu* standbyMenu = new Menu(display);
    standbyMenu->AddToggleItem("Standby", saveStandby, reinterpret_cast<bool&>(standbyModeOn), true);
    standbyMenu->AddInputItem("Standby Time", "Standby Time", "", " m", STANDBY_MODE_TIME_MIN, STANDBY_MODE_TIME_MAX, saveStandbyTime, standbyModeTime, bitmap_icon_clock, 1.0, 2.0, true);

    standbyMenu->AddBackItem("Back", bitmap_icon_back);
    advancedMenu->AddSubMenu("Standby", *standbyMenu, bitmap_icon_sleep_mode);

    /* PID Settings */
    Menu* pidSettings = new Menu(display);
    pidSettings->AddToggleItem("Enable PonM", []() { sysParaUsePonM.setStorage(true); }, reinterpret_cast<bool&>(usePonM));
    pidSettings->AddInputItem("Start Kp", "Start Kp", "", "", PID_KP_START_MIN, PID_KP_START_MAX, []() { (sysParaPidKpStart.setStorage(true)); }, startKp);
    pidSettings->AddInputItem("Start Tn", "Start Tn", "", "", PID_TN_START_MIN, PID_TN_START_MAX, []() { sysParaPidTnStart.setStorage(true); }, startTn);
    pidSettings->AddInputItem("Kp", "Kp", "", "", PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, []() { sysParaPidKpReg.setStorage(true); }, aggKp);
    pidSettings->AddInputItem("Tn", "Tn (=Kp/Ki)", "", "", PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, []() { sysParaPidTnReg.setStorage(true); }, aggTn);
    pidSettings->AddInputItem("Tv", "Tv (=Kd/Kp)", "", "", PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, []() { sysParaPidTvReg.setStorage(true); }, aggTv);
    pidSettings->AddInputItem("Integrator Max", "Integrator Max", "", "", PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, []() { sysParaPidIMaxReg.setStorage(true); }, aggIMax);
    pidSettings->AddInputItem("Steam Kp", "Steam Kp", "", "", PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, []() { sysParaPidKpSteam.setStorage(true); }, steamKp);

    /* Brew PID Settings */
    Menu* brewPidSettings = new Menu(display);
    brewPidSettings->AddToggleItem("Enable Brew PID", []() { sysParaUsePonM.setStorage(true); }, reinterpret_cast<bool&>(useBDPID));
    brewPidSettings->AddInputItem("BD Kp", "BD Kp", "", "", PID_KP_BD_MIN, PID_KP_BD_MAX, []() { sysParaPidKpBd.setStorage(true); }, aggbKp);
    brewPidSettings->AddInputItem("BD Tn", "BD Tn (=Kp/Ki)", "", "", PID_TN_BD_MIN, PID_TN_BD_MAX, []() { sysParaPidTnBd.setStorage(true); }, aggbTn);
    brewPidSettings->AddInputItem("BD Tv", "BD Tv (=Kd/Kp)", "", "", PID_TV_BD_MIN, PID_TV_BD_MAX, []() { sysParaPidTvBd.setStorage(true); }, aggbTv);
    brewPidSettings->AddInputItem("PID BD Time", "PID BD Time", "", "s", BREW_SW_TIME_MIN, BREW_SW_TIME_MAX, []() { sysParaBrewSwTime.setStorage(true); }, brewtimesoftware, hasSoftwareDetection());
    brewPidSettings->AddInputItem("PID BD Sensitivity", "Sensitivity", "", "", BD_THRESHOLD_MIN, BD_THRESHOLD_MAX, []() { sysParaBrewThresh.setStorage(true); }, brewSensitivity, hasSoftwareDetection());
    brewPidSettings->AddBackItem("Back", bitmap_icon_back);

    pidSettings->AddSubMenu("Brew PID", *brewPidSettings);
    pidSettings->AddBackItem("Back", bitmap_icon_back);

    advancedMenu->AddSubMenu("PID Settings", *pidSettings, bitmap_icon_pid);
    advancedMenu->AddBackItem("Back", bitmap_icon_back);
    menu->AddSubMenu("Advanced", *advancedMenu, bitmap_icon_settings);

    menu->AddBackItem("Close Menu", bitmap_icon_back);
    menu->Init();
}

void menuLoop() {
    menu->Loop();
}
