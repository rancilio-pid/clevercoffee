/**
 * @file MQTT.h
 *
 * @brief MQTT message handling
 *
 */

#pragma once

#include "Parameter.h"
#include <Arduino.h>
#include <PubSubClient.h>
#include <map>
#include <os.h>
#include <string>

std::map<const char*, std::string> mqttLastSent;

inline unsigned long previousMillisMQTT;
const unsigned long intervalMQTT = 5000;
const unsigned long intervalMQTTbrew = 500;
const unsigned long intervalMQTTstandby = 10000;
unsigned long timeBudget = 10; // milliseconds per loop until all data is sent

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
unsigned long previousMqttConnection = millis();
unsigned long mqttReconnectInterval = 300000; // 5 minutes

extern std::map<const char*, const char*, cmp_str> mqttVars;
extern std::map<const char*, std::function<double()>, cmp_str> mqttSensors;

struct DiscoveryObject {
        char discovery_topic[160];
        char payload_json[650];
};

inline void setupMqtt() {
    ParameterRegistry& registry = ParameterRegistry::getInstance();

    if (!registry.isReady()) {
        LOGF(ERROR, "ParameterRegistry not ready, cannot initialize MQTT");
        return;
    }

    mqtt_enabled = registry.getParameterById("mqtt.enabled")->getValueAs<bool>();

    if (!mqtt_enabled) {
        return;
    }

    mqtt_server_ip = registry.getParameterById("mqtt.broker")->getValueAs<String>();
    mqtt_server_port = registry.getParameterById("mqtt.port")->getValueAs<int>();
    mqtt_username = registry.getParameterById("mqtt.username")->getValueAs<String>();
    mqtt_password = registry.getParameterById("mqtt.password")->getValueAs<String>();
    mqtt_topic_prefix = registry.getParameterById("mqtt.topic")->getValueAs<String>();
    mqtt_hassio_enabled = registry.getParameterById("mqtt.hassio.enabled")->getValueAs<bool>();
    mqtt_hassio_discovery_prefix = registry.getParameterById("mqtt.hassio.prefix")->getValueAs<String>();
}

/**
 * @brief Check if MQTT is connected, if not reconnect. Abort function if offline or brew is running
 *      MQTT is also using maxWifiReconnects!
 */
