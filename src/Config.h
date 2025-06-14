
/**
 * @file Config.h
 *
 * @brief Centralized configuration management with JSON storage
 */

#pragma once

#include "Logger.h"
#include "defaults.h"
#include "hardware/Relay.h"
#include "hardware/Switch.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

class Config {
    public:
        /**
         * @brief Initialize the configuration system
         *
         * @return true if successful, false otherwise
         */
        bool begin() {
            if (!LittleFS.begin(true)) {
                LOG(ERROR, "Failed to initialize LittleFS");
                return false;
            }

            // Check if config file exists
            if (!LittleFS.exists(CONFIG_FILE)) {
                LOG(INFO, "Config file not found, creating from defaults");
                createDefaults();

                return save();
            }

            // Try to load existing config
            if (!load()) {
                LOG(WARNING, "Failed to load config, creating from defaults");
                createDefaults();

                return save();
            }

            return true;
        }

        /**
         * @brief Create a new configuration with default values
         */
        void createDefaults() {
            // Clear the document
            _doc.clear();

            // PID general
            _doc["pid"]["enabled"] = false;
            _doc["pid"]["use_ponm"] = false;
            _doc["pid"]["ema_factor"] = EMA_FACTOR;

            // PID regular
            _doc["pid"]["regular"]["kp"] = AGGKP;
            _doc["pid"]["regular"]["tn"] = AGGTN;
            _doc["pid"]["regular"]["tv"] = AGGTV;
            _doc["pid"]["regular"]["i_max"] = AGGIMAX;

            // PID brew detection
            _doc["pid"]["bd"]["enabled"] = false;
            _doc["pid"]["bd"]["kp"] = AGGBKP;
            _doc["pid"]["bd"]["tn"] = AGGBTN;
            _doc["pid"]["bd"]["tv"] = AGGBTV;

            // PID steam
            _doc["pid"]["steam"]["kp"] = STEAMKP;

            // Brew settings
            _doc["brew"]["setpoint"] = SETPOINT;
            _doc["brew"]["temp_offset"] = TEMPOFFSET;
            _doc["brew"]["pid_delay"] = BREW_PID_DELAY;
            _doc["brew"]["target_time"] = TARGET_BREW_TIME;
            _doc["brew"]["target_weight"] = TARGET_BREW_WEIGHT;

            // Pre-infusion
            _doc["brew"]["pre_infusion"]["time"] = PRE_INFUSION_TIME;
            _doc["brew"]["pre_infusion"]["pause"] = PRE_INFUSION_PAUSE_TIME;

            // Steam
            _doc["steam"]["setpoint"] = STEAMSETPOINT;

            // Backflushing
            _doc["backflush"]["cycles"] = BACKFLUSH_CYCLES;
            _doc["backflush"]["fill_time"] = BACKFLUSH_FILL_TIME;
            _doc["backflush"]["flush_time"] = BACKFLUSH_FLUSH_TIME;

            // Standby
            _doc["standby"]["enabled"] = false;
            _doc["standby"]["time"] = STANDBY_MODE_TIME;

            // Features
            _doc["features"]["brew_control"] = false;

            // MQTT
            _doc["mqtt"]["enabled"] = false;
            _doc["mqtt"]["broker"] = "";
            _doc["mqtt"]["port"] = 1883;
            _doc["mqtt"]["username"] = MQTT_USERNAME;
            _doc["mqtt"]["password"] = MQTT_PASSWORD;
            _doc["mqtt"]["topic"] = MQTT_TOPIC;
            _doc["mqtt"]["hassio"]["enabled"] = false;
            _doc["mqtt"]["hassio"]["prefix"] = MQTT_HASSIO_PREFIX;

            // System
            _doc["system"]["hostname"] = HOSTNAME;
            _doc["system"]["ota_password"] = OTAPASS;
            _doc["system"]["log_level"] = static_cast<int>(Logger::Level::INFO);

            // Display
            _doc["display"]["template"] = 0;
            _doc["display"]["fullscreen_brew_timer"] = false;
            _doc["display"]["fullscreen_manual_flush_timer"] = false;
            _doc["display"]["post_brew_timer_duration"] = POST_BREW_TIMER_DURATION;
            _doc["display"]["heating_logo"] = true;
            _doc["display"]["pid_off_logo"] = true;

            // Hardware
            _doc["hardware"]["relays"]["heater"]["trigger_type"] = Relay::HIGH_TRIGGER;
            _doc["hardware"]["relays"]["pump_valve"]["trigger_type"] = Relay::HIGH_TRIGGER;

            _doc["hardware"]["switches"]["brew"]["enabled"] = false;
            _doc["hardware"]["switches"]["brew"]["type"] = static_cast<int>(Switch::TOGGLE);
            _doc["hardware"]["switches"]["brew"]["mode"] = static_cast<int>(Switch::NORMALLY_OPEN);
            _doc["hardware"]["switches"]["steam"]["enabled"] = false;
            _doc["hardware"]["switches"]["steam"]["type"] = static_cast<int>(Switch::TOGGLE);
            _doc["hardware"]["switches"]["steam"]["mode"] = static_cast<int>(Switch::NORMALLY_OPEN);
            _doc["hardware"]["switches"]["power"]["enabled"] = false;
            _doc["hardware"]["switches"]["power"]["type"] = static_cast<int>(Switch::TOGGLE);
            _doc["hardware"]["switches"]["power"]["mode"] = static_cast<int>(Switch::NORMALLY_OPEN);

            _doc["hardware"]["leds"]["status"]["enabled"] = false;
            _doc["hardware"]["leds"]["status"]["inverted"] = false;
            _doc["hardware"]["leds"]["brew"]["enabled"] = false;
            _doc["hardware"]["leds"]["brew"]["inverted"] = false;
            _doc["hardware"]["leds"]["steam"]["enabled"] = false;
            _doc["hardware"]["leds"]["steam"]["inverted"] = false;

            _doc["hardware"]["sensors"]["pressure"]["enabled"] = false;

            _doc["hardware"]["sensors"]["watertank"]["enabled"] = false;
            _doc["hardware"]["sensors"]["watertank"]["mode"] = static_cast<int>(Switch::NORMALLY_CLOSED);

            // Scale
            _doc["hardware"]["sensors"]["scale"]["enabled"] = false;
            _doc["hardware"]["sensors"]["scale"]["samples"] = SCALE_SAMPLES;
            _doc["hardware"]["sensors"]["scale"]["type"] = 0;
            _doc["hardware"]["sensors"]["scale"]["calibration"] = SCALE_CALIBRATION_FACTOR;
            _doc["hardware"]["sensors"]["scale"]["calibration2"] = SCALE_CALIBRATION_FACTOR;
            _doc["hardware"]["sensors"]["scale"]["known_weight"] = SCALE_KNOWN_WEIGHT;

            // WiFi credentials flag
            _doc["wifi"]["credentials_saved"] = false;
        }

