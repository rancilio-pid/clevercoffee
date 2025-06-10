/**
 * @file MQTT.h
 *
 * @brief MQTT message handling
 *
 */

#pragma once

#include "Parameter.h"
#include "userConfig.h"
#include <Arduino.h>
#include <PubSubClient.h>
#include <os.h>

inline unsigned long previousMillisMQTT;
constexpr unsigned long intervalMQTT = 5000;

inline WiFiClient net;
inline PubSubClient mqtt(net);

inline bool mqtt_enabled = false;
inline String mqtt_server_ip = "";
inline int mqtt_server_port = 1883;
inline String mqtt_username = "";
inline String mqtt_password = "";
inline String mqtt_topic_prefix = "";
inline bool mqtt_hassio_enabled = false;
inline String mqtt_hassio_discovery_prefix = "";

inline char topic_will[256];
inline char topic_set[256];

inline unsigned long lastMQTTConnectionAttempt = millis();
inline unsigned int MQTTReCnctCount = 0;

extern std::map<const char*, const char*, cmp_str> mqttVars;
extern std::map<const char*, std::function<double()>, cmp_str> mqttSensors;

struct DiscoveryObject {
        String discovery_topic;
        String payload_json;
};

inline void setupMqtt() {
    ParameterRegistry& registry = ParameterRegistry::getInstance();

    if (!registry.isReady()) {
        LOGF(ERROR, "ParameterRegistry not ready, cannot initialize MQTT");
        return;
    }

    mqtt_enabled = registry.getParameterById("MQTT_ENABLED")->getBoolValue();
    mqtt_server_ip = registry.getParameterById("MQTT_BROKER")->getStringValue();
    mqtt_server_port = registry.getParameterById("MQTT_PORT")->getIntValue();
    mqtt_username = registry.getParameterById("MQTT_USERNAME")->getStringValue();
    mqtt_password = registry.getParameterById("MQTT_PASSWORD")->getStringValue();
    mqtt_topic_prefix = registry.getParameterById("MQTT_TOPIC")->getStringValue();
    mqtt_hassio_enabled = registry.getParameterById("MQTT_HASSIO_ENABLED")->getBoolValue();
    mqtt_hassio_discovery_prefix = registry.getParameterById("MQTT_HASSIO_PREFIX")->getStringValue();
}

/**
 * @brief Check if MQTT is connected, if not reconnect. Abort function if offline or brew is running
 *      MQTT is also using maxWifiReconnects!
 */
inline void checkMQTT() {
    if (offlineMode == 1 || currBrewState > kBrewIdle) {
        return;
    }

    if (millis() - lastMQTTConnectionAttempt >= wifiConnectionDelay && MQTTReCnctCount <= maxWifiReconnects) {
        if (!mqtt.connected()) {
            lastMQTTConnectionAttempt = millis(); // Reconnection Timer Function
            MQTTReCnctCount++;                    // Increment reconnection Counter
            LOGF(DEBUG, "Attempting MQTT reconnection: %i", MQTTReCnctCount);

            if (mqtt.connect(hostname.c_str(), mqtt_username.c_str(), mqtt_password.c_str(), topic_will, 0, true, "offline")) {
                mqtt.subscribe(topic_set);
                LOGF(DEBUG, "Subscribed to MQTT Topic: %s", topic_set);
            } // Try to reconnect to the server; connect() is a blocking
              // function, watch the timeout!
            else {
                LOGF(DEBUG, "Failed to connect to MQTT due to reason: %i", mqtt.state());
            }
        }
    }
}

/**
 * @brief Publish Data to MQTT
 */
inline bool mqtt_publish(const char* reading, const char* payload, const boolean retain = false) {
    char topic[120];
    snprintf(topic, 120, "%s%s/%s", mqtt_topic_prefix.c_str(), hostname.c_str(), reading);
    return mqtt.publish(topic, payload, retain);
}

/**
 * @brief Publishes a large message to an MQTT topic, splitting it into smaller chunks if necessary.
 *
 * @param topic The MQTT topic to publish the message to.
 * @param largeMessage The large message to be published.
 * @return 0 if the message was successfully published, otherwise an MQTT error code.
 */