inline void checkMQTT() {
    if (offlineMode || checkBrewActive()) {
        return;
    }

    if (millis() - lastMQTTConnectionAttempt >= wifiConnectionDelay && MQTTReCnctCount <= maxWifiReconnects) {
        if (!mqtt.connected()) {
            lastMQTTConnectionAttempt = millis(); // Reconnection Timer Function
            MQTTReCnctCount++;                    // Increment reconnection Counter
            LOGF(DEBUG, "Attempting MQTT reconnection: %i", MQTTReCnctCount);

            // First clean up previous connection
            mqtt.disconnect(); // graceful disconnect, frees sockets
            delay(20);         // tiny delay to allow LWIP to reclaim resources

            if (mqtt.connect(hostname.c_str(), mqtt_username.c_str(), mqtt_password.c_str(), topic_will, 0, true, "offline")) {
                mqtt.subscribe(topic_set);
                LOGF(DEBUG, "Subscribed to MQTT Topic: %s", topic_set);
                MQTTReCnctCount = 0; // reset MQTT reconnect count to zero after a successful connection
            } // Try to reconnect to the server; connect() is a blocking
              // function, watch the timeout!
            else {
                LOGF(DEBUG, "Failed to connect to MQTT due to reason: %i", mqtt.state());
            }
        }
    }
    // reset MQTT reconnect count to zero after mqttReconnectInterval so it can try to connect again
    else if (millis() - previousMqttConnection >= mqttReconnectInterval) {
        MQTTReCnctCount = 0;
        previousMqttConnection = millis();
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
inline int PublishLargeMessage(const char* topic, const char* largeMessage) {
    constexpr size_t splitSize = 128; // Maximum Message Size

    if (const size_t messageLength = strlen(largeMessage); messageLength > splitSize) {
        const size_t count = messageLength / splitSize;
        mqtt.beginPublish(topic, messageLength, true);

        for (size_t i = 0; i < count; i++) {
            mqtt.print(String(largeMessage + i * splitSize).substring(0, splitSize));
        }

        mqtt.print(String(largeMessage + count * splitSize));

        if (int publishResult = mqtt.endPublish(); publishResult == 0) {
            LOG(WARNING, "[MQTT] PublishLargeMessage sent failed");
            return 1;
        }
        else {
            return 0;
        }
    }
    else {
        return mqtt.publish(topic, largeMessage) ? 0 : -1; // Return 0 for success, -1 for failure
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
        auto var = registry.getParameterById(parameterId);

        if (!var) {
            LOGF(WARNING, "Parameter %s not found in ParameterRegistry", parameterId);
            return;
        }

        if (value >= var->getMinValue() && value <= var->getMaxValue()) {
            bool success = false;
            char buf[12];

            switch (var->getType()) {
                case kDouble:
                    success = registry.setParameterValue(parameterId, value);

                    if (success) {
                        snprintf(buf, sizeof(buf), "%.2f", value);
                    }

                    break;
                case kFloat:
                    success = registry.setParameterValue(parameterId, static_cast<float>(value));

                    if (success) {
                        snprintf(buf, sizeof(buf), "%.2f", static_cast<float>(value));
                    }

                    break;
                case kUInt8:
                    success = registry.setParameterValue(parameterId, static_cast<uint8_t>(value));

                    if (success) {
                        snprintf(buf, sizeof(buf), "%u", static_cast<uint8_t>(value));

                        if (strcasecmp(param, "steamON") == 0) {
                            steamFirstON = value;
                        }
                    }

                    break;
                case kInteger:
                    success = registry.setParameterValue(parameterId, static_cast<int>(value));

                    if (success) {
                        snprintf(buf, sizeof(buf), "%d", static_cast<int>(value));
                    }

                    break;
                default:
                    LOGF(WARNING, "%s is not a recognized type for this MQTT parameter.", var->getType());
                    return;
            }

            if (success) {
                mqtt_publish(param, buf, true);
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
 * @brief Send all current system parameter values to MQTT if changed, exit early if process is taking too long and return next loop
 *
 * @param continueOnError Flag to specify whether to continue publishing messages in case of an error (default: true)
 * @return 0 = success, MQTT error code = failure
 */

inline int writeSysParamsToMQTT(const bool continueOnError = true) {
    static auto mqttVarsIt = mqttVars.begin();
    static auto mqttSensorsIt = mqttSensors.begin();
    static bool inSensors = false;

    unsigned long currentMillisMQTT = millis();
    unsigned long interval = (machineState == kBrew) ? intervalMQTTbrew : (machineState == kStandby) ? intervalMQTTstandby : intervalMQTT;

    if ((currentMillisMQTT - previousMillisMQTT < interval) || !mqtt_enabled || !mqtt.connected()) {
        return 0;
    }

    if (!inSensors && mqttVarsIt == mqttVars.begin()) {
        previousMillisMQTT = currentMillisMQTT;
        mqtt_publish("status", (char*)"online");
    }

    mqttUpdateRunning = true;
    unsigned long start = millis();

    char data[12];
    int errorState = 0;
    auto& registry = ParameterRegistry::getInstance();

    if (!inSensors) {
        // Iterate through the mqttVars mapping to publish parameters
        while (mqttVarsIt != mqttVars.end()) {
            const char* mqttTopic = mqttVarsIt->first;
            const char* parameterId = mqttVarsIt->second;

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
                    snprintf(data, sizeof(data), "%d", param->getValueAs<int>());
                    break;
                case kUInt8:
                    snprintf(data, sizeof(data), "%u", param->getValueAs<uint8_t>());
                    break;
                case kDouble:
                    snprintf(data, sizeof(data), "%.2f", param->getValueAs<double>());
                    break;
                case kFloat:
                    snprintf(data, sizeof(data), "%.2f", param->getValueAs<float>());
                    break;
                case kCString:
                    snprintf(data, sizeof(data), "%s", param->getValueAs<String>().c_str());
                    break;
                default:

                    if (!continueOnError) {
                        LOGF(ERROR, "Unknown parameter type for topic %s", mqttTopic);
                        return 1;
                    }

                    LOGF(WARNING, "Skipping unknown parameter type for topic %s", mqttTopic);
                    continue;
            }

            if (mqttLastSent[mqttTopic].compare(data) != 0) {
                if (!mqtt_publish(mqttTopic, data, true)) {
                    errorState = mqtt.state();

                    if (!continueOnError) {
                        LOGF(ERROR, "Failed to publish parameter %s to MQTT, error: %d", mqttTopic, errorState);
                        return errorState;
                    }

                    LOGF(WARNING, "Failed to publish parameter %s to MQTT, error: %d", mqttTopic, errorState);
                }
                else {
                    mqttLastSent[mqttTopic].assign(data); // Update only if sent successfully
                    LOGF(DEBUG, "Published %s = %s to MQTT, length: %i", mqttTopic, data, strlen(data) + 1);
                }
            }

            ++mqttVarsIt;

            // Return early, continue next time
            if (millis() - start >= timeBudget) {
                return 0;
            }
        }

        // Done with mqttVars, start sensors
        mqttVarsIt = mqttVars.begin();
        inSensors = true;
    }

    while (mqttSensorsIt != mqttSensors.end()) {
        const char* topic = mqttSensorsIt->first;
        const auto& sensorFunc = mqttSensorsIt->second;

        char buf[20];

        if (strcmp(topic, "machineState") == 0) {
            snprintf(buf, sizeof(buf), "%s", machinestateEnumToString(machineState));
        }
        else {
            snprintf(buf, sizeof(buf), "%.2f", sensorFunc());
        }

        if (mqttLastSent[topic].compare(buf) != 0) {

            if (!mqtt_publish(topic, buf)) {
                errorState = mqtt.state();

                if (!continueOnError) {
                    return errorState;
                }
            }
            else {
                mqttLastSent[topic].assign(buf);
            }
        }

        ++mqttSensorsIt;

        if (millis() - start >= timeBudget) {
            return 0; // Return early, continue next time
        }
    }

    // Done with both loops
    mqttSensorsIt = mqttSensors.begin();
    inSensors = false;

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
inline DiscoveryObject GenerateSwitchDevice(const char* name, const char* displayName, const char* payload_on = "1", const char* payload_off = "0") {
    DiscoveryObject switch_device;

    char mqtt_topic[128];
    char unique_id[80];
    char topic_buffer[160];

    snprintf(mqtt_topic, sizeof(mqtt_topic), "%s%s", mqtt_topic_prefix.c_str(), hostname.c_str());
    snprintf(unique_id, sizeof(unique_id), "clevercoffee-%s", hostname);
    snprintf(switch_device.discovery_topic, sizeof(switch_device.discovery_topic), "%s/switch/%s/%s/config", mqtt_hassio_discovery_prefix, unique_id, name);

    JsonDocument switchConfigDoc;
    switchConfigDoc["name"] = displayName;
    snprintf(topic_buffer, sizeof(topic_buffer), "%s/%s/set", mqtt_topic, name);
    switchConfigDoc["command_topic"] = String(topic_buffer);
    snprintf(topic_buffer, sizeof(topic_buffer), "%s/%s", mqtt_topic, name);
    switchConfigDoc["state_topic"] = String(topic_buffer);
    snprintf(topic_buffer, sizeof(topic_buffer), "%s-%s", unique_id, name);
    switchConfigDoc["unique_id"] = String(topic_buffer);
    switchConfigDoc["payload_on"] = payload_on;
    switchConfigDoc["payload_off"] = payload_off;
    switchConfigDoc["payload_available"] = "online";
    switchConfigDoc["payload_not_available"] = "offline";
    snprintf(topic_buffer, sizeof(topic_buffer), "%s/status", mqtt_topic);
    switchConfigDoc["availability_topic"] = String(topic_buffer);

    JsonObject device = switchConfigDoc["device"].to<JsonObject>();
    device["identifiers"] = hostname;
    device["manufacturer"] = "CleverCoffee";
    device["name"] = hostname;

    serializeJson(switchConfigDoc, switch_device.payload_json, sizeof(switch_device.payload_json));

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
inline DiscoveryObject GenerateButtonDevice(const char* name, const char* displayName, const char* payload_press = "1") {
    DiscoveryObject button_device;

    char mqtt_topic[128];
    char unique_id[80];
    char topic_buffer[160];

    snprintf(mqtt_topic, sizeof(mqtt_topic), "%s%s", mqtt_topic_prefix.c_str(), hostname.c_str());
    snprintf(unique_id, sizeof(unique_id), "clevercoffee-%s", hostname);
    snprintf(button_device.discovery_topic, sizeof(button_device.discovery_topic), "%s/button/%s/%s/config", mqtt_hassio_discovery_prefix, unique_id, name);

    JsonDocument buttonConfigDoc;
    buttonConfigDoc["name"] = displayName;
    snprintf(topic_buffer, sizeof(topic_buffer), "%s/%s/set", mqtt_topic, name);
    buttonConfigDoc["command_topic"] = String(topic_buffer);
    snprintf(topic_buffer, sizeof(topic_buffer), "%s/%s", mqtt_topic, name);
    buttonConfigDoc["state_topic"] = String(topic_buffer);
    snprintf(topic_buffer, sizeof(topic_buffer), "%s-%s", unique_id, name);
    buttonConfigDoc["unique_id"] = String(topic_buffer);
    buttonConfigDoc["payload_press"] = payload_press;
    buttonConfigDoc["payload_available"] = "online";
    buttonConfigDoc["payload_not_available"] = "offline";
    snprintf(topic_buffer, sizeof(topic_buffer), "%s/status", mqtt_topic);
    buttonConfigDoc["availability_topic"] = String(topic_buffer);

    JsonObject device = buttonConfigDoc["device"].to<JsonObject>();
    device["identifiers"] = hostname;
    device["manufacturer"] = "CleverCoffee";
    device["name"] = hostname;

    serializeJson(buttonConfigDoc, button_device.payload_json, sizeof(button_device.payload_json));

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
inline DiscoveryObject GenerateSensorDevice(const char* name, const char* displayName, const char* unit_of_measurement, const char* device_class, const std::vector<const char*>& options = {}) {
    DiscoveryObject sensor_device;

    char mqtt_topic[128];
    char unique_id[80];
    char topic_buffer[160];

    snprintf(mqtt_topic, sizeof(mqtt_topic), "%s%s", mqtt_topic_prefix.c_str(), hostname.c_str());
    snprintf(unique_id, sizeof(unique_id), "clevercoffee-%s", hostname);
    snprintf(sensor_device.discovery_topic, sizeof(sensor_device.discovery_topic), "%s/sensor/%s/%s/config", mqtt_hassio_discovery_prefix, unique_id, name);

    JsonDocument sensorConfigDoc;
    sensorConfigDoc["name"] = displayName;
    snprintf(topic_buffer, sizeof(topic_buffer), "%s/%s", mqtt_topic, name);
    sensorConfigDoc["state_topic"] = String(topic_buffer);
    snprintf(topic_buffer, sizeof(topic_buffer), "%s-%s", unique_id, name);
    sensorConfigDoc["unique_id"] = String(topic_buffer);
    sensorConfigDoc["unit_of_measurement"] = unit_of_measurement;
    sensorConfigDoc["device_class"] = device_class;
    sensorConfigDoc["payload_available"] = "online";
    sensorConfigDoc["payload_not_available"] = "offline";
    snprintf(topic_buffer, sizeof(topic_buffer), "%s/status", mqtt_topic);
    sensorConfigDoc["availability_topic"] = String(topic_buffer);

    if (!options.empty()) {
        sensorConfigDoc["options"] = JsonArray();
        for (auto& opt : options) {
            sensorConfigDoc["options"].add(opt);
        }
    }

    JsonObject device = sensorConfigDoc["device"].to<JsonObject>();
    device["identifiers"] = hostname;
    device["manufacturer"] = "CleverCoffee";
    device["name"] = hostname;

    serializeJson(sensorConfigDoc, sensor_device.payload_json, sizeof(sensor_device.payload_json));

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
inline DiscoveryObject GenerateNumberDevice(const char* name, const char* displayName, int min_value, int max_value, float steps_value, const char* unit_of_measurement, const char* ui_mode = "box") {
    DiscoveryObject number_device;

    char mqtt_topic[128];
    char unique_id[80];
    char topic_buffer[160];

    snprintf(mqtt_topic, sizeof(mqtt_topic), "%s%s", mqtt_topic_prefix.c_str(), hostname.c_str());
    snprintf(unique_id, sizeof(unique_id), "clevercoffee-%s", hostname);
    snprintf(number_device.discovery_topic, sizeof(number_device.discovery_topic), "%s/number/%s/%s/config", mqtt_hassio_discovery_prefix, unique_id, name);

    JsonDocument numberConfigDoc;
    numberConfigDoc["name"] = displayName;
    snprintf(topic_buffer, sizeof(topic_buffer), "%s/%s/set", mqtt_topic, name);
    numberConfigDoc["command_topic"] = String(topic_buffer);
    snprintf(topic_buffer, sizeof(topic_buffer), "%s/%s", mqtt_topic, name);
    numberConfigDoc["state_topic"] = String(topic_buffer);
    snprintf(topic_buffer, sizeof(topic_buffer), "%s-%s", unique_id, name);
    numberConfigDoc["unique_id"] = String(topic_buffer);
    numberConfigDoc["min"] = min_value;
    numberConfigDoc["max"] = max_value;
    snprintf(topic_buffer, sizeof(topic_buffer), "%.2f", steps_value);
    numberConfigDoc["step"] = String(topic_buffer);
    numberConfigDoc["unit_of_measurement"] = unit_of_measurement;
    numberConfigDoc["mode"] = ui_mode;
    numberConfigDoc["payload_available"] = "online";
    numberConfigDoc["payload_not_available"] = "offline";
    snprintf(topic_buffer, sizeof(topic_buffer), "%s/status", mqtt_topic);
    numberConfigDoc["availability_topic"] = String(topic_buffer);

    JsonObject device = numberConfigDoc["device"].to<JsonObject>();
    device["identifiers"] = hostname;
    device["manufacturer"] = "CleverCoffee";
    device["name"] = hostname;

    serializeJson(numberConfigDoc, number_device.payload_json, sizeof(number_device.payload_json));

    return number_device;
}

/**
 * @brief Send MQTT Homeassistant Discovery Messages
 * @return 0 if successful, MQTT connection error code if failed to send messages
 */
inline int publishDiscovery(const DiscoveryObject& obj) {
    if (obj.discovery_topic == nullptr || obj.discovery_topic[0] == '\0' || obj.payload_json == nullptr || obj.payload_json[0] == '\0') {
        LOGF(WARNING, "[MQTT] Skipping invalid discovery message: topic or payload is empty");
        return 1;
    }

    LOGF(DEBUG, "Publishing topic: %s, payload length: %d", obj.discovery_topic, strlen(obj.payload_json));

    int result = PublishLargeMessage(obj.discovery_topic, obj.payload_json);

    if (result != 0) {
        LOGF(ERROR, "[MQTT] Failed to publish discovery message. Error code: %d", result);
        return 1;
    }

    return 0;
}

/**
 * @brief Send MQTT Homeassistant Discovery Messages
 * @return 0 if successful, MQTT connection error code if failed to send messages. Sets failed flag for later retry
 */
inline int sendHASSIODiscoveryMsg() {
    hassioUpdateRunning = true;

    if (!mqtt.connected()) {
        LOG(DEBUG, "[MQTT] Failed to send Hassio Discover, MQTT Client is not connected");
        hassioFailed = true;
        hassioUpdateRunning = false;
        return -1;
    }

    int failures = 0;

    // Always published devices
    failures += publishDiscovery(GenerateSensorDevice("machineState", "Machine State", "", "enum", getMachineStateOptions()));
    failures += publishDiscovery(GenerateSensorDevice("temperature", "Boiler Temperature", "°C", "temperature"));
    failures += publishDiscovery(GenerateSensorDevice("heaterPower", "Heater Power", "%", "power_factor"));

    failures += publishDiscovery(GenerateNumberDevice("brewSetpoint", "Brew setpoint", BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, 0.1, "°C"));
    failures += publishDiscovery(GenerateNumberDevice("steamSetpoint", "Steam setpoint", STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, 0.1, "°C"));
    failures += publishDiscovery(GenerateNumberDevice("brewTempOffset", "Brew Temp. Offset", BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, 0.1, "°C"));
    failures += publishDiscovery(GenerateNumberDevice("steamKp", "Steam Kp", PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, 0.1, ""));
    failures += publishDiscovery(GenerateNumberDevice("aggKp", "aggKp", PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, 0.1, ""));
    failures += publishDiscovery(GenerateNumberDevice("aggTn", "aggTn", PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, 0.1, ""));
    failures += publishDiscovery(GenerateNumberDevice("aggTv", "aggTv", PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, 0.1, ""));
    failures += publishDiscovery(GenerateNumberDevice("aggIMax", "aggIMax", PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, 0.1, ""));

    failures += publishDiscovery(GenerateSwitchDevice("pidON", "Use PID"));
    failures += publishDiscovery(GenerateSwitchDevice("steamON", "Steam"));
    failures += publishDiscovery(GenerateSwitchDevice("usePonM", "Use PonM"));

    // Conditional devices
    if (config.get<bool>("hardware.switches.brew.enabled")) {
        failures += publishDiscovery(GenerateSensorDevice("currBrewTime", "Current Brew Time ", "s", "duration"));
        failures += publishDiscovery(GenerateNumberDevice("brewPidDelay", "Brew Pid Delay", BREW_PID_DELAY_MIN, BREW_PID_DELAY_MAX, 0.1, "s"));
        failures += publishDiscovery(GenerateNumberDevice("targetBrewTime", "Target Brew time", TARGET_BREW_TIME_MIN, TARGET_BREW_TIME_MAX, 0.1, "s"));
        failures += publishDiscovery(GenerateNumberDevice("preinfusion", "Preinfusion filling time", PRE_INFUSION_TIME_MIN, PRE_INFUSION_TIME_MAX, 0.1, "s"));
        failures += publishDiscovery(GenerateNumberDevice("preinfusionPause", "Preinfusion pause time", PRE_INFUSION_PAUSE_MIN, PRE_INFUSION_PAUSE_MAX, 0.1, "s"));
        failures += publishDiscovery(GenerateNumberDevice("backflushCycles", "Backflush Cycles", BACKFLUSH_CYCLES_MIN, BACKFLUSH_CYCLES_MAX, 1, ""));
        failures += publishDiscovery(GenerateNumberDevice("backflushFillTime", "Backflush filling time", BACKFLUSH_FILL_TIME_MIN, BACKFLUSH_FILL_TIME_MAX, 0.1, "s"));
        failures += publishDiscovery(GenerateNumberDevice("backflushFlushTime", "Backflush flushing time", BACKFLUSH_FLUSH_TIME_MIN, BACKFLUSH_FLUSH_TIME_MAX, 0.1, "s"));
        failures += publishDiscovery(GenerateSwitchDevice("backflushOn", "Backflush"));
    }

    if (config.get<bool>("hardware.sensors.scale.enabled")) {
        failures += publishDiscovery(GenerateSensorDevice("currReadingWeight", "Weight", "g", "weight"));
        failures += publishDiscovery(GenerateSensorDevice("currBrewWeight", "current Brew Weight", "g", "weight"));
        failures += publishDiscovery(GenerateButtonDevice("scaleCalibrationOn", "Calibrate Scale"));
        failures += publishDiscovery(GenerateButtonDevice("scaleTareOn", "Tare Scale"));
        failures += publishDiscovery(GenerateNumberDevice("targetBrewWeight", "Brew Weight Target", TARGET_BREW_WEIGHT_MIN, TARGET_BREW_WEIGHT_MAX, 0.1, "g"));
    }

    if (config.get<bool>("hardware.sensors.pressure.enabled")) {
        failures += publishDiscovery(GenerateSensorDevice("pressure", "Pressure", "bar", "pressure"));
    }

    if (failures > 0) {
        LOGF(DEBUG, "Hassio failed to send %d entries", failures);
        hassioFailed = true;
    }
    else {
        LOG(DEBUG, "Hassio send successful");
        hassioFailed = false;
        return 0;
    }

    return -1;
}