        /**
         * @brief Load configuration from file
         *
         * @return true if successful, false otherwise
         */
        bool load() {
            if (!LittleFS.exists(CONFIG_FILE)) {
                LOG(INFO, "Config file does not exist");

                return false;
            }

            File file = LittleFS.open(CONFIG_FILE, "r");

            if (!file) {
                LOG(ERROR, "Failed to open config file for reading");

                return false;
            }

            const DeserializationError error = deserializeJson(_doc, file);
            file.close();

            if (error) {
                LOG(ERROR, "Failed to parse config file");
                return false;
            }

            LOG(INFO, "Configuration loaded successfully");

            return true;
        }

        /**
         * @brief Save configuration to file
         *
         * @return true if successful, false otherwise
         */
        [[nodiscard]] bool save() const {
            File file = LittleFS.open(CONFIG_FILE, "w");

            if (!file) {
                LOG(ERROR, "Failed to open config file for writing");
                return false;
            }

            if (serializeJson(_doc, file) == 0) {
                LOG(ERROR, "Failed to write config to file");
                file.close();
                return false;
            }

            file.close();
            LOG(INFO, "Configuration saved successfully");

            return true;
        }

        /**
         * @brief Reset configuration to factory defaults
         *
         * @return true if successful, false otherwise
         */
        bool factoryReset() {
            createDefaults();
            return save();
        }

        bool validateAndApplyFromJson(const String& jsonString) {
            DynamicJsonDocument doc(4096);
            const DeserializationError error = deserializeJson(doc, jsonString);

            if (error) {
                LOGF(ERROR, "JSON parsing failed: %s", error.c_str());
                return false;
            }

            if (!validateAndApplyConfig(doc)) {
                return false;
            }

            return true;
        }

        // WiFi credentials flag
        bool getWifiCredentialsSaved() {
            return _doc["wifi"]["credentials_saved"] | false;
        }

        void setWifiCredentialsSaved(const bool value) {
            _doc["wifi"]["credentials_saved"] = value;
        }

        // PID general
        bool getPidEnabled() {
            return _doc["pid"]["enabled"] | false;
        }

        void setPidEnabled(const bool value) {
            _doc["pid"]["enabled"] = value;
        }

        bool getUsePonM() {
            return _doc["pid"]["use_ponm"] | false;
        }

        void setUsePonM(const bool value) {
            _doc["pid"]["use_ponm"] = value;
        }

        double getPidEmaFactor() {
            return _doc["pid"]["ema_factor"] | EMA_FACTOR;
        }

        void setPidEmaFactor(const double value) {
            _doc["pid"]["ema_factor"] = constrain(value, PID_EMA_FACTOR_MIN, PID_EMA_FACTOR_MAX);
        }

        // PID regular
        double getPidKpRegular() {
            return _doc["pid"]["regular"]["kp"] | AGGKP;
        }

        void setPidKpRegular(const double value) {
            _doc["pid"]["regular"]["kp"] = constrain(value, PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX);
        }

        double getPidTnRegular() {
            return _doc["pid"]["regular"]["tn"] | AGGTN;
        }

        void setPidTnRegular(const double value) {
            _doc["pid"]["regular"]["tn"] = constrain(value, PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX);
        }

        double getPidTvRegular() {
            return _doc["pid"]["regular"]["tv"] | AGGTV;
        }

        void setPidTvRegular(const double value) {
            _doc["pid"]["regular"]["tv"] = constrain(value, PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX);
        }

        double getPidIMaxRegular() {
            return _doc["pid"]["regular"]["i_max"] | AGGIMAX;
        }

        void setPidIMaxRegular(const double value) {
            _doc["pid"]["regular"]["i_max"] = constrain(value, PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX);
        }

        // PID brew detection
        bool getUseBDPID() {
            return _doc["pid"]["bd"]["enabled"] | false;
        }

        void setUseBDPID(const bool value) {
            _doc["pid"]["bd"]["enabled"] = value;
        }

        double getPidKpBD() {
            return _doc["pid"]["bd"]["kp"] | AGGBKP;
        }

        void setPidKpBD(const double value) {
            _doc["pid"]["bd"]["kp"] = constrain(value, PID_KP_BD_MIN, PID_KP_BD_MAX);
        }

        double getPidTnBD() {
            return _doc["pid"]["bd"]["tn"] | AGGBTN;
        }

        void setPidTnBD(const double value) {
            _doc["pid"]["bd"]["tn"] = constrain(value, PID_TN_BD_MIN, PID_TN_BD_MAX);
        }

        double getPidTvBD() {
            return _doc["pid"]["bd"]["tv"] | AGGBTV;
        }

        void setPidTvBD(const double value) {
            _doc["pid"]["bd"]["tv"] = constrain(value, PID_TV_BD_MIN, PID_TV_BD_MAX);
        }

        // PID steam
        double getPidKpSteam() {
            return _doc["pid"]["steam"]["kp"] | STEAMKP;
        }

        void setPidKpSteam(const double value) {
            _doc["pid"]["steam"]["kp"] = constrain(value, PID_KP_STEAM_MIN, PID_KP_STEAM_MAX);
        }

        // Brew settings
        double getBrewSetpoint() {
            return _doc["brew"]["setpoint"] | SETPOINT;
        }

        void setBrewSetpoint(const double value) {
            _doc["brew"]["setpoint"] = constrain(value, BREW_SETPOINT_MIN, BREW_SETPOINT_MAX);
        }

        double getBrewTempOffset() {
            return _doc["brew"]["temp_offset"] | TEMPOFFSET;
        }

