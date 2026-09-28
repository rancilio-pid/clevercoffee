
#pragma once

#include <ESP32Encoder.h>
#include <Logger.h>
#include <Menu.h>
#include <button.h>
#include <hardware/pinmapping.h>
#include <icons/menuIcons.h>

enum MENUINPUT {
    BUTTONS,
    ROTARY,
};

enum MENUENCODER {
    FULL_QUAD,
    HALF_QUAD,
    SINGLE_EDGE,
};

inline Menu* menu;

inline GPIOPin* menuEnterPin;
inline GPIOPin* menuUpPin;
inline GPIOPin* menuDownPin;
inline ESP32Encoder encoder;
inline QueueHandle_t button_events;
inline button_event_t ev;

inline int last = 0;
inline int idleTime = 0;

inline std::vector<std::function<void()>>& getMenuVarUpdaters() {
    static std::vector<std::function<void()>> updaters;
    return updaters;
}

template <typename T>
T& getMenuVar(const char* paramId) {
    struct Entry {
            const char* id;
            T value;
    };
    static Entry menuVars[100];
    static int count = 0;

    for (int i = 0; i < count; i++) {
        if (strcmp(menuVars[i].id, paramId) == 0) {
            return menuVars[i].value;
        }
    }

    if (count < 100) {
        menuVars[count].id = paramId;
        menuVars[count].value = config.get<T>(paramId);

        T& ref = menuVars[count].value;
        getMenuVarUpdaters().push_back([paramId, &ref]() { ref = config.get<T>(paramId); });

        return menuVars[count++].value;
    }

    // Fallback if array is full
    static T dummy;
    return dummy;
}

inline void syncMenuVars() {
    for (auto& updater : getMenuVarUpdaters()) {
        updater();
    }
}

template <typename T>
auto makeSaveCallback(const char* param, T& value) {
    return [param, &value]() {
        T oldVal = config.get<T>(param);
        LOGF(DEBUG, "Saving %s: old=%s, new=%s", param, String(oldVal).c_str(), String(value).c_str());
        if (!ParameterRegistry::getInstance().setParameterValue(param, value)) {
            LOG(ERROR, "Failed to save config to filesystem!");
        }
    };
}

template <typename T>
auto makeSaveCallback(const char* paramId) {
    T& valueRef = getMenuVar<T>(paramId);
    return [paramId, &valueRef]() {
        T oldVal = config.get<T>(paramId);
        LOGF(DEBUG, "Saving %s: old=%s, new=%s", paramId, String(oldVal).c_str(), String(valueRef).c_str());
        if (!ParameterRegistry::getInstance().setParameterValue(paramId, valueRef)) {
            LOG(ERROR, "Failed to save config to filesystem!");
        }
    };
}

// Special helper for MenuInfoEntry (returns const char**)
inline const char** getMenuInfoVar(const std::string& paramId) {
    static std::map<std::string, String> infoStrings;
    static std::map<std::string, const char*> infoPointers;

    if (infoStrings.find(paramId) == infoStrings.end()) {
        infoStrings[paramId] = config.get<String>(paramId.c_str());

        infoPointers[paramId] = infoStrings[paramId].c_str();
    }

    return &infoPointers[paramId];
}

// Return array of enum options
inline const char* const* getMenuEnumOptions(const std::string& paramId) {
    auto param = ParameterRegistry::getInstance().getParameterById(paramId.c_str());
    if (param && param->isEnum()) {
        return param->getEnumOptions();
    }
    return nullptr;
}

// Return number of enum options
inline size_t getMenuEnumCount(const std::string& paramId) {
    auto param = ParameterRegistry::getInstance().getParameterById(paramId.c_str());
    if (param && param->isEnum()) {
        return param->getEnumCount();
    }
    return 0;
}

auto timeoutIdleActive() {
    makeSaveCallback<bool>("display.menu.idle_timeout.enabled")();
    LOGF(DEBUG, "timeoutIdleActive() enabled: %d", getMenuVar<bool>("display.menu.idle_timeout.enabled"));
    menu->TimeoutActive(getMenuVar<bool>("display.menu.idle_timeout.enabled"));
}

auto timeoutIdleTimeout() {
    makeSaveCallback<double>("display.menu.idle_timeout.time")();
    LOGF(DEBUG, "timeoutIdleTimeout() time: %f", getMenuVar<double>("display.menu.idle_timeout.time"));
    menu->SetTimeout(getMenuVar<double>("display.menu.idle_timeout.time"));
}

auto scrollInvertActive() {
    makeSaveCallback<bool>("display.scroll.invert")();
    LOGF(DEBUG, "scrollInvertActive() invert: %d", getMenuVar<bool>("display.scroll.invert"));
    menu->InvertScrollInput(getMenuVar<bool>("display.scroll.invert"));
}

