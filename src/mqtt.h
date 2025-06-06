/**
 * @file MQTT.h
 *
 * @brief MQTT message handling
 *
 */

#pragma once

#include "userConfig.h"
#include <Arduino.h>
#include <PubSubClient.h>
#include <map>
#include <os.h>
#include <string>

std::map<std::string, std::string> mqttLastSent;

unsigned long previousMillisMQTT;
const unsigned long intervalMQTT = 5000;
const unsigned long intervalMQTTbrew = 500;
const unsigned long intervalMQTTstandby = 10000;

WiFiClient net;
PubSubClient mqtt(net);

const char* mqtt_server_ip = MQTT_SERVER_IP;
const int mqtt_server_port = MQTT_SERVER_PORT;
const char* mqtt_username = MQTT_USERNAME;
const char* mqtt_password = MQTT_PASSWORD;
const char* mqtt_topic_prefix = MQTT_TOPIC_PREFIX;

char topic_will[256];
char topic_set[256];

unsigned long lastMQTTConnectionAttempt = millis();
unsigned int MQTTReCnctCount = 0;
unsigned long previousMqttConnection = millis();
unsigned long mqttReconnectInterval = 600000; // 10 minutes

extern std::map<const char*, std::function<editable_t*()>, cmp_str> mqttVars;
extern std::map<const char*, std::function<double()>, cmp_str> mqttSensors;

struct DiscoveryObject {
        String discovery_topic;
        String payload_json;
};

/**
 * @brief Check if MQTT is connected, if not reconnect. Abort function if offline or brew is running
 *      MQTT is also using maxWifiReconnects!
 */