inline int PublishLargeMessage(const String& topic, const String& largeMessage) {
    constexpr size_t splitSize = 128; // Maximum Message Size

    if (const size_t messageLength = largeMessage.length(); messageLength > splitSize) {
        const size_t count = messageLength / splitSize;
        mqtt.beginPublish(topic.c_str(), messageLength, true);

        for (size_t i = 0; i < count; i++) {
            const size_t startIndex = i * splitSize;
            const size_t endIndex = startIndex + splitSize;
            mqtt.print(largeMessage.substring(startIndex, endIndex));
        }

        mqtt.print(largeMessage.substring(count * splitSize));

        if (const int publishResult = mqtt.endPublish(); publishResult == 0) {
            LOG(WARNING, "[MQTT] PublishLargeMessage sent failed");
            return 1;
        }
        else {
            return 0;
        }
    }
    else {
        boolean publishResult = mqtt.publish(topic.c_str(), largeMessage.c_str());

        return publishResult ? 0 : -1; // Return 0 for success, -1 for failure
    }
}

/**
 * @brief Assign the value of the mqtt parameter to the associated variable
 *
 * @param param MQTT parameter name
 * @param value MQTT value
 */
inline void assignMQTTParam(char* param, double value) {
    try {
        const auto it = mqttVars.find(param);

        if (it == mqttVars.end()) {
            LOGF(WARNING, "MQTT topic %s not found in mapping", param);
            return;
        }

        const char* parameterId = it->second;

        auto& registry = ParameterRegistry::getInstance();
        const std::shared_ptr<Parameter> var = registry.getParameterById(parameterId);

        if (!var) {
            LOGF(WARNING, "Parameter %s not found in ParameterRegistry", parameterId);
            return;
        }

        if (value >= var->getMinValue() && value <= var->getMaxValue()) {
            bool success = false;

            switch (var->getType()) {
                case kDouble:
                    success = registry.setParameterValue(parameterId, value);
                    break;
                case kFloat:
                    success = registry.setParameterValue(parameterId, static_cast<float>(value));
                    break;
                case kUInt8:
                    success = registry.setParameterValue(parameterId, static_cast<uint8_t>(value));
                    if (success && strcasecmp(param, "steamON") == 0) {
                        steamFirstON = value;
                    }
                    break;
                case kInteger:
                    success = registry.setParameterValue(parameterId, static_cast<int>(value));
                    break;
                default:
                    LOGF(WARNING, "%s is not a recognized type for this MQTT parameter.", var->getType());
                    return;
            }

            if (success) {
                mqtt_publish(param, number2string(value), true); // Publish back with MQTT topic name
                LOGF(DEBUG, "MQTT parameter %s (ID: %s) updated to %f", param, parameterId, value);
            }
            else {
                LOGF(WARNING, "Failed to update MQTT parameter %s", param);
            }
        }
        else {
            LOGF(WARNING, "Value %f is out of range for MQTT parameter %s (min: %f, max: %f)", value, param, var->getMinValue(), var->getMaxValue());
        }
    } catch (const std::exception& e) {
        LOGF(WARNING, "Error processing MQTT parameter %s: %s", param, e.what());
    }
}

/**
 * @brief MQTT Callback Function: set Parameters through MQTT
 */
inline void mqtt_callback(const char* topic, const byte* data, const unsigned int length) {
    char topic_str[256];
    os_memcpy(topic_str, topic, sizeof(topic_str));
    topic_str[255] = '\0';
    char data_str[length + 1];
    os_memcpy(data_str, data, length);
    data_str[length] = '\0';
    char topic_pattern[255];
    char configVar[120];
    char cmd[64];
    double data_double;

    snprintf(topic_pattern, sizeof(topic_pattern), "%s%s/%%[^\\/]/%%[^\\/]", mqtt_topic_prefix.c_str(), hostname.c_str());

    if (sscanf(topic_str, topic_pattern, &configVar, &cmd) != 2 || strcmp(cmd, "set") != 0) {
        LOGF(WARNING, "Invalid MQTT topic/command: %s", topic_str);
        return;
    }

    LOGF(DEBUG, "Received MQTT command %s %s\n", topic_str, data_str);

    // convert received string value to double assuming it's a number
    sscanf(data_str, "%lf", &data_double);
    assignMQTTParam(configVar, data_double);
}