auto inputInvertActive() {
    makeSaveCallback<bool>("display.input.invert")();
    LOGF(DEBUG, "inputInvertActive() invert: %d", getMenuVar<bool>("display.input.invert"));
    menu->InvertMenuInput(getMenuVar<bool>("display.input.invert"));
}

const auto reboot = []() {
    LOG(DEBUG, "reboot()");
    ParameterRegistry::getInstance().forceSave();
    delay(100);
    ESP.restart();
};

inline int activeMenuInput = -1;
inline int32_t encoderDivisor = 4;

inline void menuInputInit() {
    activeMenuInput = config.get<int>("hardware.oled.menu.input");
    LOGF(DEBUG, "menuInputInit() activeMenuInput: %d", activeMenuInput);
    LOGF(DEBUG, "enter: %d, A: %d B: %d", PIN_MENU_ENTER, PIN_MENU_OUT_A, PIN_MENU_OUT_B);
    switch (activeMenuInput) {
        case MENUINPUT::BUTTONS:
            LOG(DEBUG, "Using rotary encoder for menu input");
            menuEnterPin = new GPIOPin(PIN_MENU_ENTER, GPIOPin::IN_PULLUP);
            menuUpPin = new GPIOPin(PIN_MENU_OUT_A, GPIOPin::IN_PULLUP);
            menuDownPin = new GPIOPin(PIN_MENU_OUT_B, GPIOPin::IN_PULLUP);

            button_events = pulled_button_init(PIN_BIT(menuEnterPin->getPinNumber()) | PIN_BIT(menuUpPin->getPinNumber()) | PIN_BIT(menuDownPin->getPinNumber()), GPIO_PULLUP_ONLY);

            break;
        case MENUINPUT::ROTARY:
            LOG(DEBUG, "Using rotary encoder for menu input");
            menuEnterPin = new GPIOPin(PIN_MENU_ENTER, GPIOPin::IN_PULLUP);
            menuUpPin = new GPIOPin(PIN_MENU_OUT_A, GPIOPin::IN_PULLUP);
            menuDownPin = new GPIOPin(PIN_MENU_OUT_B, GPIOPin::IN_PULLUP);

            encoder.useInternalWeakPullResistors = puType::up;

            switch (config.get<int>("hardware.oled.menu.encoder_type")) {
                case MENUENCODER::FULL_QUAD:
                    encoder.attachFullQuad(PIN_MENU_OUT_A, PIN_MENU_OUT_B);
                    encoderDivisor = 4;
                    break;
                case MENUENCODER::HALF_QUAD:
                    encoder.attachHalfQuad(PIN_MENU_OUT_A, PIN_MENU_OUT_B);
                    encoderDivisor = 2;
                    break;
                case MENUENCODER::SINGLE_EDGE:
                    encoder.attachSingleEdge(PIN_MENU_OUT_A, PIN_MENU_OUT_B);
                    encoderDivisor = 1;
                    break;
            }

            encoder.setFilter(1023);
            encoder.setCount(0);

            button_events = pulled_button_init(PIN_BIT(menuEnterPin->getPinNumber()), GPIO_PULLUP_ONLY);

            break;
        default:
            break;
    }
}

