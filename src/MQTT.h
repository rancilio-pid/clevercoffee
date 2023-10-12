/**
 * @file MQTT.h
 *
 * @brief MQTT message handling
 *
 */

#pragma once

#include "userConfig.h"
#include <os.h>
#include <Arduino.h>
#include <PubSubClient.h>

unsigned long previousMillisMQTT;   // initialisation at the end of init()
const unsigned long intervalMQTT = 5000;

WiFiClient net;
PubSubClient mqtt(net);

const char *mqtt_server_ip = MQTT_SERVER_IP;
const int mqtt_server_port = MQTT_SERVER_PORT;
const char *mqtt_username = MQTT_USERNAME;
const char *mqtt_password = MQTT_PASSWORD;
const char *mqtt_topic_prefix = MQTT_TOPIC_PREFIX;

char topic_will[256];
char topic_set[256];

unsigned long lastMQTTConnectionAttempt = millis();
unsigned int MQTTReCnctCount = 0;

extern std::map<const char*, std::function<editable_t*()>, cmp_str> mqttVars;
extern std::map<const char*, std::function<double()>, cmp_str> mqttSensors;

struct DiscoveryObject {
  String discovery_topic;
  String payload_json;
};

/**
 * @brief Get the string name corresponding to a MACHINE enum value.
 * 
 * @param machine The MACHINE enum value.
 * @return String The string name of the MACHINE.
 */
String getMachineName(MACHINE machine) {
    switch (machine) {
        case RancilioSilvia:
            return "RancilioSilvia";
        case RancilioSilviaE:
            return "RancilioSilviaE";
        case Gaggia:
            return "Gaggia";
        case QuickMill:
            return "QuickMill";
        default:
            return "";  // Handle any unknown or invalid values
    }
}


/**
 * @brief Check if MQTT is connected, if not reconnect. Abort function if offline or brew is running
 *      MQTT is also using maxWifiReconnects!
 */
void checkMQTT() {
    if (offlineMode == 1 || brewcounter > kBrewIdle) return;

    if ((millis() - lastMQTTConnectionAttempt >= wifiConnectionDelay) && (MQTTReCnctCount <= maxWifiReconnects)) {
        if (!mqtt.connected()) {
            lastMQTTConnectionAttempt = millis();  // Reconnection Timer Function
            MQTTReCnctCount++;                     // Increment reconnection Counter
            debugPrintf("Attempting MQTT reconnection: %i\n", MQTTReCnctCount);

            if (mqtt.connect(hostname, mqtt_username, mqtt_password, topic_will, 0, true, "offline")) {
                mqtt.subscribe(topic_set);
                debugPrintf("Subscribed to MQTT Topic: %s\n", topic_set);
            }   // Try to reconnect to the server; connect() is a blocking
                // function, watch the timeout!
            else {
                debugPrintf("Failed to connect to MQTT due to reason: %i\n", mqtt.state());
            }
        }
    }
}


/**
 * @brief Publish Data to MQTT
 */
