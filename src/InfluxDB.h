/**
 * @file InfluxDB.h
 *
 * @brief InfluxDB time series database client
 *
 */

#pragma once

#include "userConfig.h"
#include <InfluxDbClient.h>


// InfluxDB Client
InfluxDBClient influxClient(INFLUXDB_URL, INFLUXDB_DB_NAME);
Point influxSensor("machineState");
const unsigned long intervalInflux = INFLUXDB_INTERVAL;
unsigned long previousMillisInflux;  // initialisation at the end of init()
boolean influxdb_healthy = true;
const int influxdb_retries = INFLUXDB_RETRIES;
int influxdb_tries = 0;

void influxDbSetup() {
    if (INFLUXDB_AUTH_TYPE == 1) {
        influxClient.setConnectionParams(INFLUXDB_URL, INFLUXDB_ORG_NAME, INFLUXDB_DB_NAME, INFLUXDB_API_TOKEN);
        influxClient.setHTTPOptions(HTTPOptions().httpReadTimeout(INFLUXDB_TIMEOUT));
    }
    else if (INFLUXDB_AUTH_TYPE == 2 && (strlen(INFLUXDB_USER) > 0) && (strlen(INFLUXDB_PASSWORD) > 0)) {
        influxClient.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
        influxClient.setHTTPOptions(HTTPOptions().httpReadTimeout(INFLUXDB_TIMEOUT));
    }
    if (INFLUXDB_INSECURE == 1) {
        influxClient.setInsecure();
        debugPrintf("InfluxDB setInsecure");
    }
}


/**
 * @brief
 *
 */
void sendInflux() {
    if (influxdb_healthy) {
        unsigned long currentMillisInflux = millis();
        if (currentMillisInflux - previousMillisInflux >= intervalInflux) {
            previousMillisInflux = currentMillisInflux;
            influxSensor.clearFields();
            influxSensor.addField("value", temperature);
            influxSensor.addField("setpoint", setpoint);
            influxSensor.addField("HeaterPower", pidOutput);
            influxSensor.addField("Kp", bPID.GetKp());
            influxSensor.addField("Ki", bPID.GetKi());
            influxSensor.addField("Kd", bPID.GetKd());
            influxSensor.addField("pidON", pidON);
            influxSensor.addField("brewtime", brewtime);
            influxSensor.addField("preinfusionpause", preinfusionpause);
            influxSensor.addField("preinfusion", preinfusion);
            influxSensor.addField("steamON", steamON);

            byte mac[6];
            WiFi.macAddress(mac);
            String macaddr0 = number2string(mac[0]);
            String macaddr1 = number2string(mac[1]);
            String macaddr2 = number2string(mac[2]);
            String macaddr3 = number2string(mac[3]);
            String macaddr4 = number2string(mac[4]);
            String macaddr5 = number2string(mac[5]);
            String completemac = macaddr0 + macaddr1 + macaddr2 + macaddr3 + macaddr4 + macaddr5;
            influxSensor.addField("mac", completemac);

            // Write point
            if (!influxClient.writePoint(influxSensor)) {
                debugPrintf("InfluxDB write failed: %s\n", influxClient.getLastErrorMessage().c_str());
                influxdb_tries++;
                debugPrintf("InfluxDB retries %i\n", influxdb_tries);
                if (influxdb_tries >= influxdb_retries) {
                    influxdb_healthy = false;
                    debugPrintln("InlfuxDB retries reached therefore we disable InfluxDB");
                }
            }
            else {
                // Set it back to 0 on successful transmission
                influxdb_tries = 0;
            }
        }
    }
}