void checkMQTT() {
    if (offlineMode == 1 || currBrewState > kBrewIdle) {
        return;
    }

    if ((millis() - lastMQTTConnectionAttempt >= wifiConnectionDelay) && (MQTTReCnctCount <= maxWifiReconnects)) {
        if (!mqtt.connected()) {
            lastMQTTConnectionAttempt = millis(); // Reconnection Timer Function
            MQTTReCnctCount++;                    // Increment reconnection Counter
            LOGF(DEBUG, "Attempting MQTT reconnection: %i", MQTTReCnctCount);

            if (mqtt.connect(hostname, mqtt_username, mqtt_password, topic_will, 0, true, "offline")) {
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
bool mqtt_publish(const char* reading, char* payload, boolean retain = false) {
    char topic[120];
    snprintf(topic, 120, "%s%s/%s", mqtt_topic_prefix, hostname, reading);
    return mqtt.publish(topic, payload, retain);
}

/**
 * @brief Publishes a large message to an MQTT topic, splitting it into smaller chunks if necessary.
 *
 * @param topic The MQTT topic to publish the message to.
 * @param largeMessage The large message to be published.
 * @return 0 if the message was successfully published, otherwise an MQTT error code.
 */
int PublishLargeMessage(const String& topic, const String& largeMessage) {
    const size_t splitSize = 128; // Maximum Message Size
    const size_t messageLength = largeMessage.length();

    if (messageLength > splitSize) {
        size_t count = messageLength / splitSize;
        mqtt.beginPublish(topic.c_str(), messageLength, true);

        for (size_t i = 0; i < count; i++) {
            size_t startIndex = i * splitSize;
            size_t endIndex = startIndex + splitSize;
            mqtt.print(largeMessage.substring(startIndex, endIndex));
        }

        mqtt.print(largeMessage.substring(count * splitSize));
        int publishResult = mqtt.endPublish();

        if (publishResult == 0) {
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
void assignMQTTParam(char* param, double value) {
    try {
        editable_t* var = mqttVars.at(param)();

        if (value >= var->minValue && value <= var->maxValue) {
            switch (var->type) {
                case kDouble:
                    *(double*)var->ptr = value;
                    mqtt_publish(param, number2string(value), true);
                    break;

                case kFloat:
                    *(float*)var->ptr = static_cast<float>(value);
                    mqtt_publish(param, number2string(static_cast<float>(value)), true);
                    break;

                case kUInt8:
                    *(uint8_t*)var->ptr = static_cast<uint8_t>(value);

                    if (strcasecmp(param, "steamON") == 0) {
                        steamFirstON = value;
                    }

                    mqtt_publish(param, number2string(static_cast<uint8_t>(value)), true);
                    writeSysParamsToStorage();
                    break;

                case kInteger:
                    *(int*)var->ptr = static_cast<int>(value);
                    mqtt_publish(param, number2string(static_cast<int>(value)), true);
                    break;

                default:
                    LOGF(WARNING, "%s is not a recognized type for this MQTT parameter.", var->type);
                    break;
            }
        }
        else {
            LOGF(WARNING, "%s is not a valid MQTT parameter.", param);
        }
    } catch (const std::out_of_range& e) {
        LOGF(WARNING, "Value out of range for MQTT parameter %s", param);
    }
}

/**
 * @brief MQTT Callback Function: set Parameters through MQTT
 */
void mqtt_callback(char* topic, byte* data, unsigned int length) {
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

    snprintf(topic_pattern, sizeof(topic_pattern), "%s%s/%%[^\\/]/%%[^\\/]", mqtt_topic_prefix, hostname);

    if ((sscanf(topic_str, topic_pattern, &configVar, &cmd) != 2) || (strcmp(cmd, "set") != 0)) {
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
int writeSysParamsToMQTT(bool continueOnError = true) {
    unsigned long currentMillisMQTT = millis();
    unsigned long interval = (machineState == kBrew) ? intervalMQTTbrew : (machineState == kStandby) ? intervalMQTTstandby : intervalMQTT;

    if ((currentMillisMQTT - previousMillisMQTT >= interval) && FEATURE_MQTT == 1) {
        previousMillisMQTT = currentMillisMQTT;
        mqttUpdateRunning = true;

        if (mqtt.connected()) {
            mqtt_publish("status", (char*)"online");

            int errorState = 0;
            char buffer[32]; // shared buffer for snprintf

            for (const auto& pair : mqttVars) {
                editable_t* e = pair.second();

                switch (e->type) {
                    case kFloat:
                        snprintf(buffer, sizeof(buffer), "%.2f", *(float*)e->ptr);
                        break;
                    case kDouble:
                    case kDoubletime:
                        snprintf(buffer, sizeof(buffer), "%.2f", *(double*)e->ptr);
                        break;
                    case kInteger:
                        snprintf(buffer, sizeof(buffer), "%d", *(int*)e->ptr);
                        break;
                    case kUInt8:
                        snprintf(buffer, sizeof(buffer), "%u", *(uint8_t*)e->ptr);
                        break;
                    case kCString:
                        snprintf(buffer, sizeof(buffer), "%s", (char*)e->ptr);
                        break;
                    default:
                        continue; // Skip unknown types
                }

                std::string topic = pair.first;
                std::string value = std::string(buffer);

                if (mqttLastSent[topic] != value) {
                    if (mqtt_publish(topic.c_str(), buffer, true)) {
                        mqttLastSent[topic] = value; // Update only if sent successfully
                    }
                    else {
                        errorState = mqtt.state();
                        if (errorState != 0 && !continueOnError) {
                            // An error occurred and continueOnError is false, return the error state
                            return errorState;
                        }
                    }
                }
            }

            // === mqttSensors loop ===
            for (const auto& pair : mqttSensors) {
                snprintf(buffer, sizeof(buffer), "%.2f", pair.second());
                std::string topic = pair.first;
                std::string value = std::string(buffer);

                if (mqttLastSent[topic] != value) {
                    if (mqtt_publish(topic.c_str(), buffer)) {
                        mqttLastSent[topic] = value;
                    }
                    else {
                        errorState = mqtt.state();
                        if (errorState != 0 && !continueOnError) {
                            // An error occurred and continueOnError is false, return the error state
                            return errorState;
                        }
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
DiscoveryObject GenerateSwitchDevice(String name, String displayName, String payload_on = "1", String payload_off = "0") {
    String mqtt_topic = String(mqtt_topic_prefix) + String(hostname);
    DiscoveryObject switch_device;
    String unique_id = "clevercoffee-" + String(hostname);
    String SwitchDiscoveryTopic = String(MQTT_HASSIO_DISCOVERY_PREFIX) + "/switch/";

    String switch_command_topic = mqtt_topic + "/" + name + "/set";
    String switch_state_topic = mqtt_topic + "/" + name;

    switch_device.discovery_topic = SwitchDiscoveryTopic + unique_id + "-" + name + "" + "/config";

    DynamicJsonDocument DeviceMapDoc(1024);
    DeviceMapDoc["identifiers"] = String(hostname);
    DeviceMapDoc["manufacturer"] = "CleverCoffee";
    DeviceMapDoc["name"] = String(hostname);

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
DiscoveryObject GenerateButtonDevice(String name, String displayName, String payload_press = "1") {
    String mqtt_topic = String(mqtt_topic_prefix) + String(hostname);
    DiscoveryObject button_device;
    String unique_id = "clevercoffee-" + String(hostname);
    String buttonDiscoveryTopic = String(MQTT_HASSIO_DISCOVERY_PREFIX) + "/button/";

    String button_command_topic = mqtt_topic + "/" + name + "/set";
    String button_state_topic = mqtt_topic + "/" + name;

    button_device.discovery_topic = buttonDiscoveryTopic + unique_id + "-" + name + "" + "/config";

    DynamicJsonDocument DeviceMapDoc(1024);
    DeviceMapDoc["identifiers"] = String(hostname);
    DeviceMapDoc["manufacturer"] = "CleverCoffee";
    DeviceMapDoc["name"] = String(hostname);

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
 * @return A `DiscoveryObject` containing the sensor device configuration
 */
DiscoveryObject GenerateSensorDevice(String name, String displayName, String unit_of_measurement, String device_class) {
    String mqtt_topic = String(mqtt_topic_prefix) + String(hostname);
    DiscoveryObject sensor_device;
    String unique_id = "clevercoffee-" + String(hostname);
    String SensorDiscoveryTopic = String(MQTT_HASSIO_DISCOVERY_PREFIX) + "/sensor/";

    String sensor_state_topic = mqtt_topic + "/" + name;
    sensor_device.discovery_topic = SensorDiscoveryTopic + unique_id + "-" + name + "" + "/config";

    DynamicJsonDocument DeviceMapDoc(1024);
    DeviceMapDoc["identifiers"] = String(hostname);
    DeviceMapDoc["manufacturer"] = "CleverCoffee";
    DeviceMapDoc["name"] = String(hostname);

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
DiscoveryObject GenerateNumberDevice(String name, String displayName, int min_value, int max_value, float steps_value, String unit_of_measurement, String ui_mode = "box") {
    String mqtt_topic = String(mqtt_topic_prefix) + String(hostname);
    DiscoveryObject number_device;
    String unique_id = "clevercoffee-" + String(hostname);

    String NumberDiscoveryTopic = String(MQTT_HASSIO_DISCOVERY_PREFIX) + "/number/";
    number_device.discovery_topic = NumberDiscoveryTopic + unique_id + "-" + name + "" + "/config";

    DynamicJsonDocument DeviceMapDoc(1024);
    DeviceMapDoc["identifiers"] = String(hostname);
    DeviceMapDoc["manufacturer"] = "CleverCoffee";
    DeviceMapDoc["name"] = String(hostname);

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
int sendHASSIODiscoveryMsg() {
    hassioUpdateRunning = true;

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
    std::vector<DiscoveryObject> discoveryObjects = {machineStateDevice, actual_temperature, heaterPower, brewSetpoint, steamSetpoint, brewTempOffset, steamKp, aggKp, aggTn, aggTv, aggIMax, pidOn, steamON, usePonM};

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
        for (int i = 0; i < discoveryObjects.size(); i++) {
            const DiscoveryObject& discoveryObj = discoveryObjects[i];
            int publishResult = PublishLargeMessage(discoveryObj.discovery_topic.c_str(), discoveryObj.payload_json.c_str());

            if (publishResult != 0) {
                LOGF(ERROR, "[MQTT] Failed to publish discovery message. Error code: %d\n", publishResult);
                return publishResult;
            }
        }

        return 0;
    }
    else {
        LOG(DEBUG, "[MQTT] Failed to send Hassio Discover, MQTT Client is not connected");
        return -1;
    }
}
