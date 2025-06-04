
/**
 * @file Config.h
 *
 * @brief Centralized configuration management with JSON storage
 */

#pragma once

#include "Logger.h"
#include "defaults.h"
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
            _doc["pid"]["enabled"] = 0;
            _doc["pid"]["use_ponm"] = 0;

            // PID regular
            _doc["pid"]["regular"]["kp"] = AGGKP;
            _doc["pid"]["regular"]["tn"] = AGGTN;
            _doc["pid"]["regular"]["tv"] = AGGTV;
            _doc["pid"]["regular"]["i_max"] = AGGIMAX;

            // PID brew detection
            _doc["pid"]["bd"]["enabled"] = 0;
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

            // Scale
            _doc["scale"]["calibration"] = SCALE_CALIBRATION_FACTOR;
            _doc["scale"]["calibration2"] = SCALE_CALIBRATION_FACTOR;
            _doc["scale"]["known_weight"] = SCALE_KNOWN_WEIGHT;

            // Backflushing
            _doc["backflush"]["cycles"] = BACKFLUSH_CYCLES;
            _doc["backflush"]["fill_time"] = BACKFLUSH_FILL_TIME;
            _doc["backflush"]["flush_time"] = BACKFLUSH_FLUSH_TIME;

            // Standby
            _doc["standby"]["enabled"] = STANDBY_MODE_ON;
            _doc["standby"]["time"] = STANDBY_MODE_TIME;

            // Features
            _doc["features"]["brew_control"] = FEATURE_BREW_CONTROL;
            _doc["features"]["fullscreen_brew_timer"] = FEATURE_FULLSCREEN_BREW_TIMER;
            _doc["features"]["fullscreen_manual_flush_timer"] = FEATURE_FULLSCREEN_MANUAL_FLUSH_TIMER;
            _doc["features"]["post_brew_timer_duration"] = POST_BREW_TIMER_DURATION;
            _doc["features"]["heating_logo"] = FEATURE_HEATING_LOGO;
            _doc["features"]["pid_off_logo"] = FEATURE_PID_OFF_LOGO;

            // MQTT
            _doc["mqtt"]["enabled"] = 0;
            _doc["mqtt"]["broker"] = "";
            _doc["mqtt"]["port"] = 1883;
            _doc["mqtt"]["username"] = "rancilio";
            _doc["mqtt"]["password"] = "silvia";
            _doc["mqtt"]["topic"] = "custom/kitchen.";
            _doc["mqtt"]["hassio"]["enabled"] = 0;
            _doc["mqtt"]["hassio"]["prefix"] = "homeassistant";

            // WiFi credentials flag
            _doc["wifi"]["credentials_saved"] = 0;
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

        // WiFi credentials flag
        bool getWifiCredentialsSaved() {
            return _doc["wifi"]["credentials_saved"] | 0;
        }

        void setWifiCredentialsSaved(const bool value) {
            _doc["wifi"]["credentials_saved"] = value ? 1 : 0;
        }

        // PID general
        bool getPidEnabled() {
            return _doc["pid"]["enabled"] | 0;
        }

        void setPidEnabled(const bool value) {
            _doc["pid"]["enabled"] = value ? 1 : 0;
        }

        bool getUsePonM() {
            return _doc["pid"]["use_ponm"] | 0;
        }

        void setUsePonM(const bool value) {
            _doc["pid"]["use_ponm"] = value ? 1 : 0;
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
            return _doc["pid"]["bd"]["enabled"] | 0;
        }

        void setUseBDPID(const bool value) {
            _doc["pid"]["bd"]["enabled"] = value ? 1 : 0;
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

        // Scale
        float getScaleCalibration() {
            return _doc["scale"]["calibration"] | static_cast<float>(SCALE_CALIBRATION_FACTOR);
        }

        void setScaleCalibration(const float value) {
            _doc["scale"]["calibration"] = constrain(value, -100000.0f, 100000.0f);
        }

        void setScaleKnownWeight(const float value) {
            _doc["scale"]["known_weight"] = constrain(value, 0.0f, 2000.0f);
        }

        float getScaleKnownWeight() {
            return _doc["scale"]["known_weight"] | static_cast<float>(SCALE_KNOWN_WEIGHT);
        }

        float getScale2Calibration() {
            return _doc["scale"]["calibration2"] | static_cast<float>(SCALE_CALIBRATION_FACTOR);
        }

        void setScale2Calibration(const float value) {
            _doc["scale"]["calibration2"] = constrain(value, -100000.0f, 100000.0f);
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
            return _doc["standby"]["enabled"] | STANDBY_MODE_ON;
        }

        void setStandbyModeOn(const bool value) {
            _doc["standby"]["enabled"] = value ? 1 : 0;
        }

        double getStandbyModeTime() {
            return _doc["standby"]["time"] | STANDBY_MODE_TIME;
        }

        void setStandbyModeTime(const double value) {
            _doc["standby"]["time"] = constrain(value, STANDBY_MODE_TIME_MIN, STANDBY_MODE_TIME_MAX);
        }

        // Features
        bool getFeatureBrewControl() {
            return _doc["features"]["brew_control"] | FEATURE_BREW_CONTROL;
        }

        void setFeatureBrewControl(const bool value) {
            _doc["features"]["brew_control"] = value ? 1 : 0;
        }

        bool getFeatureFullscreenBrewTimer() {
            return _doc["features"]["fullscreen_brew_timer"] | FEATURE_FULLSCREEN_BREW_TIMER;
        }

        void setFeatureFullscreenBrewTimer(const bool value) {
            _doc["features"]["fullscreen_brew_timer"] = value ? 1 : 0;
        }

        bool getFeatureFullscreenManualFlushTimer() {
            return _doc["features"]["fullscreen_manual_flush_timer"] | FEATURE_FULLSCREEN_MANUAL_FLUSH_TIMER;
        }

        void setFeatureFullscreenManualFlushTimer(const bool value) {
            _doc["features"]["fullscreen_manual_flush_timer"] = value ? 1 : 0;
        }

        double getPostBrewTimerDuration() {
            return _doc["features"]["post_brew_timer_duration"] | POST_BREW_TIMER_DURATION;
        }

        void setPostBrewTimerDuration(const double value) {
            _doc["features"]["post_brew_timer_duration"] = constrain(value, POST_BREW_TIMER_DURATION_MIN, POST_BREW_TIMER_DURATION_MAX);
        }

        bool getFeatureHeatingLogo() {
            return _doc["features"]["heating_logo"] | FEATURE_HEATING_LOGO;
        }

        void setFeatureHeatingLogo(const bool value) {
            _doc["features"]["heating_logo"] = value ? 1 : 0;
        }

        bool getFeaturePidOffLogo() {
            return _doc["features"]["pid_off_logo"] | FEATURE_PID_OFF_LOGO;
        }

        void setFeaturePidOffLogo(const bool value) {
            _doc["features"]["pid_off_logo"] = value ? 1 : 0;
        }

        // MQTT
        bool getMqttEnabled() {
            return _doc["mqtt"]["enabled"] | 0;
        }

        void setMqttEnabled(const bool value) {
            _doc["mqtt"]["enabled"] = value ? 1 : 0;
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
            return _doc["mqtt"]["hassio"]["enabled"] | 0;
        }

        void setMqttHassioEnabled(const bool value) {
            _doc["mqtt"]["hassio"]["enabled"] = value ? 1 : 0;
        }

        String getMqttHassioPrefix() {
            return _doc["mqtt"]["hassio"]["prefix"];
        }

        void setMqttHassioPrefix(const String& value) {
            _doc["mqtt"]["hassio"]["prefix"] = constrainStringParameter(value, MQTT_HASSIO_PREFIX_MAX_LENGTH, "mqtt.hassio.prefix");
        }

    private:
        inline static auto CONFIG_FILE = "/config.json";

        StaticJsonDocument<4096> _doc;

        static String constrainStringParameter(const String& value, const size_t maxLength, const char* paramName = nullptr) {
            if (value.length() <= maxLength) {
                return value;
            }

            LOGF(WARNING, "Parameter '%s' truncated from %d to %d characters", paramName, value.length(), maxLength);

            return value.substring(0, maxLength);
        }
};