        void setBrewTempOffset(const double value) {
            _doc["brew"]["temp_offset"] = constrain(value, BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX);
        }

        double getBrewPIDDelay() {
            return _doc["brew"]["pid_delay"] | BREW_PID_DELAY;
        }

        void setBrewPIDDelay(const double value) {
            _doc["brew"]["pid_delay"] = constrain(value, BREW_PID_DELAY_MIN, BREW_PID_DELAY_MAX);
        }

        double getTargetBrewTime() {
            return _doc["brew"]["target_time"] | TARGET_BREW_TIME;
        }

        void setTargetBrewTime(const double value) {
            _doc["brew"]["target_time"] = constrain(value, TARGET_BREW_TIME_MIN, TARGET_BREW_TIME_MAX);
        }

        double getTargetBrewWeight() {
            return _doc["brew"]["target_weight"] | TARGET_BREW_WEIGHT;
        }

        void setTargetBrewWeight(const double value) {
            _doc["brew"]["target_weight"] = constrain(value, TARGET_BREW_WEIGHT_MIN, TARGET_BREW_WEIGHT_MAX);
        }

        // Pre-infusion
        double getPreInfusionTime() {
            return _doc["brew"]["pre_infusion"]["time"] | PRE_INFUSION_TIME;
        }

        void setPreInfusionTime(const double value) {
            _doc["brew"]["pre_infusion"]["time"] = constrain(value, PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX);
        }

        double getPreInfusionPause() {
            return _doc["brew"]["pre_infusion"]["pause"] | PRE_INFUSION_PAUSE_TIME;
        }

        void setPreInfusionPause(const double value) {
            _doc["brew"]["pre_infusion"]["pause"] = constrain(value, PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX);
        }

        // Steam
        double getSteamSetpoint() {
            return _doc["steam"]["setpoint"] | STEAMSETPOINT;
        }

        void setSteamSetpoint(const double value) {
            _doc["steam"]["setpoint"] = constrain(value, STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX);
        }

        // Backflushing
        int getBackflushCycles() {
            return _doc["backflush"]["cycles"] | BACKFLUSH_CYCLES;
        }

        void setBackflushCycles(const int value) {
            _doc["backflush"]["cycles"] = constrain(value, BACKFLUSH_CYCLES_MIN, BACKFLUSH_CYCLES_MAX);
        }

        double getBackflushFillTime() {
            return _doc["backflush"]["fill_time"] | BACKFLUSH_FILL_TIME;
        }

        void setBackflushFillTime(const double value) {
            _doc["backflush"]["fill_time"] = constrain(value, BACKFLUSH_FILL_TIME_MIN, BACKFLUSH_FILL_TIME_MAX);
        }

        double getBackflushFlushTime() {
            return _doc["backflush"]["flush_time"] | BACKFLUSH_FLUSH_TIME;
        }

        void setBackflushFlushTime(const double value) {
            _doc["backflush"]["flush_time"] = constrain(value, BACKFLUSH_FLUSH_TIME_MIN, BACKFLUSH_FLUSH_TIME_MAX);
        }

        // Standby
        bool getStandbyModeOn() {
            return _doc["standby"]["enabled"] | false;
        }

        void setStandbyModeOn(const bool value) {
            _doc["standby"]["enabled"] = value;
        }

        double getStandbyModeTime() {
            return _doc["standby"]["time"] | STANDBY_MODE_TIME;
        }

        void setStandbyModeTime(const double value) {
            _doc["standby"]["time"] = constrain(value, STANDBY_MODE_TIME_MIN, STANDBY_MODE_TIME_MAX);
        }

        // Features
        bool getFeatureBrewControl() {
            return _doc["features"]["brew_control"] | false;
        }

        void setFeatureBrewControl(const bool value) {
            _doc["features"]["brew_control"] = value;
        }

        // MQTT
        bool getMqttEnabled() {
            return _doc["mqtt"]["enabled"] | false;
        }

        void setMqttEnabled(const bool value) {
            _doc["mqtt"]["enabled"] = value;
        }

        String getMqttBroker() {
            return _doc["mqtt"]["broker"];
        }

        void setMqttBroker(const String& value) {
            _doc["mqtt"]["broker"] = constrainStringParameter(value, MQTT_BROKER_MAX_LENGTH, "mqtt.broker");
        }

        int getMqttPort() {
            return _doc["mqtt"]["port"];
        }

        void setMqttPort(const int value) {
            _doc["mqtt"]["port"] = value;
        }

        String getMqttUsername() {
            return _doc["mqtt"]["username"];
        }

        void setMqttUsername(const String& value) {
            _doc["mqtt"]["username"] = constrainStringParameter(value, MQTT_USERNAME_MAX_LENGTH, "mqtt.username");
        }

        String getMqttPassword() {
            return _doc["mqtt"]["password"];
        }

        void setMqttPassword(const String& value) {
            _doc["mqtt"]["password"] = constrainStringParameter(value, MQTT_PASSWORD_MAX_LENGTH, "mqtt.password");
        }

        String getMqttTopic() {
            return _doc["mqtt"]["topic"];
        }

        void setMqttTopic(const String& value) {
            _doc["mqtt"]["topic"] = constrainStringParameter(value, MQTT_TOPIC_MAX_LENGTH, "mqtt.topic");
        }

        bool getMqttHassioEnabled() {
            return _doc["mqtt"]["hassio"]["enabled"] | false;
        }

        void setMqttHassioEnabled(const bool value) {
            _doc["mqtt"]["hassio"]["enabled"] = value;
        }

        String getMqttHassioPrefix() {
            return _doc["mqtt"]["hassio"]["prefix"];
        }

        void setMqttHassioPrefix(const String& value) {
            _doc["mqtt"]["hassio"]["prefix"] = constrainStringParameter(value, MQTT_HASSIO_PREFIX_MAX_LENGTH, "mqtt.hassio.prefix");
        }

        // System
        String getHostname() {
            return _doc["system"]["hostname"];
        }

        void setHostname(const String& value) {
            _doc["system"]["hostname"] = constrainStringParameter(value, HOSTNAME_MAX_LENGTH, "system.hostname");
        }

        String getOtaPass() {
            return _doc["system"]["ota_password"];
        }

        void setOtaPass(const String& value) {
            _doc["system"]["ota_password"] = constrainStringParameter(value, OTAPASS_MAX_LENGTH, "system.ota_password");
        }

