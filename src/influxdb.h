/**
 * @file influxdb.h
 *
 * @brief
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


void influxDbSetup() {
    if (INFLUXDB_AUTH_TYPE == 1) {
        influxClient.setConnectionParams(INFLUXDB_URL, INFLUXDB_ORG_NAME, INFLUXDB_DB_NAME, INFLUXDB_API_TOKEN);
    }
    else if (INFLUXDB_AUTH_TYPE == 2 && (strlen(INFLUXDB_USER) > 0) && (strlen(INFLUXDB_PASSWORD) > 0)) {
        influxClient.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
    }
}


/**
 * @brief
 *
 */
void sendInflux() {
    unsigned long currentMillisInflux = millis();

    if (currentMillisInflux - previousMillisInflux >= intervalInflux) {
        previousMillisInflux = currentMillisInflux;
        influxSensor.clearFields();
        influxSensor.addField("value", temperature);
        influxSensor.addField("setPoint", setPoint);
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
        }
    }
}