bool mqtt_publish(const char *reading, char *payload, boolean retain = false) {
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
      debugPrintln("[MQTT] PublishLargeMessage sent failed");
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
void assignMQTTParam(char *param, double value) {
    try
    {
        editable_t *var = mqttVars.at(param)();
        if (value >= var->minValue && value <= var->maxValue) {
            switch (var->type) {
                case kDouble:
                    *(double *)var->ptr = value;
                    mqtt_publish(param, number2string(value), true);
                    break;
                case kUInt8:
                    *(uint8_t *)var->ptr = value;
                    if (strcasecmp(param, "steamON") == 0) {
                        steamFirstON = value;
                    }
                    mqtt_publish(param, number2string(value), true);
                    writeSysParamsToStorage();
                    break;
                default:
                    debugPrintf("%s is not a recognized type for this MQTT parameter.\n", var->type);
                    break;
            }
        }
        else {
            debugPrintf("Value out of range for MQTT parameter %s\n", param);
        }
    }
    catch(const std::out_of_range& e) {
        debugPrintf("%s is not a valid MQTT parameter.\n", param);
    }
}


/**
 * @brief MQTT Callback Function: set Parameters through MQTT
 */
void mqtt_callback(char *topic, byte *data, unsigned int length) {
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
        debugPrintf("Invalid MQTT topic/command: %s\n", topic_str);
        return;
    }
    debugPrintf("Received MQTT command %s %s\n", topic_str, data_str);

    //convert received string value to double assuming it's a number
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
  if ((currentMillisMQTT - previousMillisMQTT >= intervalMQTT) && MQTT == 1) {
    previousMillisMQTT = currentMillisMQTT;

    if (mqtt.connected()) {
      mqtt_publish("status", (char *)"online");

      int errorState = 0; // MQTT error state

      for (const auto& pair : mqttVars) {
        editable_t *e = pair.second();

        switch (e->type) {
          case kDouble:
            if (!mqtt_publish(pair.first, number2string(*(double *)e->ptr), true))
              errorState = mqtt.state();
            break;
          case kDoubletime:
            if (!mqtt_publish(pair.first, number2string(*(double *)e->ptr), true))
              errorState = mqtt.state();
            break;
          case kInteger:
            if (!mqtt_publish(pair.first, number2string(*(int *)e->ptr), true))
              errorState = mqtt.state();
            break;
          case kUInt8:
            if (!mqtt_publish(pair.first, number2string(*(uint8_t *)e->ptr), true))
              errorState = mqtt.state();
            break;
          case kCString:
            if (!mqtt_publish(pair.first, number2string(*(char *)e->ptr), true))
              errorState = mqtt.state();
            break;
        }

        if (errorState != 0 && !continueOnError) {
          // An error occurred and continueOnError is false, return the error state
          return errorState;
        }
      }

      for (const auto& pair : mqttSensors) {
        if (!mqtt_publish(pair.first, number2string(pair.second())))
          errorState = mqtt.state();

        if (errorState != 0 && !continueOnError) {
          // An error occurred and continueOnError is false, return the error state
          return errorState;
        }
      }
    }
  }

  return 0; // Success
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

  String switch_command_topic = mqtt_topic + "/"+ name +"/set";
  String switch_state_topic = mqtt_topic + "/"+ name;

  switch_device.discovery_topic = SwitchDiscoveryTopic + unique_id + "-"+ name +"" +"/config";
  
  DynamicJsonDocument DeviceMapDoc(1024);
  char DeviceMapBuffer[256];
  DeviceMapDoc["identifiers"] = String(hostname);
  DeviceMapDoc["manufacturer"] = "CleverCoffee";
  DeviceMapDoc["model"] = getMachineName(machine);
  DeviceMapDoc["name"] = String(hostname);
  
  DynamicJsonDocument switchConfigDoc(512);
  switchConfigDoc["name"] = displayName;
  switchConfigDoc["command_topic"] = switch_command_topic;
  switchConfigDoc["state_topic"] = switch_state_topic;
  switchConfigDoc["unique_id"] = unique_id + "-"+name;
  switchConfigDoc["payload_on"] = payload_on;
  switchConfigDoc["payload_off"] = payload_off;
  switchConfigDoc["payload_available"] = "online";
  switchConfigDoc["payload_not_available"] = "offline";
  switchConfigDoc["availability_topic"] = mqtt_topic+"/status";

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
 * @brief Generate a sensor device for Home Assistant MQTT discovery
 *
 * This function generates a sensor device configuration for Home Assistant MQTT discovery. It creates a `DiscoveryObject` containing the necessary information for Home Assistant to discover and monitor the sensor device.
 *
 * @param name The name of the sensor (used in MQTT topics)
 * @param displayName The display name of the sensor (shown in Home Assistant)
 * @param unit_of_measurement The unit of measurement for the sensor data (default: "°C")
 * @return A `DiscoveryObject` containing the sensor device configuration
 */
DiscoveryObject GenerateSensorDevice(String name, String displayName, String unit_of_measurement = "°C", String device_class = "temperature") {
  String mqtt_topic = String(mqtt_topic_prefix) + String(hostname);
  DiscoveryObject sensor_device;
  String unique_id = "clevercoffee-" + String(hostname);
  String SensorDiscoveryTopic = String(MQTT_HASSIO_DISCOVERY_PREFIX) + "/sensor/";

  String sensor_state_topic = mqtt_topic + "/"+ name;
  sensor_device.discovery_topic = SensorDiscoveryTopic + unique_id + "-"+ name +"" +"/config";
  
  DynamicJsonDocument DeviceMapDoc(1024);
  char DeviceMapBuffer[256];
  DeviceMapDoc["identifiers"] = String(hostname);
  DeviceMapDoc["manufacturer"] = "CleverCoffee";
  DeviceMapDoc["model"] = getMachineName(machine);
  DeviceMapDoc["name"] = String(hostname);
  
  DynamicJsonDocument sensorConfigDoc(512);
  sensorConfigDoc["name"] = displayName;
  sensorConfigDoc["state_topic"] = sensor_state_topic;
  sensorConfigDoc["unique_id"] = unique_id + "-"+name;
  sensorConfigDoc["unit_of_measurement"] = unit_of_measurement;
  sensorConfigDoc["device_class"] = device_class;
  sensorConfigDoc["payload_available"] = "online";
  sensorConfigDoc["payload_not_available"] = "offline";
  sensorConfigDoc["availability_topic"] = mqtt_topic+"/status";

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
DiscoveryObject GenerateNumberDevice(String name, String displayName, int min_value, int max_value, float steps_value, String unit_of_measurement = "°C", String ui_mode = "box") {
  String mqtt_topic = String(mqtt_topic_prefix) + String(hostname);
  DiscoveryObject number_device;
  String unique_id = "clevercoffee-" + String(hostname);

  String NumberDiscoveryTopic = String(MQTT_HASSIO_DISCOVERY_PREFIX) + "/number/";
  number_device.discovery_topic = NumberDiscoveryTopic + unique_id + "-"+ name +"" +"/config";

  DynamicJsonDocument DeviceMapDoc(1024);
  char DeviceMapBuffer[256];
  DeviceMapDoc["identifiers"] = String(hostname);
  DeviceMapDoc["manufacturer"] = "CleverCoffee";
  DeviceMapDoc["model"] = getMachineName(machine);
  DeviceMapDoc["name"] = String(hostname);

  DynamicJsonDocument numberConfigDoc(512);
  numberConfigDoc["name"] = displayName;
  numberConfigDoc["command_topic"] = mqtt_topic + "/"+ name + "/set";
  numberConfigDoc["state_topic"] = mqtt_topic + "/"+ name;
  numberConfigDoc["unique_id"] = unique_id + "-"+name;
  numberConfigDoc["min"] = min_value;  // Minimum value
  numberConfigDoc["max"] = max_value;  // Maximum value
  numberConfigDoc["step"] = String(steps_value,2);  // Steps value
  numberConfigDoc["unit_of_measurement"] = unit_of_measurement;
  numberConfigDoc["mode"] = ui_mode;
  numberConfigDoc["payload_available"] = "online";
  numberConfigDoc["payload_not_available"] = "offline";
  numberConfigDoc["availability_topic"] = mqtt_topic+"/status";

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
// Number Devices
  DiscoveryObject brewSetpoint = GenerateNumberDevice("brewSetpoint", "Brew setpoint", BREW_SETPOINT_MIN, BREW_SETPOINT_MAX, 0.1, "°C"); 
  DiscoveryObject steamSetPoint = GenerateNumberDevice("steamSetpoint", "Steam setpoint", STEAM_SETPOINT_MIN, STEAM_SETPOINT_MAX, 0.1, "°C"); 
  DiscoveryObject brewTempOffset = GenerateNumberDevice("brewTempOffset", "Brew Temp. Offset", BREW_TEMP_OFFSET_MIN, BREW_TEMP_OFFSET_MAX, 0.1, "°C"); 
  DiscoveryObject brewPidDelay = GenerateNumberDevice("brewPidDelay", "Brew Pid Delay", BREW_PID_DELAY_MIN, BREW_PID_DELAY_MAX, 0.1, ""); 
  DiscoveryObject startKp = GenerateNumberDevice("startKp", "Start kP", PID_KP_START_MIN, PID_KP_START_MAX, 0.1, ""); 
  DiscoveryObject startTn = GenerateNumberDevice("startTn", "Start Tn", PID_TN_START_MIN, PID_TN_START_MAX, 0.1, ""); 
  DiscoveryObject steamKp = GenerateNumberDevice("steamKp", "Start Kp", PID_KP_STEAM_MIN, PID_KP_STEAM_MAX, 0.1, ""); 
  DiscoveryObject aggKp = GenerateNumberDevice("aggKp", "aggKp", PID_KP_REGULAR_MIN, PID_KP_REGULAR_MAX, 0.1, ""); 
  DiscoveryObject aggTn = GenerateNumberDevice("aggTn", "aggTn", PID_TN_REGULAR_MIN, PID_TN_REGULAR_MAX, 0.1, ""); 
  DiscoveryObject aggTv = GenerateNumberDevice("aggTv", "aggTv", PID_TV_REGULAR_MIN, PID_TV_REGULAR_MAX, 0.1, ""); 
  DiscoveryObject aggIMax = GenerateNumberDevice("aggIMax", "aggIMax", PID_I_MAX_REGULAR_MIN, PID_I_MAX_REGULAR_MAX, 0.1, ""); 
  DiscoveryObject brewtime = GenerateNumberDevice("brewtime", "Brew time", BREW_TIME_MIN, BREW_TIME_MAX, 0.1, "s"); 
  // Sensor Devices 
  DiscoveryObject actual_temperature = GenerateSensorDevice("temperature", "Boiler Temperature"); 
  DiscoveryObject heaterPower = GenerateSensorDevice("heaterPower", "Heater Power", "%", "power_factor"); 
  // Switch Devices 
  DiscoveryObject pidOn = GenerateSwitchDevice("pidON", "Use PID"); 
  DiscoveryObject steamON = GenerateSwitchDevice("steamON", "Steam"); 
  DiscoveryObject backflushON = GenerateSwitchDevice("backflushON", "Backflush"); 
  DiscoveryObject startUsePonM = GenerateSwitchDevice("startUsePonM", "Use PonM");

  

// Define an array to store the DiscoveryObject instances
  DiscoveryObject discoveryObjects[] = {
    brewSetpoint,
    steamSetPoint,
    brewTempOffset,
    brewPidDelay,
    startKp,
    startTn,
    steamKp,
    aggKp,
    aggTn,
    aggTv,
    aggIMax,
    actual_temperature,
    heaterPower,
    brewtime,
    pidOn,
    steamON,
    backflushON,
    startUsePonM
  };

  const int numDiscoveryObjects = sizeof(discoveryObjects) / sizeof(discoveryObjects[0]);


  // Send the Objects to Hassio
  if (mqtt.connected()) {
    for (int i = 0; i < numDiscoveryObjects; i++) {
      const DiscoveryObject& discoveryObj = discoveryObjects[i];
      int publishResult = PublishLargeMessage(discoveryObj.discovery_topic.c_str(), discoveryObj.payload_json.c_str());

      if (publishResult != 0) {
        debugPrintf("[MQTT] Failed to publish discovery message. Error code: %d\n", publishResult);
        return publishResult;
      }
    }
    //debugPrintln("[MQTT] Hassio Discover sent");
    return 0;
  }
  else {
    debugPrintln("[MQTT] Failed to send Hassio Discover, MQTT Client is not connected");
    return -1;
  }
}