/**
 * @brief Send all current system parameter values to MQTT
 *
 * @param continueOnError Flag to specify whether to continue publishing messages in case of an error (default: true)
 * @return 0 = success, MQTT error code = failure
 */

inline int writeSysParamsToMQTT(const bool continueOnError = true) {
    unsigned long currentMillisMQTT = millis();

    if (currentMillisMQTT - previousMillisMQTT >= intervalMQTT && mqtt_enabled) {
        previousMillisMQTT = currentMillisMQTT;

        if (mqtt.connected()) {
            mqtt_publish("status", (char*)"online");

            char data[256];
            int errorState = 0;
            auto& registry = ParameterRegistry::getInstance();

            // Iterate through the mqttVars mapping to publish parameters
            for (const auto& [fst, snd] : mqttVars) {
                const char* mqttTopic = fst;   // MQTT topic name
                const char* parameterId = snd; // Parameter ID

                std::shared_ptr<Parameter> param = registry.getParameterById(parameterId);

                if (param == nullptr) {
                    if (!continueOnError) {
                        LOGF(ERROR, "Parameter %s not found for MQTT topic %s", parameterId, mqttTopic);
                        return 1;
                    }

                    LOGF(WARNING, "Parameter %s not found for MQTT topic %s, skipping", parameterId, mqttTopic);
                    continue;
                }

                // Get value based on parameter type and format as string
                switch (param->getType()) {
                    case kInteger:
                        snprintf(data, sizeof(data), "%d", param->getIntValue());
                        break;
                    case kUInt8:
                        snprintf(data, sizeof(data), "%u", param->getUInt8Value());
                        break;
                    case kDouble:
                    case kDoubletime:
                        snprintf(data, sizeof(data), "%.2f", param->getValue());
                        break;
                    case kFloat:
                        snprintf(data, sizeof(data), "%.2f", param->getFloatValue());
                        break;
                    case kCString:
                        snprintf(data, sizeof(data), "%s", param->getStringValue().c_str());
                        break;
                    default:
                        if (!continueOnError) {
                            LOGF(ERROR, "Unknown parameter type for topic %s", mqttTopic);
                            return 1;
                        }

                        LOGF(WARNING, "Skipping unknown parameter type for topic %s", mqttTopic);
                        continue;
                }

                if (!mqtt_publish(mqttTopic, data, true)) {
                    errorState = mqtt.state();

                    if (!continueOnError) {
                        LOGF(ERROR, "Failed to publish parameter %s to MQTT, error: %d", mqttTopic, errorState);
                        return errorState;
                    }

                    LOGF(WARNING, "Failed to publish parameter %s to MQTT, error: %d", mqttTopic, errorState);
                }
                else {
                    LOGF(DEBUG, "Published %s = %s to MQTT", mqttTopic, data);
                }
            }

            for (const auto& [fst, snd] : mqttSensors) {
                if (!mqtt_publish(fst, number2string(snd()))) {
                    errorState = mqtt.state();

                    if (!continueOnError) {
                        return errorState;
                    }
                }
            }
        }
    }

    return 0;
}

/**
 * @brief Generate a switch device for Home Assistant MQTT discovery
 *
 * This function generates a switch device configuration for Home Assistant MQTT discovery. It creates a `DiscoveryObject` containing the necessary information for Home Assistant to discover and control the switch device.
 *
 * @param name The name of the switch (used in MQTT topics)
 * @param displayName The display name of the switch (shown in Home Assistant)
 * @param payload_on The payload value to turn the switch on (default: "1")
 * @param payload_off The payload value to turn the switch off (default: "0")
 * @return A `DiscoveryObject` containing the switch device configuration
 */