void initMenu(U8G2& display) {
    LOG(DEBUG, "initMenu()");
    menu = new Menu(display);

    auto& params = ParameterRegistry::getInstance();

    menuInputInit();

    menu->InvertScrollInput(getMenuVar<bool>("display.menu.input.inverted"));
    menu->InvertMenuInput(getMenuVar<bool>("display.menu.scroll.inverted"));

    /* Main Menu */
    menu->AddInputItem("Brew Temp.", "Brew Temperature", "", "°C", BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, makeSaveCallback<double>("brew.setpoint"), getMenuVar<double>("brew.setpoint"), bitmap_icon_temp, 0.1, 0.5);
    menu->AddInputItem("Steam Temp.", "Steam Temperature", "", "°C", STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, makeSaveCallback<double>("steam.setpoint"), getMenuVar<double>("steam.setpoint"), bitmap_icon_steam, 0.1, 0.5);
    menu->AddToggleItem("PID", makeSaveCallback<bool>("pid.enabled"), getMenuVar<bool>("pid.enabled"), bitmap_icon_pid);

    menu->SetEventHandler([&]() {
        if (xQueueReceive(button_events, &ev, 1 / portTICK_PERIOD_MS)) {
            if (ev.pin == menuEnterPin->getPinNumber()) {
                LOG(DEBUG, "Menu: Enter");

                if (ev.event == EventState::STATE_DOWN) {}
                menu->Event(EVENT_ENTER, EventState(ev.event));
            }
            else {
                if (activeMenuInput == MENUINPUT::BUTTONS) {
                    if (ev.pin == menuUpPin->getPinNumber()) {
                        menu->Event(EVENT_UP, EventState(ev.event));
                    }
                    else if (ev.pin == menuDownPin->getPinNumber()) {
                        menu->Event(EVENT_DOWN, EventState(ev.event));
                    }
                }
            }
        }
        if (activeMenuInput == MENUINPUT::ROTARY) {
            int32_t pos = encoder.getCount() / encoderDivisor;
            while (pos < last) {
                menu->Event(EVENT_UP, EventState(EventState::STATE_DOWN));
                LOG(DEBUG, "Menu: Up");
                menu->Event(EVENT_UP, EventState(EventState::STATE_UP));
                last--;
            }
            while (pos > last) {
                menu->Event(EVENT_DOWN, EventState(EventState::STATE_DOWN));
                LOG(DEBUG, "Menu: Down");
                menu->Event(EVENT_DOWN, EventState(EventState::STATE_UP));
                last++;
            }

            last = pos;
        }
    });

    Menu* m_pidMenu = new Menu(display);
    m_pidMenu->AddToggleItem("PID", makeSaveCallback<bool>("pid.enabled"), getMenuVar<bool>("pid.enabled"), bitmap_icon_pid);
    m_pidMenu->AddToggleItem("PonM", makeSaveCallback<bool>("pid.use_ponm"), getMenuVar<bool>("pid.use_ponm"));
    m_pidMenu->AddInputItem("EMA Factor", "EMA Factor", "", "", PID_EMA_FACTOR_MIN, PID_EMA_FACTOR_MAX, makeSaveCallback<double>("pid.ema_factor"), getMenuVar<double>("pid.ema_factor"));
    m_pidMenu->AddInputItem("Kp", "Kp", "", "", PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, makeSaveCallback<double>("pid.regular.kp"), getMenuVar<double>("pid.regular.kp"));
    m_pidMenu->AddInputItem("Tn", "Tn", "", "", PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, makeSaveCallback<double>("pid.regular.tn"), getMenuVar<double>("pid.regular.tn"));
    m_pidMenu->AddInputItem("Tv", "Tv", "", "", PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, makeSaveCallback<double>("pid.regular.tv"), getMenuVar<double>("pid.regular.tv"));
    m_pidMenu->AddInputItem("I Max", "I Max", "", "", PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, makeSaveCallback<double>("pid.regular.i_max"), getMenuVar<double>("pid.regular.i_max"));
    m_pidMenu->AddInputItem("Steam Kp", "Steam Kp", "", "", PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, makeSaveCallback<double>("pid.steam.kp"), getMenuVar<double>("pid.steam.kp"));

    m_pidMenu->AddBackItem("Back", bitmap_icon_back);

    Menu* m_temperature = new Menu(display);
    m_temperature->AddInputItem("Brew Temp.", "Brew Setpoint", "", "°C", BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, makeSaveCallback<double>("brew.setpoint"), getMenuVar<double>("brew.setpoint"), bitmap_icon_temp, 0.1, 0.5);
    m_temperature->AddInputItem("Temp Offset", "Temp Offset", "", "°C", BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, makeSaveCallback<double>("brew.temp_offset"), getMenuVar<double>("brew.temp_offset"), bitmap_icon_temp);
    m_temperature->AddInputItem("Steam Temp.", "Steam Setpoint", "", "s", STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, makeSaveCallback<double>("steam.setpoint"), getMenuVar<double>("steam.setpoint"), bitmap_icon_temp);
    m_temperature->AddBackItem("Back", bitmap_icon_back);

    // if (config.get<bool>("hardware.switches.brew.enabled"))
    Menu* m_brewPid = new Menu(display);
    m_brewPid->AddToggleItem("Brew PID", makeSaveCallback<bool>("pid.bd.enabled"), getMenuVar<bool>("pid.bd.enabled"), bitmap_icon_pid);
    m_brewPid->AddInputItem("Delay", "PID Delay", "", "s", BREW_PID_DELAY_MIN, BREW_PID_DELAY_MAX, makeSaveCallback<double>("brew.pid_delay"), getMenuVar<double>("brew.pid_delay"), bitmap_icon_pid);
    m_brewPid->AddInputItem("Kp", "Kp", "", "", PID_KP_BD_MIN, PID_KP_BD_MAX, makeSaveCallback<double>("pid.bd.kp"), getMenuVar<double>("pid.bd.kp"));
    m_brewPid->AddInputItem("Tn", "Tn", "", "", PID_TN_BD_MIN, PID_TN_BD_MAX, makeSaveCallback<double>("pid.bd.tn"), getMenuVar<double>("pid.bd.tn"));
    m_brewPid->AddInputItem("Tv", "Tv", "", "", PID_TV_BD_MIN, PID_TV_BD_MAX, makeSaveCallback<double>("pid.bd.tv"), getMenuVar<double>("pid.bd.tv"));

    m_brewPid->AddBackItem("Back", bitmap_icon_back);

    Menu* m_preInfusion = new Menu(display);
    m_preInfusion->AddToggleItem("Pre Infusion", makeSaveCallback<bool>("brew.pre_infusion.enabled"), getMenuVar<bool>("brew.pre_infusion.enabled"));
    m_preInfusion->AddInputItem("Time", "Time", "", "s", PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, makeSaveCallback<double>("brew.pre_infusion.time"), getMenuVar<double>("brew.pre_infusion.time"));
    m_preInfusion->AddInputItem("Pause", "Pause", "", "s", PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, makeSaveCallback<double>("brew.pre_infusion.pause"), getMenuVar<double>("brew.pre_infusion.pause"));

    m_preInfusion->AddBackItem("Back", bitmap_icon_back);

    // if (config.get<bool>("hardware.switches.brew.enabled"))
    Menu* m_brewControl = new Menu(display);
    m_brewControl->AddEnumItem("Brew Mode", "Brew Mode", getMenuEnumOptions("brew.mode"), getMenuEnumCount("brew.mode"), getMenuVar<uint8_t>("brew.mode"), makeSaveCallback<uint8_t>("brew.mode"));
    m_brewControl->AddToggleItem("By Time", makeSaveCallback<bool>("brew.by_time.enabled"), getMenuVar<bool>("brew.by_time.enabled"), bitmap_icon_clock, config.get<int>("brew.mode") == 1);
    m_brewControl->AddInputItem("Target Time", "Target Time", "", "s", TARGET_BREW_TIME_MIN, TARGET_BREW_TIME_MAX, makeSaveCallback<double>("brew.by_time.target_time"), getMenuVar<double>("brew.by_time.target_time"), 0.1,
                                0.5, false, config.get<int>("brew.mode") == 1);
    m_brewControl->AddToggleItem("By Weight", makeSaveCallback<bool>("brew.by_weight.enabled"), getMenuVar<bool>("brew.by_weight.enabled"), bitmap_icon_scale, config.get<int>("brew.mode") == 1);
    m_brewControl->AddInputItem("Target Weight", "Target Weight", "", "g", TARGET_BREW_WEIGHT_MIN, TARGET_BREW_WEIGHT_MAX, makeSaveCallback<double>("brew.by_weight.target_weight"),
                                getMenuVar<double>("brew.by_weight.target_weight"), 0.1, 0.5, false, config.get<int>("brew.mode") == 1);
    m_brewControl->AddToggleItem("Auto Tare", makeSaveCallback<bool>("brew.by_weight.auto_tare"), getMenuVar<bool>("brew.by_weight.auto_tare"),
                                 config.get<int>("brew.mode") == 1 && config.get<int>("hardware.sensors.scale.type") == 2);

    m_brewControl->AddSubMenu("Pre Infusion", *m_preInfusion);

    m_brewControl->AddBackItem("Back", bitmap_icon_back);

    // if (config.get<bool>("hardware.switches.brew.enabled"))
    Menu* m_backflushing = new Menu(display);
    m_backflushing->AddInputItem("Cycles", "Cycles", "", "", BACKFLUSH_CYCLES_MIN, BACKFLUSH_CYCLES_MAX, makeSaveCallback<double>("brew.backflushing.cycles"), getMenuVar<double>("brew.backflushing.cycles"));
    m_backflushing->AddInputItem("Fill Time", "Fill Time", "", "s", BACKFLUSH_FILL_TIME_MIN, BACKFLUSH_FILL_TIME_MAX, makeSaveCallback<double>("brew.backflushing.fill_time"),
                                 getMenuVar<double>("brew.backflushing.fill_time"));
    m_backflushing->AddInputItem("Flush Time", "Flush Time", "", "s", BACKFLUSH_FLUSH_TIME_MIN, BACKFLUSH_FLUSH_TIME_MAX, makeSaveCallback<double>("brew.backflushing.flush_time"),
                                 getMenuVar<double>("brew.backflushing.flush_time"));
    m_backflushing->AddBackItem("Back", bitmap_icon_back);

    Menu* m_standby = new Menu(display);
    m_standby->AddToggleItem("Standby", makeSaveCallback<bool>("standby.enabled"), getMenuVar<bool>("standby.enabled"));
    m_standby->AddInputItem("Time", "Time", "", "m", STANDBY_MODE_TIME_MIN, STANDBY_MODE_TIME_MAX, makeSaveCallback<double>("standby.time"), getMenuVar<double>("standby.time"));
    m_standby->AddBackItem("Back", bitmap_icon_back);

    Menu* m_display = new Menu(display);
    m_display->AddEnumItem("Template", "Template", getMenuEnumOptions("display.template"), getMenuEnumCount("display.template"), getMenuVar<uint8_t>("display.template"), makeSaveCallback<uint8_t>("display.template"))
        ->SetConfirm("Change need reboot!", reboot);
    m_display->AddToggleItem("Invert", makeSaveCallback<bool>("display.inverted"), getMenuVar<bool>("display.inverted"))->SetConfirm("Change need reboot!", reboot);
    m_display->AddEnumItem("Language", "Language", getMenuEnumOptions("display.language"), getMenuEnumCount("display.language"), getMenuVar<uint8_t>("display.language"), makeSaveCallback<uint8_t>("display.language"))
        ->SetConfirm("Change need reboot!", reboot);

    m_display->AddToggleItem("Fullscreen Brew Timer", makeSaveCallback<bool>("display.fullscreen_brew_timer"), getMenuVar<bool>("display.fullscreen_brew_timer"));
    m_display->AddToggleItem("Fullscreen Hot Water Timer", makeSaveCallback<bool>("display.fullscreen_hot_water_timer"), getMenuVar<bool>("display.fullscreen_hot_water_timer"));
    m_display->AddInputItem("Post Brew Timer Duration", "Post Brew Timer Duration", "", "", POST_BREW_TIMER_DURATION_MIN, POST_BREW_TIMER_DURATION_MAX, makeSaveCallback<double>("display.post_brew_timer_duration"),
                            getMenuVar<double>("display.post_brew_timer_duration"));
    m_display->AddToggleItem("BLE Scale Brew Timer", makeSaveCallback<bool>("display.blescale_brew_timer"), getMenuVar<bool>("display.blescale_brew_timer"));

    m_display->AddToggleItem("Heating Logo", makeSaveCallback<bool>("display.heating_logo"), getMenuVar<bool>("display.heating_logo"));

    m_display->AddToggleItem("Fullscreen Manual Flush Timer", makeSaveCallback<bool>("display.fullscreen_manual_flush_timer"), getMenuVar<bool>("display.fullscreen_manual_flush_timer"));
    m_display->AddEnumItem("Blinking Mode", "Blinking Mode", getMenuEnumOptions("display.blinking.mode"), getMenuEnumCount("display.blinking.mode"), getMenuVar<uint8_t>("display.blinking.mode"),
                           makeSaveCallback<int>("display.blinking.mode"));
    m_display->AddInputItem("Blinking Delta", "Delta", "", "", BLINKING_DELTA_MIN, BLINKING_DELTA_MAX, makeSaveCallback<double>("display.blinking.delta"), getMenuVar<double>("display.blinking.delta"));
    m_display->AddBackItem("Back", bitmap_icon_back);

    Menu* m_menu = new Menu(display);
    m_menu->AddToggleItem("Menu Idle", timeoutIdleActive, getMenuVar<bool>("display.menu.idle_timeout.enabled"));
    m_menu->AddInputItem("Menu Idle Time", "Menu Idle Time", "", "s", 1, 600, timeoutIdleTimeout, getMenuVar<double>("display.menu.idle_timeout.time"), 1.0, 10.0, true);
    m_menu->AddToggleItem("Invert Scroll", scrollInvertActive, getMenuVar<bool>("display.scroll.invert"));
    m_menu->AddToggleItem("Invert Input", inputInvertActive, getMenuVar<bool>("display.input.invert"));
    m_menu->AddBackItem("Back", bitmap_icon_back);

    Menu* m_hw_relay = new Menu(display);
    m_hw_relay
        ->AddEnumItem("Heater Trigger", "Heater Trigger", getMenuEnumOptions("hardware.relays.heater.trigger_type"), getMenuEnumCount("hardware.relays.heater.trigger_type"),
                      getMenuVar<uint8_t>("hardware.relays.heater.trigger_type"), makeSaveCallback<uint8_t>("hardware.relays.heater.trigger_type"))
        ->SetConfirm("Change need reboot!", reboot);
    m_hw_relay
        ->AddEnumItem("Valve Trigger", "Valve Trigger", getMenuEnumOptions("hardware.relays.valve.trigger_type"), getMenuEnumCount("hardware.relays.valve.trigger_type"),
                      getMenuVar<uint8_t>("hardware.relays.valve.trigger_type"), makeSaveCallback<uint8_t>("hardware.relays.valve.trigger_type"))
        ->SetConfirm("Change need reboot!", reboot);
    m_hw_relay
        ->AddEnumItem("Pump Trigger", "Pump Trigger", getMenuEnumOptions("hardware.relays.pump.trigger_type"), getMenuEnumCount("hardware.relays.pump.trigger_type"), getMenuVar<uint8_t>("hardware.relays.pump.trigger_type"),
                      makeSaveCallback<uint8_t>("hardware.relays.pump.trigger_type"))
        ->SetConfirm("Change need reboot!", reboot);
    m_hw_relay->AddBackItem("Back", bitmap_icon_back);

    Menu* m_hw_switch = new Menu(display);
    m_hw_switch->AddToggleItem("Brew Switch", makeSaveCallback("hardware.switches.brew.enabled", getMenuVar<bool>("hardware.switches.brew.enabled")), getMenuVar<bool>("hardware.switches.brew.enabled"))
        ->SetConfirm("Change need reboot!", reboot);
    m_hw_switch
        ->AddEnumItem("Brew Switch Type", "Brew Switch Type", getMenuEnumOptions("hardware.switches.brew.type"), getMenuEnumCount("hardware.switches.brew.type"), getMenuVar<uint8_t>("hardware.switches.brew.type"),
                      makeSaveCallback<uint8_t>("hardware.switches.brew.type"))
        ->SetConfirm("Change need reboot!", reboot);
    m_hw_switch
        ->AddEnumItem("Brew Switch Mode", "Brew Switch Mode", getMenuEnumOptions("hardware.switches.brew.mode"), getMenuEnumCount("hardware.switches.brew.mode"), getMenuVar<uint8_t>("hardware.switches.brew.mode"),
                      makeSaveCallback<uint8_t>("hardware.switches.brew.mode"))
        ->SetConfirm("Change need reboot!", reboot);
    m_hw_switch->AddToggleItem("Steam Switch", makeSaveCallback("hardware.switches.steam.enabled", getMenuVar<bool>("hardware.switches.steam.enabled")), getMenuVar<bool>("hardware.switches.steam.enabled"))
        ->SetConfirm("Change need reboot!", reboot);
    m_hw_switch
        ->AddEnumItem("Steam Switch Type", "Steam Switch Type", getMenuEnumOptions("hardware.switches.steam.type"), getMenuEnumCount("hardware.switches.steam.type"), getMenuVar<uint8_t>("hardware.switches.steam.type"),
                      makeSaveCallback<uint8_t>("hardware.switches.steam.type"))
        ->SetConfirm("Change need reboot!", reboot);
    m_hw_switch
        ->AddEnumItem("Steam Switch Mode", "Steam Switch Mode", getMenuEnumOptions("hardware.switches.steam.mode"), getMenuEnumCount("hardware.switches.steam.mode"), getMenuVar<uint8_t>("hardware.switches.steam.mode"),
                      makeSaveCallback<uint8_t>("hardware.switches.steam.mode"))
        ->SetConfirm("Change need reboot!", reboot);
    m_hw_switch->AddToggleItem("Power Switch", makeSaveCallback("hardware.switches.power.enabled", getMenuVar<bool>("hardware.switches.power.enabled")), getMenuVar<bool>("hardware.switches.power.enabled"));
    m_hw_switch
        ->AddEnumItem("Power Switch Type", "Power Switch Type", getMenuEnumOptions("hardware.switches.power.type"), getMenuEnumCount("hardware.switches.power.type"), getMenuVar<uint8_t>("hardware.switches.power.type"),
                      makeSaveCallback<uint8_t>("hardware.switches.power.type"))
        ->SetConfirm("Change need reboot!", reboot);
    m_hw_switch
        ->AddEnumItem("Power Switch Mode", "Power Switch Mode", getMenuEnumOptions("hardware.switches.power.mode"), getMenuEnumCount("hardware.switches.power.mode"), getMenuVar<uint8_t>("hardware.switches.power.mode"),
                      makeSaveCallback<uint8_t>("hardware.switches.power.mode"))
        ->SetConfirm("Change need reboot!", reboot);
    m_hw_switch->AddToggleItem("Hot Water Switch", makeSaveCallback("hardware.switches.hotwater.enabled", getMenuVar<bool>("hardware.switches.hotwater.enabled")), getMenuVar<bool>("hardware.switches.hotwater.enabled"))
        ->SetConfirm("Change need reboot!", reboot);
    m_hw_switch
        ->AddEnumItem("Hot Water Switch Type", "Hot Water Switch Type", getMenuEnumOptions("hardware.switches.hotwater.type"), getMenuEnumCount("hardware.switches.hotwater.type"),
                      getMenuVar<uint8_t>("hardware.switches.hotwater.type"), makeSaveCallback<uint8_t>("hardware.switches.hotwater.type"))
        ->SetConfirm("Change need reboot!", reboot);
    m_hw_switch
        ->AddEnumItem("Hot Water Switch Mode", "Hot Water Switch Mode", getMenuEnumOptions("hardware.switches.hotwater.mode"), getMenuEnumCount("hardware.switches.hotwater.mode"),
                      getMenuVar<uint8_t>("hardware.switches.hotwater.mode"), makeSaveCallback<uint8_t>("hardware.switches.hotwater.mode"))
        ->SetConfirm("Change need reboot!", reboot);
    m_hw_switch->AddBackItem("Back", bitmap_icon_back);

    Menu* m_leds = new Menu(display);
    m_leds->AddToggleItem("Status LED", makeSaveCallback<bool>("hardware.leds.status.enabled"), getMenuVar<bool>("hardware.leds.status.enabled"))->SetConfirm("Change need reboot!", reboot);
    m_leds->AddToggleItem("Status LED Invert", makeSaveCallback<bool>("hardware.leds.status.invert"), getMenuVar<bool>("hardware.leds.status.invert"))->SetConfirm("Change need reboot!", reboot);
    m_leds->AddToggleItem("Brew LED", makeSaveCallback<bool>("hardware.leds.brew.enabled"), getMenuVar<bool>("hardware.leds.brew.enabled"))->SetConfirm("Change need reboot!", reboot);
    m_leds->AddToggleItem("Brew LED Invert", makeSaveCallback<bool>("hardware.leds.brew.invert"), getMenuVar<bool>("hardware.leds.brew.invert"))->SetConfirm("Change need reboot!", reboot);
    m_leds->AddToggleItem("Steam LED", makeSaveCallback<bool>("hardware.leds.steam.enabled"), getMenuVar<bool>("hardware.leds.steam.enabled"))->SetConfirm("Change need reboot!", reboot);
    m_leds->AddToggleItem("Steam LED Invert", makeSaveCallback<bool>("hardware.leds.steam.invert"), getMenuVar<bool>("hardware.leds.steam.invert"))->SetConfirm("Change need reboot!", reboot);

    m_leds->AddBackItem("Back", bitmap_icon_back);

    Menu* m_sensors = new Menu(display);
    m_sensors
        ->AddEnumItem("Temp. Sensor Type", "Temp. Sensor Type", getMenuEnumOptions("hardware.sensors.temperature.type"), getMenuEnumCount("hardware.sensors.temperature.type"),
                      getMenuVar<uint8_t>("hardware.sensors.temperature.type"), makeSaveCallback<uint8_t>("hardware.sensors.temperature.type"))
        ->SetConfirm("Change need reboot!", reboot);
    m_sensors->AddToggleItem("Pressure Sensor", makeSaveCallback("hardware.sensors.pressure.enabled", getMenuVar<bool>("hardware.sensors.pressure.enabled")), getMenuVar<bool>("hardware.sensors.pressure.enabled"))
        ->SetConfirm("Change need reboot!", reboot);
    m_sensors->AddToggleItem("Watertank Sensor", makeSaveCallback("hardware.sensors.watertank.enabled", getMenuVar<bool>("hardware.sensors.watertank.enabled")), getMenuVar<bool>("hardware.sensors.watertank.enabled"))
        ->SetConfirm("Change need reboot!", reboot);
    m_sensors->AddEnumItem("Watertank Mode", "Watertank Mode", getMenuEnumOptions("hardware.sensors.watertank.mode"), getMenuEnumCount("hardware.sensors.watertank.mode"),
                           getMenuVar<uint8_t>("hardware.sensors.watertank.mode"), makeSaveCallback<uint8_t>("hardware.sensors.watertank.mode"));
    m_sensors->AddToggleItem("Scale", makeSaveCallback("hardware.sensors.scale.enabled", getMenuVar<bool>("hardware.sensors.scale.enabled")), getMenuVar<bool>("hardware.sensors.scale.enabled"));
    m_sensors
        ->AddEnumItem("Scale Type", "Scale Type", getMenuEnumOptions("hardware.sensors.scale.type"), getMenuEnumCount("hardware.sensors.scale.type"), getMenuVar<uint8_t>("hardware.sensors.scale.type"),
                      makeSaveCallback<uint8_t>("hardware.sensors.scale.type"))
        ->SetConfirm("Change need reboot!", reboot);
    m_sensors
        ->AddInputItem("Scale Samples", "Samples", "", "", SCALE_SAMPLES_MIN, SCALE_SAMPLES_MAX, makeSaveCallback<int>("hardware.sensors.scale.samples"), getMenuVar<double>("hardware.sensors.scale.samples"), 1.0, 5.0, true)
        ->SetConfirm("Change need reboot!", reboot);
    m_sensors->AddInputItem("Scale Calibration", "Calibration", "", "", SCALE_CALIBRATION_MIN, SCALE_CALIBRATION_MAX, makeSaveCallback<int>("hardware.sensors.scale.calibration"),
                            getMenuVar<double>("hardware.sensors.scale.calibration"), 0.0, 5.0, true);
    m_sensors->AddInputItem("Scale Calibration2", "Calibration2", "", "", SCALE_CALIBRATION_MIN, SCALE_CALIBRATION_MAX, makeSaveCallback<double>("hardware.sensors.scale.calibration2"),
                            getMenuVar<double>("hardware.sensors.scale.calibration2"), 0.0, 5.0, true);
    m_sensors->AddInputItem("Scale known Weight", "Known Weight", "", "g", SCALE_KNOWN_WEIGHT_MIN, SCALE_KNOWN_WEIGHT_MAX, makeSaveCallback<double>("hardware.sensors.scale.known_weight"),
                            getMenuVar<double>("hardware.sensors.scale.known_weight"), 0.0, 5.0, true);

    m_sensors->AddBackItem("Back", bitmap_icon_back);

    Menu* m_mqtt = new Menu(display);
    m_mqtt->AddToggleItem("MQTT", makeSaveCallback<bool>("mqtt.enabled"), getMenuVar<bool>("mqtt.enabled"))->SetConfirm("Change need reboot!", reboot);
    m_mqtt->AddToggleItem("Hass.io", makeSaveCallback<bool>("mqtt.hassio.enabled"), getMenuVar<bool>("mqtt.hassio.enabled"))->SetConfirm("Change need reboot!", reboot);

    std::vector<MenuInfoEntry> mqttInfo = {{"Broker", getMenuInfoVar("mqtt.broker")}, {"Port", getMenuInfoVar("mqtt.port")},   {"User", getMenuInfoVar("mqtt.username")},
                                           {"Pass", getMenuInfoVar("mqtt.password")}, {"Topic", getMenuInfoVar("mqtt.topic")}, {"Prefix", getMenuInfoVar("mqtt.hassio.prefix")}};
    m_mqtt->AddInfoItem("Info", mqttInfo);
    m_mqtt->AddBackItem("Back", bitmap_icon_back);

    Menu* m_system_debug = new Menu(display);
    m_system_debug->AddToggleItem("Loop Timing", makeSaveCallback<bool>("system.timing_debug.enabled"), getMenuVar<bool>("system.timing_debug.enabled"));
    m_system_debug->AddToggleItem("Display Log", makeSaveCallback<bool>("system.showdisplay.enabled"), getMenuVar<bool>("system.showdisplay.enabled"));
    m_system_debug->AddBackItem("Back", bitmap_icon_back);

    std::vector<MenuInfoEntry> systemInfo = {
        {"Hostname", getMenuInfoVar("system.hostname")},
        {"OTA Pass", getMenuInfoVar("system.ota_password")},
        {"Auth User", getMenuInfoVar("system.auth.username")},
        {"Auth Pass", getMenuInfoVar("system.auth.password")},

    };

    Menu* m_system = new Menu(display);
    m_system->AddToggleItem("Offline Mode", makeSaveCallback<bool>("system.offline_mode", getMenuVar<bool>("system.offline_mode")), getMenuVar<bool>("system.offline_mode"));
    m_system->AddToggleItem("Auth", makeSaveCallback<bool>("system.auth.enabled", getMenuVar<bool>("system.auth.enabled")), getMenuVar<bool>("system.auth.enabled"));
    m_system->AddEnumItem("Log Level", "LogLevel", getMenuEnumOptions("system.log_level"), getMenuEnumCount("system.log_level"), getMenuVar<uint8_t>("system.log_level"), makeSaveCallback<uint8_t>("system.log_level"));
    m_system->AddSubMenu("Debug", *m_system_debug, {config.get<int>("system.log_level") == static_cast<int>(Logger::Level::DEBUG)});
    m_system->AddInfoItem("Info", systemInfo);
    m_system->AddBackItem("Back", bitmap_icon_back);

    Menu* m_pid_settings = new Menu(display);
    m_pid_settings->AddSubMenu("General", *m_pidMenu);
    m_pid_settings->AddSubMenu("Brew Detection", *m_brewPid, config.get<bool>("hardware.switches.brew.enabled"));
    m_pid_settings->AddBackItem("Back", bitmap_icon_back);

    Menu* m_hardware = new Menu(display);

    m_hardware->AddSubMenu("Relays", *m_hw_relay);
    m_hardware->AddSubMenu("Switches", *m_hw_switch);
    m_hardware->AddSubMenu("LEDs", *m_leds);
    m_hardware->AddSubMenu("Sensors", *m_sensors);
    m_hardware->AddBackItem("Back", bitmap_icon_back);

    /* Build Menu */
    menu->AddSubMenu("PID Settings", *m_pid_settings, bitmap_icon_pid);
    menu->AddSubMenu("Temperature", *m_temperature, bitmap_icon_temp);
    menu->AddSubMenu("Brew PID", *m_brewPid, bitmap_icon_pid);
    menu->AddSubMenu("Brew Control", *m_brewControl, config.get<bool>("hardware.switches.brew.enabled"));
    menu->AddSubMenu("Display", *m_display);
    menu->AddSubMenu("Menu", *m_menu);
    menu->AddSubMenu("Maintenance", *m_backflushing, bitmap_icon_tools, config.get<bool>("hardware.switches.brew.enabled"));
    menu->AddSubMenu("Standby", *m_standby, bitmap_icon_power);
    menu->AddSubMenu("MQTT", *m_mqtt);
    menu->AddSubMenu("System", *m_system);
    menu->AddSubMenu("Hardware", *m_hardware);

    menu->AddBackItem("Back", bitmap_icon_back);

    menu->Init();
}

void menuLoop() {
    static bool wasOpen = false;

    menu->EventHandler();

    if (menu->IsOpen()) {
        u8g2->setPowerSave(0);
        if (!wasOpen) {
            LOG(DEBUG, "sync values");
            syncMenuVars();
            timeoutIdleActive();
            timeoutIdleTimeout();
            wasOpen = true;
        }
        u8g2->clearBuffer();
    }
    else {
        wasOpen = false;
    }

    menu->Loop();
}
