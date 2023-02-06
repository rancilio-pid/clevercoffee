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
unsigned int MQTTReCnctFlag;
unsigned int MQTTReCnctCount = 0;


extern std::map<const char*, std::function<editable_t*()>, cmp_str> mqttVars;
extern std::map<const char*, std::function<double()>, cmp_str> mqttSensors;


/**
 * @brief Check if MQTT is connected, if not reconnect. Abort function if offline or brew is running
 *      MQTT is also using maxWifiReconnects!
 */
void checkMQTT() {
    if (offlineMode == 1 || brewcounter > kBrewIdle) return;

    if ((millis() - lastMQTTConnectionAttempt >= wifiConnectionDelay) && (MQTTReCnctCount <= maxWifiReconnects)) {
        int statusTemp = mqtt.connected();

        if (statusTemp != 1) {
            lastMQTTConnectionAttempt = millis();  // Reconnection Timer Function
            MQTTReCnctCount++;                     // Increment reconnection Counter
            debugPrintf("Attempting MQTT reconnection: %i\n", MQTTReCnctCount);

            if (mqtt.connect(hostname, mqtt_username, mqtt_password, topic_will, 0, true, "offline") == true) {
                mqtt.subscribe(topic_set);
                debugPrintln("Subscribe to MQTT Topics");
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
bool mqtt_publish(const char *reading, char *payload) {
    char topic[120];
    snprintf(topic, 120, "%s%s/%s", mqtt_topic_prefix, hostname, reading);
    return mqtt.publish(topic, payload, true);
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
                    mqtt_publish(param, number2string(value));
                    break;
                case kUInt8:
                    *(uint8_t *)var->ptr = value;
                    if (strcasecmp(param, "steamON") == 0) {
                        steamFirstON = value;
                    }
                    mqtt_publish(param, number2string(value));
                    writeSysParamsToStorage();
                    break;
                default:
                    debugPrintf("%s is not a recognized type for this MQTT parameter.\n", var->type);
            }
        }
        else
        {
            debugPrintf("Value out of range for MQTT parameter %s\n", param);
        }
    }
    catch(const std::out_of_range& e)
    {
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
    debugPrintln(topic_pattern);

    if ((sscanf(topic_str, topic_pattern, &configVar, &cmd) != 2) || (strcmp(cmd, "set") != 0)) {
        debugPrintln(topic_str);
        return;
    }

    debugPrintln(topic_str);
    debugPrintln(data_str);

    sscanf(data_str, "%lf", &data_double);

    assignMQTTParam(configVar, data_double);
}


/**
 * @brief Send all current system parameter values to MQTT
 *
 * @return TODO 0 = success, < 0 = failure
 */
void writeSysParamsToMQTT(void) {
    unsigned long currentMillisMQTT = millis();
    if ((currentMillisMQTT - previousMillisMQTT >= intervalMQTT) && MQTT == 1) {
        previousMillisMQTT = currentMillisMQTT;

        if (mqtt.connected() == 1) {
            // status topic (will sets it to offline)
            mqtt_publish("status", (char *)"online");

            for (const auto& pair : mqttVars) {
                editable_t *e = pair.second();

                switch (e->type) {
                    case kDouble:
                        mqtt_publish(pair.first, number2string(*(double *) e->ptr));
                        break;
                    case kDoubletime:
                        mqtt_publish(pair.first, number2string(*(double *) e->ptr));
                        break;
                    case kInteger:
                        mqtt_publish(pair.first, number2string(*(int *) e->ptr));
                        break;
                    case kUInt8:
                        mqtt_publish(pair.first, number2string(*(uint8_t *) e->ptr));
                        break;
                    case kCString:
                        mqtt_publish(pair.first, number2string(*(char *) e->ptr));
                        break;
                }
            }
            
            for (const auto& pair : mqttSensors) {
                mqtt_publish(pair.first, number2string(pair.second()));
            }
        }
    }
}