inline DiscoveryObject GenerateSwitchDevice(const String& name, const String& displayName, const String& payload_on = "1", const String& payload_off = "0") {
    String mqtt_topic = String(mqtt_topic_prefix) + hostname;
    DiscoveryObject switch_device;
    String unique_id = "clevercoffee-" + hostname;
    String SwitchDiscoveryTopic = mqtt_hassio_discovery_prefix + "/switch/";

    String switch_command_topic = mqtt_topic + "/" + name + "/set";
    String switch_state_topic = mqtt_topic + "/" + name;

    switch_device.discovery_topic = SwitchDiscoveryTopic + unique_id + "-" + name + "" + "/config";

    DynamicJsonDocument DeviceMapDoc(1024);
    DeviceMapDoc["identifiers"] = hostname;
    DeviceMapDoc["manufacturer"] = "CleverCoffee";
    DeviceMapDoc["name"] = hostname;

    DynamicJsonDocument switchConfigDoc(512);
    switchConfigDoc["name"] = displayName;
    switchConfigDoc["command_topic"] = switch_command_topic;
    switchConfigDoc["state_topic"] = switch_state_topic;
    switchConfigDoc["unique_id"] = unique_id + "-" + name;
    switchConfigDoc["payload_on"] = payload_on;
    switchConfigDoc["payload_off"] = payload_off;
    switchConfigDoc["payload_available"] = "online";
    switchConfigDoc["payload_not_available"] = "offline";
    switchConfigDoc["availability_topic"] = mqtt_topic + "/status";

    JsonObject switchDeviceField = switchConfigDoc.createNestedObject("device");

    for (JsonPair keyValue : DeviceMapDoc.as<JsonObject>()) {
        switchDeviceField[keyValue.key()] = keyValue.value();
    }

    String switchConfigDocBuffer;
    serializeJson(switchConfigDoc, switchConfigDocBuffer);

    switch_device.payload_json = switchConfigDocBuffer;

    return switch_device;
}

/**
 * @brief Generate a button device for Home Assistant MQTT discovery
 *
 * This function generates a button device configuration for Home Assistant MQTT discovery. It creates a `DiscoveryObject` containing the necessary information for Home Assistant to discover and control the button device.
 *
 * @param name The name of the button (used in MQTT topics)
 * @param displayName The display name of the button (shown in Home Assistant)
 * @param payload_press The payload value to turn the button is pressed (default: "1")
 * @return A `DiscoveryObject` containing the switch device configuration
 */
inline DiscoveryObject GenerateButtonDevice(const String& name, const String& displayName, const String& payload_press = "1") {
    String mqtt_topic = String(mqtt_topic_prefix) + hostname;
    DiscoveryObject button_device;
    String unique_id = "clevercoffee-" + hostname;
    String buttonDiscoveryTopic = mqtt_hassio_discovery_prefix + "/button/";

    String button_command_topic = mqtt_topic + "/" + name + "/set";
    String button_state_topic = mqtt_topic + "/" + name;

    button_device.discovery_topic = buttonDiscoveryTopic + unique_id + "-" + name + "" + "/config";

    DynamicJsonDocument DeviceMapDoc(1024);
    DeviceMapDoc["identifiers"] = hostname;
    DeviceMapDoc["manufacturer"] = "CleverCoffee";
    DeviceMapDoc["name"] = hostname;

    DynamicJsonDocument buttonConfigDoc(512);
    buttonConfigDoc["name"] = displayName;
    buttonConfigDoc["command_topic"] = button_command_topic;
    buttonConfigDoc["state_topic"] = button_state_topic;
    buttonConfigDoc["unique_id"] = unique_id + "-" + name;
    buttonConfigDoc["payload_press"] = payload_press;
    buttonConfigDoc["payload_available"] = "online";
    buttonConfigDoc["payload_not_available"] = "offline";
    buttonConfigDoc["availability_topic"] = mqtt_topic + "/status";

    JsonObject buttonDeviceField = buttonConfigDoc.createNestedObject("device");

    for (JsonPair keyValue : DeviceMapDoc.as<JsonObject>()) {
        buttonDeviceField[keyValue.key()] = keyValue.value();
    }

    String buttonConfigDocBuffer;
    serializeJson(buttonConfigDoc, buttonConfigDocBuffer);
    LOG(DEBUG, "Generated button device");
    button_device.payload_json = buttonConfigDocBuffer;

    return button_device;
}

