/**
 * @file mqtt.h
 *
 * @brief
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

enum MQTTSettableType {
    tUInt8,
    tDouble,
};

struct mqttVars_t {
    String mqttParamName;
    MQTTSettableType type;
    int minValue;
    int maxValue;
    void *mqttVarPtr;
};

extern std::vector<mqttVars_t> mqttVars;


/**
 * @brief Check if MQTT is connected, if not reconnect abort function if offline, or brew is running
 *      MQTT is also using maxWifiReconnects!
 */
void checkMQTT() {
    if (offlineMode == 1 || brewcounter > 11) return;

    if ((millis() - lastMQTTConnectionAttempt >= wifiConnectionDelay) && (MQTTReCnctCount <= maxWifiReconnects)) {
        int statusTemp = mqtt.connected();

        if (statusTemp != 1) {
            lastMQTTConnectionAttempt = millis();  // Reconnection Timer Function
            MQTTReCnctCount++;                     // Increment reconnection Counter
            Serial.printf("Attempting MQTT reconnection: %i\n", MQTTReCnctCount);

            if (mqtt.connect(hostname, mqtt_username, mqtt_password, topic_will, 0, 0,"exit") == true) {
                mqtt.subscribe(topic_set);
                Serial.println("Subscribe to MQTT Topics");
            }   // Try to reconnect to the server; connect() is a blocking
                // function, watch the timeout!
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
    String key = String(param);
    boolean paramValid = false;
    boolean paramInRange = false;

    for (mqttVars_t m : mqttVars) {
        if (m.mqttParamName.equals(key)) {
            if (value >= m.minValue && value <= m.maxValue) {
                switch (m.type) {
                    case tDouble:
                        *(double *)m.mqttVarPtr = value;
                        paramValid = true;
                        break;
                    case tUInt8:
                        *(uint8_t *)m.mqttVarPtr = value;
                        paramValid = true;
                        break;
                    default:
                        Serial.println(String(m.type) + " is not a recognized type for this MQTT parameter.");
                }

                paramInRange = true;
            }
            else {
                Serial.println("Value out of range for MQTT parameter "+ key + ".");
                paramInRange = false;
            }

            break;
        }
    }

    if (paramValid && paramInRange) {

        if (key.equals("SteamON")) {
            steamFirstON = value;
        }

        mqtt_publish(param, number2string(value));
        writeSysParamsToStorage();
    }
    else {
        Serial.println(key + " is not a valid MQTT parameter.");
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
    Serial.println(topic_pattern);

    if ((sscanf(topic_str, topic_pattern, &configVar, &cmd) != 2) || (strcmp(cmd, "set") != 0)) {
        Serial.println(topic_str);
        return;
    }

    Serial.println(topic_str);
    Serial.println(data_str);

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
            mqtt_publish("temperature", number2string(temperature));
            mqtt_publish("brewSetPoint", number2string(brewSetPoint));
            mqtt_publish("brewTempOffset", number2string(brewTempOffset));
            mqtt_publish("steamSetPoint", number2string(steamSetPoint));
            mqtt_publish("heaterPower", number2string(pidOutput));
            mqtt_publish("currentKp", number2string(bPID.GetKp()));
            mqtt_publish("currentKi", number2string(bPID.GetKi()));
            mqtt_publish("currentKd", number2string(bPID.GetKd()));
            mqtt_publish("pidON", number2string(pidON));
            mqtt_publish("brewtime", number2string(brewtime));
            mqtt_publish("preinfusionpause", number2string(preinfusionpause));
            mqtt_publish("preinfusion", number2string(preinfusion));
            mqtt_publish("steamON", number2string(steamON));
            mqtt_publish("backflushON", number2string(backflushON));

            // Normal PID
            mqtt_publish("aggKp", number2string(aggKp));
            mqtt_publish("aggTn", number2string(aggTn));
            mqtt_publish("aggTv", number2string(aggTv));
            mqtt_publish("aggIMax", number2string(aggIMax));

            // BD PID
            mqtt_publish("aggbKp", number2string(aggbKp));
            mqtt_publish("aggbTn", number2string(aggbTn));
            mqtt_publish("aggbTv", number2string(aggbTv));

            // Start PI
            mqtt_publish("startKp", number2string(startKp));
            mqtt_publish("startTn", number2string(startTn));

             // Steam P
            mqtt_publish("steamKp", number2string(steamKp));

            //BD Parameter
        #if BREWDETECTION == 1
            mqtt_publish("brewTimer", number2string(brewtimersoftware));
            mqtt_publish("brewLimit", number2string(brewsensitivity));
        #endif

        #if BREWMODE == 2
            mqtt_publish("weightSetpoint", number2string(weightSetpoint));
        #endif
        }
    }
}