        int getLogLevel() {
            return _doc["system"]["log_level"];
        }

        void setLogLevel(int value) {
            value = constrain(value, 0, 6);
            _doc["system"]["log_level"] = value;
        }

        int getDisplayTemplate() {
            return _doc["display"]["template"];
        }

        void setDisplayTemplate(int value) {
            value = constrain(value, 0, 4);
            _doc["display"]["template"] = value;
        }

        bool getFeatureFullscreenBrewTimer() {
            return _doc["display"]["fullscreen_brew_timer"] | false;
        }

        void setFeatureFullscreenBrewTimer(const bool value) {
            _doc["display"]["fullscreen_brew_timer"] = value;
        }

        bool getFeatureFullscreenManualFlushTimer() {
            return _doc["display"]["fullscreen_manual_flush_timer"] | false;
        }

        void setFeatureFullscreenManualFlushTimer(const bool value) {
            _doc["display"]["fullscreen_manual_flush_timer"] = value;
        }

        double getPostBrewTimerDuration() {
            return _doc["display"]["post_brew_timer_duration"] | POST_BREW_TIMER_DURATION;
        }

        void setPostBrewTimerDuration(const double value) {
            _doc["display"]["post_brew_timer_duration"] = constrain(value, POST_BREW_TIMER_DURATION_MIN, POST_BREW_TIMER_DURATION_MAX);
        }

        bool getFeatureHeatingLogo() {
            return _doc["display"]["heating_logo"] | false;
        }

        void setFeatureHeatingLogo(const bool value) {
            _doc["display"]["heating_logo"] = value;
        }

        bool getFeaturePidOffLogo() {
            return _doc["display"]["pid_off_logo"] | false;
        }

        void setFeaturePidOffLogo(const bool value) {
            _doc["display"]["pid_off_logo"] = value;
        }

        // Hardware - Relays
        int getHeaterRelayTriggerType() {
            return _doc["hardware"]["relays"]["heater"]["trigger_type"];
        }

        void setHeaterRelayTriggerType(const int value) {
            _doc["hardware"]["relays"]["heater"]["trigger_type"] = value;
        }

        int getPumpValveRelayTriggerType() {
            return _doc["hardware"]["relays"]["pump_valve"]["trigger_type"];
        }

        void setPumpValveRelayTriggerType(const int value) {
            _doc["hardware"]["relays"]["pump_valve"]["trigger_type"] = value;
        }

        // Hardware - Switches
        bool getBrewSwitchEnabled() {
            return _doc["hardware"]["switches"]["brew"]["enabled"] | false;
        }

        void setBrewSwitchEnabled(const bool value) {
            _doc["hardware"]["switches"]["brew"]["enabled"] = value;
        }

        int getBrewSwitchType() {
            return _doc["hardware"]["switches"]["brew"]["type"];
        }

        void setBrewSwitchType(const int value) {
            _doc["hardware"]["switches"]["brew"]["type"] = value;
        }

        int getBrewSwitchMode() {
            return _doc["hardware"]["switches"]["brew"]["mode"];
        }

        void setBrewSwitchMode(const int value) {
            _doc["hardware"]["switches"]["brew"]["mode"] = value;
        }

        bool getSteamSwitchEnabled() {
            return _doc["hardware"]["switches"]["steam"]["enabled"] | false;
        }

        void setSteamSwitchEnabled(const bool value) {
            _doc["hardware"]["switches"]["steam"]["enabled"] = value;
        }

        int getSteamSwitchType() {
            return _doc["hardware"]["switches"]["steam"]["type"];
        }

        void setSteamSwitchType(const int value) {
            _doc["hardware"]["switches"]["steam"]["type"] = value;
        }

        int getSteamSwitchMode() {
            return _doc["hardware"]["switches"]["steam"]["mode"];
        }

        void setSteamSwitchMode(const int value) {
            _doc["hardware"]["switches"]["steam"]["mode"] = value;
        }

        bool getPowerSwitchEnabled() {
            return _doc["hardware"]["switches"]["power"]["enabled"] | false;
        }

        void setPowerSwitchEnabled(const bool value) {
            _doc["hardware"]["switches"]["power"]["enabled"] = value;
        }

        int getPowerSwitchType() {
            return _doc["hardware"]["switches"]["power"]["type"];
        }

        void setPowerSwitchType(const int value) {
            _doc["hardware"]["switches"]["power"]["type"] = value;
        }

        int getPowerSwitchMode() {
            return _doc["hardware"]["switches"]["power"]["mode"];
        }

        void setPowerSwitchMode(const int value) {
            _doc["hardware"]["switches"]["power"]["mode"] = value;
        }

        // Hardware - LEDs
        bool getStatusLedEnabled() {
            return _doc["hardware"]["leds"]["status"]["enabled"] | false;
        }

        void setStatusLedEnabled(const bool value) {
            _doc["hardware"]["leds"]["status"]["enabled"] = value;
        }

        bool getStatusLedInverted() {
            return _doc["hardware"]["leds"]["status"]["inverted"] | false;
        }

        void setStatusLedInverted(const bool value) {
            _doc["hardware"]["leds"]["status"]["inverted"] = value;
        }

        bool getBrewLedEnabled() {
            return _doc["hardware"]["leds"]["brew"]["enabled"] | false;
        }

        void setBrewLedEnabled(const bool value) {
            _doc["hardware"]["leds"]["brew"]["enabled"] = value;
        }

        bool getBrewLedInverted() {
            return _doc["hardware"]["leds"]["brew"]["inverted"] | false;
        }

        void setBrewLedInverted(const bool value) {
            _doc["hardware"]["leds"]["brew"]["inverted"] = value;
        }

        bool getSteamLedEnabled() {
            return _doc["hardware"]["leds"]["steam"]["enabled"] | false;
        }

        void setSteamLedEnabled(const bool value) {
            _doc["hardware"]["leds"]["steam"]["enabled"] = value;
        }

        bool getSteamLedInverted() {
            return _doc["hardware"]["leds"]["steam"]["inverted"] | false;
        }

        void setSteamLedInverted(const bool value) {
            _doc["hardware"]["leds"]["steam"]["inverted"] = value;
        }