/**
 * @brief Generate a sensor device for Home Assistant MQTT discovery
 *
 * This function generates a sensor device configuration for Home Assistant MQTT discovery. It creates a `DiscoveryObject` containing the necessary information for Home Assistant to discover and monitor the sensor device.
 *
 * @param name The name of the sensor (used in MQTT topics)
 * @param displayName The display name of the sensor (shown in Home Assistant)
 * @param unit_of_measurement The unit of measurement for the sensor data (default: "°C")
 * @param device_class
 * @return A `DiscoveryObject` containing the sensor device configuration
 */
inline DiscoveryObject GenerateSensorDevice(const String& name, const String& displayName, const String& unit_of_measurement, const String& device_class) {
    String mqtt_topic = String(mqtt_topic_prefix) + hostname;
    DiscoveryObject sensor_device;
    String unique_id = "clevercoffee-" + hostname;
    String SensorDiscoveryTopic = mqtt_hassio_discovery_prefix + "/sensor/";

    String sensor_state_topic = mqtt_topic + "/" + name;
    sensor_device.discovery_topic = SensorDiscoveryTopic + unique_id + "-" + name + "" + "/config";

    DynamicJsonDocument DeviceMapDoc(1024);
    DeviceMapDoc["identifiers"] = hostname;
    DeviceMapDoc["manufacturer"] = "CleverCoffee";
    DeviceMapDoc["name"] = hostname;

    DynamicJsonDocument sensorConfigDoc(512);
    sensorConfigDoc["name"] = displayName;
    sensorConfigDoc["state_topic"] = sensor_state_topic;
    sensorConfigDoc["unique_id"] = unique_id + "-" + name;
    sensorConfigDoc["unit_of_measurement"] = unit_of_measurement;
    sensorConfigDoc["device_class"] = device_class;
    sensorConfigDoc["payload_available"] = "online";
    sensorConfigDoc["payload_not_available"] = "offline";
    sensorConfigDoc["availability_topic"] = mqtt_topic + "/status";

    JsonObject sensorDeviceField = sensorConfigDoc.createNestedObject("device");

    for (JsonPair keyValue : DeviceMapDoc.as<JsonObject>()) {
        sensorDeviceField[keyValue.key()] = keyValue.value();
    }

    String sensorConfigDocBuffer;
    serializeJson(sensorConfigDoc, sensorConfigDocBuffer);

    sensor_device.payload_json = sensorConfigDocBuffer;

    return sensor_device;
}

/**
 * @brief Generate a number device for Home Assistant MQTT discovery
 *
 * This function generates a number device configuration for Home Assistant MQTT discovery. It creates a `DiscoveryObject` containing the necessary information for Home Assistant to discover and control the number device.
 *
 * @param name The name of the number device (used in MQTT topics)
 * @param displayName The display name of the number device (shown in Home Assistant)
 * @param min_value The minimum value allowed for the number device
 * @param max_value The maximum value allowed for the number device
 * @param steps_value The step value for incrementing/decrementing the number device value
 * @param unit_of_measurement The unit of measurement for the number device (default: "°C")
 * @param ui_mode Control how the number should be displayed in the UI
 * @return A `DiscoveryObject` containing the number device configuration
 */
inline DiscoveryObject GenerateNumberDevice(const String& name, const String& displayName, int min_value, int max_value, float steps_value, const String& unit_of_measurement, const String& ui_mode = "box") {
    String mqtt_topic = String(mqtt_topic_prefix) + hostname;
    DiscoveryObject number_device;
    String unique_id = "clevercoffee-" + hostname;

    String NumberDiscoveryTopic = String(mqtt_hassio_discovery_prefix) + "/number/";
    number_device.discovery_topic = NumberDiscoveryTopic + unique_id + "-" + name + "" + "/config";

    DynamicJsonDocument DeviceMapDoc(1024);
    DeviceMapDoc["identifiers"] = hostname;
    DeviceMapDoc["manufacturer"] = "CleverCoffee";
    DeviceMapDoc["name"] = hostname;

    DynamicJsonDocument numberConfigDoc(512);
    numberConfigDoc["name"] = displayName;
    numberConfigDoc["command_topic"] = mqtt_topic + "/" + name + "/set";
    numberConfigDoc["state_topic"] = mqtt_topic + "/" + name;
    numberConfigDoc["unique_id"] = unique_id + "-" + name;
    numberConfigDoc["min"] = min_value;
    numberConfigDoc["max"] = max_value;
    numberConfigDoc["step"] = String(steps_value, 2);
    numberConfigDoc["unit_of_measurement"] = unit_of_measurement;
    numberConfigDoc["mode"] = ui_mode;
    numberConfigDoc["payload_available"] = "online";
    numberConfigDoc["payload_not_available"] = "offline";
    numberConfigDoc["availability_topic"] = mqtt_topic + "/status";

    JsonObject numberDeviceField = numberConfigDoc.createNestedObject("device");

    for (JsonPair keyValue : DeviceMapDoc.as<JsonObject>()) {
        numberDeviceField[keyValue.key()] = keyValue.value();
    }

    String numberConfigDocBuffer;
    serializeJson(numberConfigDoc, numberConfigDocBuffer);

    number_device.payload_json = numberConfigDocBuffer;

    return number_device;
}

/**
 * @brief Send MQTT Homeassistant Discovery Messages
 * @return 0 if successful, MQTT connection error code if failed to send messages
 */