        // Hardware - Sensors
        bool getPressureSensorEnabled() {
            return _doc["hardware"]["sensors"]["pressure"]["enabled"] | false;
        }

        void setPressureSensorEnabled(const bool value) {
            _doc["hardware"]["sensors"]["pressure"]["enabled"] = value;
        }

        bool getWaterTankSensorEnabled() {
            return _doc["hardware"]["sensors"]["watertank"]["enabled"] | false;
        }

        void setWaterTankSensorEnabled(const bool value) {
            _doc["hardware"]["sensors"]["watertank"]["enabled"] = value;
        }

        int getWaterTankSensorMode() {
            return _doc["hardware"]["sensors"]["watertank"]["mode"];
        }

        void setWaterTankSensorMode(const int value) {
            _doc["hardware"]["sensors"]["watertank"]["mode"] = value;
        }

        // Scale
        bool getScaleEnabled() {
            return _doc["hardware"]["sensors"]["scale"]["enabled"] | false;
        }

        void setScaleEnabled(const bool value) {
            _doc["hardware"]["sensors"]["scale"]["enabled"] = value;
        }

        int getScaleSamples() {
            return _doc["hardware"]["sensors"]["scale"]["samples"];
        }

        void setScaleSamples(const int value) {
            _doc["hardware"]["sensors"]["scale"]["samples"] = value;
        }

        int getScaleType() {
            return _doc["hardware"]["sensors"]["scale"]["type"];
        }

        void setScaleType(const int value) {
            _doc["hardware"]["sensors"]["scale"]["type"] = value;
        }

        float getScaleCalibration() {
            return _doc["hardware"]["sensors"]["scale"]["calibration"] | static_cast<float>(SCALE_CALIBRATION_FACTOR);
        }

        void setScaleCalibration(const float value) {
            _doc["hardware"]["sensors"]["scale"]["calibration"] = constrain(value, -100000.0f, 100000.0f);
        }

        void setScaleKnownWeight(const float value) {
            _doc["hardware"]["sensors"]["scale"]["known_weight"] = constrain(value, 0.0f, 2000.0f);
        }

        float getScaleKnownWeight() {
            return _doc["hardware"]["sensors"]["scale"]["known_weight"] | static_cast<float>(SCALE_KNOWN_WEIGHT);
        }

        float getScale2Calibration() {
            return _doc["hardware"]["sensors"]["scale"]["calibration2"] | static_cast<float>(SCALE_CALIBRATION_FACTOR);
        }

        void setScale2Calibration(const float value) {
            _doc["hardware"]["sensors"]["scale"]["calibration2"] = constrain(value, -100000.0f, 100000.0f);
        }

    private:
        inline static auto CONFIG_FILE = "/config.json";

        StaticJsonDocument<4096> _doc;