inline int sendHASSIODiscoveryMsg() {
    // Sensor, number and switch objects which will always be published

    DiscoveryObject machineStateDevice = GenerateSensorDevice("machineState", "Machine State", "", "enum");
    DiscoveryObject actual_temperature = GenerateSensorDevice("temperature", "Boiler Temperature", "°C", "temperature");
    DiscoveryObject heaterPower = GenerateSensorDevice("heaterPower", "Heater Power", "%", "power_factor");

    DiscoveryObject brewSetpoint = GenerateNumberDevice("brewSetpoint", "Brew setpoint", BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, 0.1, "°C");
    DiscoveryObject steamSetpoint = GenerateNumberDevice("steamSetpoint", "Steam setpoint", STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, 0.1, "°C");
    DiscoveryObject brewTempOffset = GenerateNumberDevice("brewTempOffset", "Brew Temp. Offset", BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, 0.1, "°C");
    DiscoveryObject steamKp = GenerateNumberDevice("steamKp", "Steam Kp", PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, 0.1, "");
    DiscoveryObject aggKp = GenerateNumberDevice("aggKp", "aggKp", PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, 0.1, "");
    DiscoveryObject aggTn = GenerateNumberDevice("aggTn", "aggTn", PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, 0.1, "");
    DiscoveryObject aggTv = GenerateNumberDevice("aggTv", "aggTv", PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, 0.1, "");
    DiscoveryObject aggIMax = GenerateNumberDevice("aggIMax", "aggIMax", PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, 0.1, "");

    DiscoveryObject pidOn = GenerateSwitchDevice("pidON", "Use PID");
    DiscoveryObject steamON = GenerateSwitchDevice("steamON", "Steam");
    DiscoveryObject usePonM = GenerateSwitchDevice("usePonM", "Use PonM");

    // List of all DiscoveryObjects, will be always published
    std::vector discoveryObjects = {machineStateDevice, actual_temperature, heaterPower, brewSetpoint, steamSetpoint, brewTempOffset, steamKp, aggKp, aggTn, aggTv, aggIMax, pidOn, steamON, usePonM};

    // Sensor, number and switch object which will be published based on feature set
#if FEATURE_BREWSWITCH == 1
    DiscoveryObject currBrewTime = GenerateSensorDevice("currBrewTime", "Current Brew Time ", "s", "duration");

    DiscoveryObject brewPidDelay = GenerateNumberDevice("brewPidDelay", "Brew Pid Delay", BREW_PID_DELAY_MIN, BREW_PID_DELAY_MAX, 0.1, "s");
    DiscoveryObject targetBrewTime = GenerateNumberDevice("targetBrewTime", "Target Brew time", TARGET_BREW_TIME_MIN, TARGET_BREW_TIME_MAX, 0.1, "s");
    DiscoveryObject preinfusion = GenerateNumberDevice("preinfusion", "Preinfusion filling time", PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, 0.1, "s");
    DiscoveryObject preinfusionPause = GenerateNumberDevice("preinfusionPause", "Preinfusion pause time", PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, 0.1, "s");
    DiscoveryObject backflushCycles = GenerateNumberDevice("backflushCycles", "Backflush Cycles", BACKFLUSH_CYCLES_MIN, BACKFLUSH_CYCLES_MAX, 1, "");
    DiscoveryObject backflushFillTime = GenerateNumberDevice("backflushFillTime", "Backflush filling time", BACKFLUSH_FILL_TIME_MIN, BACKFLUSH_FILL_TIME_MAX, 0.1, "s");
    DiscoveryObject backflushFlushTime = GenerateNumberDevice("backflushFlushTime", "Backflush flushing time", BACKFLUSH_FLUSH_TIME_MIN, BACKFLUSH_FLUSH_TIME_MAX, 0.1, "s");

    DiscoveryObject backflushOn = GenerateSwitchDevice("backflushOn", "Backflush");

    discoveryObjects.push_back(currBrewTime);
    discoveryObjects.push_back(brewPidDelay);
    discoveryObjects.push_back(targetBrewTime);
    discoveryObjects.push_back(preinfusion);
    discoveryObjects.push_back(preinfusionPause);
    discoveryObjects.push_back(backflushCycles);
    discoveryObjects.push_back(backflushFillTime);
    discoveryObjects.push_back(backflushFlushTime);
    discoveryObjects.push_back(backflushOn);
#endif

#if FEATURE_SCALE == 1
    DiscoveryObject currReadingWeight = GenerateSensorDevice("currReadingWeight", "Weight", "g", "weight");
    DiscoveryObject currBrewWeight = GenerateSensorDevice("currBrewWeight", "current Brew Weight", "g", "weight");

    DiscoveryObject scaleCalibrateButton = GenerateButtonDevice("scaleCalibrationOn", "Calibrate Scale");
    DiscoveryObject scaleTareButton = GenerateButtonDevice("scaleTareOn", "Tare Scale");

    DiscoveryObject targetBrewWeight = GenerateNumberDevice("targetBrewWeight", "Brew Weight Target", TARGET_BREW_WEIGHT_MIN, TARGET_BREW_WEIGHT_MAX, 0.1, "g");

    discoveryObjects.push_back(currReadingWeight);
    discoveryObjects.push_back(currBrewWeight);
    discoveryObjects.push_back(scaleCalibrateButton);
    discoveryObjects.push_back(scaleTareButton);
    discoveryObjects.push_back(targetBrewWeight);
#endif

#if FEATURE_PRESSURESENSOR == 1

    DiscoveryObject pressure = GenerateSensorDevice("pressure", "Pressure", "bar", "pressure");

    discoveryObjects.push_back(pressure);
#endif

    // Send the Objects to Hassio
    if (mqtt.connected()) {
        for (const auto& [discovery_topic, payload_json] : discoveryObjects) {

            if (int publishResult = PublishLargeMessage(discovery_topic.c_str(), payload_json.c_str()); publishResult != 0) {
                LOGF(ERROR, "[MQTT] Failed to publish discovery message. Error code: %d\n", publishResult);
                return publishResult;
            }
        }

        return 0;
    }

    LOG(DEBUG, "[MQTT] Failed to send Hassio Discover, MQTT Client is not connected");

    return -1;
}