        bool validateAndApplyConfig(const JsonDocument& doc) {
            // PID parameters
            if (doc["pid"].containsKey("enabled")) {
                if (!doc["pid"]["enabled"].is<bool>()) {
                    return false;
                }

                setPidEnabled(doc["pid"]["enabled"].as<bool>());
            }

            if (doc["pid"].containsKey("use_ponm")) {
                if (!doc["pid"]["use_ponm"].is<bool>()) {
                    return false;
                }

                setUsePonM(doc["pid"]["use_ponm"].as<bool>());
            }

            if (doc["pid"].containsKey("ema_factor")) {
                const double value = doc["pid"]["ema_factor"].as<double>();

                if (!validateParameterRange("pid.ema_factor", value, PID_EMA_FACTOR_MIN, PID_EMA_FACTOR_MAX)) {
                    return false;
                }

                setPidEmaFactor(value);
            }

            // PID regular parameters
            if (doc["pid"]["regular"].containsKey("kp")) {
                const double value = doc["pid"]["regular"]["kp"].as<double>();

                if (!validateParameterRange("pid.regular.kp", value, PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX)) {
                    return false;
                }

                setPidKpRegular(value);
            }

            if (doc["pid"]["regular"].containsKey("tn")) {
                const double value = doc["pid"]["regular"]["tn"].as<double>();

                if (!validateParameterRange("pid.regular.tn", value, PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX)) {
                    return false;
                }

                setPidTnRegular(value);
            }

            if (doc["pid"]["regular"].containsKey("tv")) {
                const double value = doc["pid"]["regular"]["tv"].as<double>();

                if (!validateParameterRange("pid.regular.tv", value, PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX)) {
                    return false;
                }

                setPidTvRegular(value);
            }

            if (doc["pid"]["regular"].containsKey("i_max")) {
                const double value = doc["pid"]["regular"]["i_max"].as<double>();

                if (!validateParameterRange("pid.regular.i_max", value, PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX)) {
                    return false;
                }

                setPidIMaxRegular(value);
            }

            // PID brew detection parameters
            if (doc["pid"]["bd"].containsKey("enabled")) {
                if (!doc["pid"]["bd"]["enabled"].is<bool>()) {
                    return false;
                }

                setUseBDPID(doc["pid"]["bd"]["enabled"].as<bool>());
            }

            if (doc["pid"]["bd"].containsKey("kp")) {
                const double value = doc["pid"]["bd"]["kp"].as<double>();

                if (!validateParameterRange("pid.bd.kp", value, PID_KP_BD_MIN, PID_KP_BD_MAX)) {
                    return false;
                }

                setPidKpBD(value);
            }

            if (doc["pid"]["bd"].containsKey("tn")) {
                const double value = doc["pid"]["bd"]["tn"].as<double>();

                if (!validateParameterRange("pid.bd.tn", value, PID_TN_BD_MIN, PID_TN_BD_MAX)) {
                    return false;
                }

                setPidTnBD(value);
            }

            if (doc["pid"]["bd"].containsKey("tv")) {
                const double value = doc["pid"]["bd"]["tv"].as<double>();

                if (!validateParameterRange("pid.bd.tv", value, PID_TV_BD_MIN, PID_TV_BD_MAX)) {
                    return false;
                }

                setPidTvBD(value);
            }

            // PID steam parameters
            if (doc["pid"]["steam"].containsKey("kp")) {
                const double value = doc["pid"]["steam"]["kp"].as<double>();

                if (!validateParameterRange("pid.steam.kp", value, PID_KP_STEAM_MIN, PID_KP_STEAM_MAX)) {
                    return false;
                }

                setPidKpSteam(value);
            }

            // Brew parameters
            if (doc["brew"].containsKey("setpoint")) {
                const double value = doc["brew"]["setpoint"].as<double>();

                if (!validateParameterRange("brew.setpoint", value, BREW_SETPOINT_MIN, BREW_SETPOINT_MAX)) {
                    return false;
                }

                setBrewSetpoint(value);
            }

            if (doc["brew"].containsKey("temp_offset")) {
                const double value = doc["brew"]["temp_offset"].as<double>();

                if (!validateParameterRange("brew.temp_offset", value, BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX)) {
                    return false;
                }

                setBrewTempOffset(value);
            }

            if (doc["brew"].containsKey("pid_delay")) {
                const double value = doc["brew"]["pid_delay"].as<double>();

                if (!validateParameterRange("brew.pid_delay", value, BREW_PID_DELAY_MIN, BREW_PID_DELAY_MAX)) {
                    return false;
                }

                setBrewPIDDelay(value);
            }

            if (doc["brew"].containsKey("target_time")) {
                const double value = doc["brew"]["target_time"].as<double>();

                if (!validateParameterRange("brew.target_time", value, TARGET_BREW_TIME_MIN, TARGET_BREW_TIME_MAX)) {
                    return false;
                }

                setTargetBrewTime(value);
            }

            if (doc["brew"].containsKey("target_weight")) {
                const double value = doc["brew"]["target_weight"].as<double>();

                if (!validateParameterRange("brew.target_weight", value, TARGET_BREW_WEIGHT_MIN, TARGET_BREW_WEIGHT_MAX)) {
                    return false;
                }

                setTargetBrewWeight(value);
            }

            // Pre-infusion parameters
            if (doc["brew"]["pre_infusion"].containsKey("time")) {
                const double value = doc["brew"]["pre_infusion"]["time"].as<double>();

                if (!validateParameterRange("brew.pre_infusion.time", value, PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX)) {
                    return false;
                }

                setPreInfusionTime(value);
            }

            if (doc["brew"]["pre_infusion"].containsKey("pause")) {
                const double value = doc["brew"]["pre_infusion"]["pause"].as<double>();

                if (!validateParameterRange("brew.pre_infusion.pause", value, PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX)) {
                    return false;
                }

                setPreInfusionPause(value);
            }

            // Steam parameters
            if (doc["steam"].containsKey("setpoint")) {
                const double value = doc["steam"]["setpoint"].as<double>();

                if (!validateParameterRange("steam.setpoint", value, STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX)) {
                    return false;
                }

                setSteamSetpoint(value);
            }

            // Backflush parameters
            if (doc["backflush"].containsKey("cycles")) {
                const int value = doc["backflush"]["cycles"].as<int>();

                if (!validateParameterRange("backflush.cycles", value, BACKFLUSH_CYCLES_MIN, BACKFLUSH_CYCLES_MAX)) {
                    return false;
                }

                setBackflushCycles(value);
            }

            if (doc["backflush"].containsKey("fill_time")) {
                const double value = doc["backflush"]["fill_time"].as<double>();

                if (!validateParameterRange("backflush.fill_time", value, BACKFLUSH_FILL_TIME_MIN, BACKFLUSH_FILL_TIME_MAX)) {
                    return false;
                }

                setBackflushFillTime(value);
            }

            if (doc["backflush"].containsKey("flush_time")) {
                const double value = doc["backflush"]["flush_time"].as<double>();

                if (!validateParameterRange("backflush.flush_time", value, BACKFLUSH_FLUSH_TIME_MIN, BACKFLUSH_FLUSH_TIME_MAX)) {
                    return false;
                }

                setBackflushFlushTime(value);
            }

            // Standby parameters
            if (doc["standby"].containsKey("enabled")) {
                if (!doc["standby"]["enabled"].is<bool>()) {
                    return false;
                }

                setStandbyModeOn(doc["standby"]["enabled"].as<bool>());
            }

            if (doc["standby"].containsKey("time")) {
                const double value = doc["standby"]["time"].as<double>();

                if (!validateParameterRange("standby.time", value, STANDBY_MODE_TIME_MIN, STANDBY_MODE_TIME_MAX)) {
                    return false;
                }

                setStandbyModeTime(value);
            }

            // Feature parameters
            if (doc["features"].containsKey("brew_control")) {
                if (!doc["features"]["brew_control"].is<bool>()) {
                    return false;
                }

                setFeatureBrewControl(doc["features"]["brew_control"].as<bool>());
            }

            if (doc["display"].containsKey("fullscreen_brew_timer")) {
                if (!doc["display"]["fullscreen_brew_timer"].is<bool>()) {
                    return false;
                }

                setFeatureFullscreenBrewTimer(doc["features"]["fullscreen_brew_timer"].as<bool>());
            }

            if (doc["display"].containsKey("fullscreen_manual_flush_timer")) {
                if (!doc["display"]["fullscreen_manual_flush_timer"].is<bool>()) {
                    return false;
                }

                setFeatureFullscreenManualFlushTimer(doc["display"]["fullscreen_manual_flush_timer"].as<bool>());
            }

            if (doc["display"].containsKey("post_brew_timer_duration")) {
                const double value = doc["display"]["post_brew_timer_duration"].as<double>();

                if (!validateParameterRange("display.post_brew_timer_duration", value, POST_BREW_TIMER_DURATION_MIN, POST_BREW_TIMER_DURATION_MAX)) {
                    return false;
                }

                setPostBrewTimerDuration(value);
            }

            if (doc["display"].containsKey("heating_logo")) {
                if (!doc["display"]["heating_logo"].is<bool>()) {
                    return false;
                }

                setFeatureHeatingLogo(doc["display"]["heating_logo"].as<bool>());
            }

            if (doc["display"].containsKey("pid_off_logo")) {
                if (!doc["display"]["pid_off_logo"].is<bool>()) {
                    return false;
                }

                setFeaturePidOffLogo(doc["display"]["pid_off_logo"].as<bool>());
            }

            // MQTT parameters
            if (doc["mqtt"].containsKey("enabled")) {
                if (!doc["mqtt"]["enabled"].is<bool>()) {
                    return false;
                }

                setMqttEnabled(doc["mqtt"]["enabled"].as<bool>());
            }

            if (doc["mqtt"].containsKey("broker")) {
                const String value = constrainStringParameter(doc["mqtt"]["broker"].as<String>(), MQTT_BROKER_MAX_LENGTH, "mqtt.broker");
                setMqttBroker(value);
            }

            if (doc["mqtt"].containsKey("port")) {
                const int value = doc["mqtt"]["port"].as<int>();

                if (value < 1 || value > 65535) {
                    return false;
                }

                setMqttPort(value);
            }

            if (doc["mqtt"].containsKey("username")) {
                const String value = constrainStringParameter(doc["mqtt"]["username"].as<String>(), MQTT_USERNAME_MAX_LENGTH, "mqtt.username");
                setMqttUsername(value);
            }

            if (doc["mqtt"].containsKey("password")) {
                const String value = constrainStringParameter(doc["mqtt"]["password"].as<String>(), MQTT_PASSWORD_MAX_LENGTH, "mqtt.password");
                setMqttPassword(value);
            }

            if (doc["mqtt"].containsKey("topic")) {
                const String value = constrainStringParameter(doc["mqtt"]["topic"].as<String>(), MQTT_TOPIC_MAX_LENGTH, "mqtt.topic");
                setMqttTopic(value);
            }

            if (doc["mqtt"]["hassio"].containsKey("enabled")) {
                if (!doc["mqtt"]["hassio"]["enabled"].is<bool>()) {
                    return false;
                }

                setMqttHassioEnabled(doc["mqtt"]["hassio"]["enabled"].as<bool>());
            }

            if (doc["mqtt"]["hassio"].containsKey("prefix")) {
                const String value = constrainStringParameter(doc["mqtt"]["hassio"]["prefix"].as<String>(), MQTT_HASSIO_PREFIX_MAX_LENGTH, "mqtt.hassio.prefix");
                setMqttHassioPrefix(value);
            }

            // System parameters
            if (doc["system"].containsKey("hostname")) {
                const String value = constrainStringParameter(doc["system"]["hostname"].as<String>(), HOSTNAME_MAX_LENGTH, "system.hostname");
                setHostname(value);
            }

            if (doc["system"].containsKey("ota_password")) {
                const String value = constrainStringParameter(doc["system"]["ota_password"].as<String>(), OTAPASS_MAX_LENGTH, "system.ota_password");
                setOtaPass(value);
            }

            if (doc["system"].containsKey("log_level")) {
                const int value = doc["system"]["log_level"].as<int>();

                if (value < 0 || value > 6) {
                    return false;
                }

                setLogLevel(value);
            }

            if (doc["display"].containsKey("template")) {
                const int value = doc["display"]["template"].as<int>();

                if (value < 0 || value > 4) {
                    return false;
                }

                setDisplayTemplate(value);
            }

            // Hardware - Relays
            if (doc["hardware"]["relays"]["heater"].containsKey("trigger_type")) {
                int value = doc["hardware"]["relays"]["heater"]["trigger_type"].as<int>();

                if (!validateParameterRange("hardware.relays.heater.trigger_type", value, 0, 1)) {
                    return false;
                }

                setHeaterRelayTriggerType(value);
            }

            if (doc["hardware"]["relays"]["pump_valve"].containsKey("trigger_type")) {
                int value = doc["hardware"]["relays"]["pump_valve"]["trigger_type"].as<int>();

                if (!validateParameterRange("hardware.relays.pump_valve.trigger_type", value, 0, 1)) {
                    return false;
                }

                setPumpValveRelayTriggerType(value);
            }

            // Hardware - Switches
            if (doc["hardware"]["switches"]["brew"].containsKey("enabled")) {
                if (!doc["hardware"]["switches"]["brew"]["enabled"].is<bool>()) {
                    return false;
                }

                setBrewSwitchEnabled(doc["hardware"]["switches"]["brew"]["enabled"].as<bool>());
            }

            if (doc["hardware"]["switches"]["brew"].containsKey("type")) {
                int value = doc["hardware"]["switches"]["brew"]["type"].as<int>();

                if (!validateParameterRange("hardware.switches.brew.type", value, 0, 1)) {
                    return false;
                }

                setBrewSwitchType(value);
            }

            if (doc["hardware"]["switches"]["brew"].containsKey("mode")) {
                int value = doc["hardware"]["switches"]["brew"]["mode"].as<int>();

                if (!validateParameterRange("hardware.switches.brew.mode", value, 0, 1)) {
                    return false;
                }

                setBrewSwitchMode(value);
            }

            if (doc["hardware"]["switches"]["steam"].containsKey("enabled")) {
                if (!doc["hardware"]["switches"]["steam"]["enabled"].is<bool>()) {
                    return false;
                }

                setSteamSwitchEnabled(doc["hardware"]["switches"]["steam"]["enabled"].as<bool>());
            }

            if (doc["hardware"]["switches"]["steam"].containsKey("type")) {
                int value = doc["hardware"]["switches"]["steam"]["type"].as<int>();

                if (!validateParameterRange("hardware.switches.steam.type", value, 0, 1)) {
                    return false;
                }

                setSteamSwitchType(value);
            }

            if (doc["hardware"]["switches"]["steam"].containsKey("mode")) {
                int value = doc["hardware"]["switches"]["steam"]["mode"].as<int>();

                if (!validateParameterRange("hardware.switches.steam.mode", value, 0, 1)) {
                    return false;
                }

                setSteamSwitchMode(value);
            }

            if (doc["hardware"]["switches"]["power"].containsKey("enabled")) {
                if (!doc["hardware"]["switches"]["power"]["enabled"].is<bool>()) {
                    return false;
                }

                setPowerSwitchEnabled(doc["hardware"]["switches"]["power"]["enabled"].as<bool>());
            }

            if (doc["hardware"]["switches"]["power"].containsKey("type")) {
                int value = doc["hardware"]["switches"]["power"]["type"].as<int>();

                if (!validateParameterRange("hardware.switches.power.type", value, 0, 1)) {
                    return false;
                }

                setPowerSwitchType(value);
            }

            if (doc["hardware"]["switches"]["power"].containsKey("mode")) {
                int value = doc["hardware"]["switches"]["power"]["mode"].as<int>();

                if (!validateParameterRange("hardware.switches.power.mode", value, 0, 1)) {
                    return false;
                }

                setPowerSwitchMode(value);
            }

            // Hardware - LEDs
            if (doc["hardware"]["leds"]["status"].containsKey("enabled")) {
                if (!doc["hardware"]["leds"]["status"]["enabled"].is<bool>()) {
                    return false;
                }

                setStatusLedEnabled(doc["hardware"]["leds"]["status"]["enabled"].as<bool>());
            }

            if (doc["hardware"]["leds"]["status"].containsKey("inverted")) {
                if (!doc["hardware"]["leds"]["status"]["inverted"].is<bool>()) {
                    return false;
                }

                setStatusLedInverted(doc["hardware"]["leds"]["status"]["inverted"].as<bool>());
            }

            if (doc["hardware"]["leds"]["brew"].containsKey("enabled")) {
                if (!doc["hardware"]["leds"]["brew"]["enabled"].is<bool>()) {
                    return false;
                }

                setBrewLedEnabled(doc["hardware"]["leds"]["brew"]["enabled"].as<bool>());
            }

            if (doc["hardware"]["leds"]["brew"].containsKey("inverted")) {
                if (!doc["hardware"]["leds"]["brew"]["inverted"].is<bool>()) {
                    return false;
                }
                setBrewLedInverted(doc["hardware"]["leds"]["brew"]["inverted"].as<bool>());
            }

            if (doc["hardware"]["leds"]["steam"].containsKey("enabled")) {
                if (!doc["hardware"]["leds"]["steam"]["enabled"].is<bool>()) {
                    return false;
                }

                setSteamLedEnabled(doc["hardware"]["leds"]["steam"]["enabled"].as<bool>());
            }

            if (doc["hardware"]["leds"]["steam"].containsKey("inverted")) {
                if (!doc["hardware"]["leds"]["steam"]["inverted"].is<bool>()) {
                    return false;
                }

                setSteamLedInverted(doc["hardware"]["leds"]["steam"]["inverted"].as<bool>());
            }

            // Hardware - Sensors
            if (doc["hardware"]["sensors"]["pressure"].containsKey("enabled")) {
                if (!doc["hardware"]["sensors"]["pressure"]["enabled"].is<bool>()) {
                    return false;
                }

                setPressureSensorEnabled(doc["hardware"]["sensors"]["pressure"]["enabled"].as<bool>());
            }

            if (doc["hardware"]["sensors"]["watertank"].containsKey("enabled")) {
                if (!doc["hardware"]["sensors"]["watertank"]["enabled"].is<bool>()) {
                    return false;
                }

                setWaterTankSensorEnabled(doc["hardware"]["sensors"]["watertank"]["enabled"].as<bool>());
            }

            if (doc["hardware"]["sensors"]["watertank"].containsKey("mode")) {
                int value = doc["hardware"]["sensors"]["watertank"]["mode"].as<int>();

                if (!validateParameterRange("hardware.sensors.watertank.mode", value, 0, 1)) {
                    return false;
                }

                setWaterTankSensorMode(value);
            }

            // Scale
            if (doc["hardware"]["sensors"]["scale"].containsKey("calibration")) {
                const float value = doc["scale"]["calibration"].as<float>();

                if (!validateParameterRange("scale.calibration", static_cast<double>(value), SCALE_CALIBRATION_MIN, SCALE_CALIBRATION_MAX)) {
                    return false;
                }

                setScaleCalibration(value);
            }

            if (doc["hardware"]["sensors"]["scale"].containsKey("calibration2")) {
                const float value = doc["scale"]["calibration2"].as<float>();

                if (!validateParameterRange("scale.calibration2", static_cast<double>(value), SCALE2_CALIBRATION_MIN, SCALE2_CALIBRATION_MAX)) {
                    return false;
                }

                setScale2Calibration(value);
            }

            if (doc["hardware"]["sensors"]["scale"].containsKey("known_weight")) {
                const float value = doc["scale"]["known_weight"].as<float>();

                if (!validateParameterRange("scale.known_weight", static_cast<double>(value), SCALE_KNOWN_WEIGHT_MIN, SCALE_KNOWN_WEIGHT_MAX)) {
                    return false;
                }

                setScaleKnownWeight(value);
            }

            if (doc["hardware"]["sensors"]["hardware"]["sensors"]["scale"].containsKey("enabled")) {
                if (!doc["hardware"]["sensors"]["scale"]["enabled"].is<bool>()) {
                    return false;
                }

                setScaleEnabled(doc["hardware"]["sensors"]["scale"]["enabled"].as<bool>());
            }

            if (doc["hardware"]["sensors"]["hardware"]["sensors"]["scale"].containsKey("samples")) {
                int value = doc["hardware"]["sensors"]["scale"]["samples"].as<int>();

                if (!validateParameterRange("hardware.sensors.scale.samples", value, 1, 10)) {
                    return false;
                }

                setScaleSamples(value);
            }

            if (doc["hardware"]["sensors"]["hardware"]["sensors"]["scale"].containsKey("type")) {
                int value = doc["hardware"]["sensors"]["scale"]["type"].as<int>();

                if (!validateParameterRange("hardware.sensors.scale.type", value, 0, 1)) {
                    return false;
                }

                setScaleType(value);
            }

            // WiFi parameters
            if (doc["wifi"].containsKey("credentials_saved")) {
                if (!doc["wifi"]["credentials_saved"].is<bool>()) {
                    return false;
                }

                setWifiCredentialsSaved(doc["wifi"]["credentials_saved"].as<bool>());
            }

            return save();
        }

        template <typename T>
        static bool validateParameterRange(const char* paramName, T value, T min, T max) {
            if (value < min || value > max) {
                LOGF(ERROR, "Parameter %s value %.2f out of range [%.2f, %.2f]", paramName, static_cast<double>(value), static_cast<double>(min), static_cast<double>(max));
                return false;
            }
            return true;
        }

        static String constrainStringParameter(const String& value, const size_t maxLength, const char* paramName = nullptr) {
            if (value.length() <= maxLength) {
                return value;
            }

            LOGF(WARNING, "Parameter '%s' truncated from %d to %d characters", paramName, value.length(), maxLength);

            return value.substring(0, maxLength);
        }
};
