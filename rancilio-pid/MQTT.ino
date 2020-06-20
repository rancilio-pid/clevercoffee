/********************************************************
  MQTT
*****************************************************/
bool almostEqual(float a, float b) {
    return fabs(a - b) <= FLT_EPSILON;
}
char* bool2string(bool in) {
  if (in) {
    return "1";
  } else {
    return "0";
  }
}
char number2string_double[22];
char* number2string(double in) {
  snprintf(number2string_double, sizeof(number2string_double), "%0.2f", in);
  return number2string_double;
}
char number2string_float[22];
char* number2string(float in) {
  snprintf(number2string_float, sizeof(number2string_float), "%0.2f", in);
  return number2string_float;
}
char number2string_int[22];
char* number2string(int in) {
  snprintf(number2string_int, sizeof(number2string_int), "%d", in);
  return number2string_int;
}
char number2string_uint[22];
char* number2string(unsigned int in) {
  snprintf(number2string_uint, sizeof(number2string_uint), "%u", in);
  return number2string_uint;
}

char* mqtt_build_topic(char* reading) {
  char* topic = (char *) malloc(sizeof(char) * 256);
  snprintf(topic, sizeof(topic), "%s%s/%s", mqtt_topic_prefix, hostname, reading);
  return topic;
}

/* ------------------------------ */
#if (MQTT_ENABLE == 0)  //MQTT Disabled
bool mqtt_publish(char* reading, char* payload) { return true; }
bool mqtt_reconnect(bool force_connect = false) { return true; }
bool mqtt_working() { return false; }

/* ------------------------------ */
#elif (MQTT_ENABLE == 1)  //MQTT Client
bool mqtt_working() {
  return ((MQTT_ENABLE >0) && (wifi_working()) && (mqtt_client.connected()));
}

bool mqtt_publish(char* reading, char* payload) {
  if (!MQTT_ENABLE || force_offline || mqtt_disabled_temporary) return true;
  if (!mqtt_working()) { return false; }
  char topic[MQTT_MAX_PUBLISH_SIZE];
  snprintf(topic, MQTT_MAX_PUBLISH_SIZE, "%s%s/%s", mqtt_topic_prefix, hostname, reading);
  
  if (strlen(topic) + strlen(payload) >= MQTT_MAX_PUBLISH_SIZE) {
    ERROR_print("mqtt_publish() wants to send too much data (len=%u)\n", strlen(topic) + strlen(payload));
    return false;
  } else {
    unsigned long currentMillis = millis();
    if (currentMillis > mqtt_dontPublishUntilTime) {
      bool ret = mqtt_client.publish(topic, payload, mqtt_flag_retained);
      if (ret == false) {
        mqtt_dontPublishUntilTime = millis() + mqtt_dontPublishBackoffTime;
        ERROR_print("Error on publish. Wont publish the next %ul ms\n", mqtt_dontPublishBackoffTime);
        mqtt_client.disconnect();
      }
      return ret;
    } else { //TODO test this code block later (faking an error)
      ERROR_print("Data not published (still for the next %ul ms)\n", mqtt_dontPublishUntilTime - currentMillis);
      return false;
    }
  }
}

bool mqtt_reconnect(bool force_connect = false) {
  if (!MQTT_ENABLE || force_offline || mqtt_disabled_temporary || mqtt_working() || in_sensitive_phase() ) return true;
  espClient.setTimeout(2000); // set timeout for mqtt connect()/write() to 2 seconds (default 5 seconds).
  unsigned long now = millis();
  if ( force_connect || ((now > mqtt_lastReconnectAttemptTime + (mqtt_reconnect_incremental_backoff * (mqtt_reconnectAttempts))) && now > all_services_lastReconnectAttemptTime + all_services_min_reconnect_interval)) {
    mqtt_lastReconnectAttemptTime = now;
    all_services_lastReconnectAttemptTime = now;
    DEBUG_print("Connecting to mqtt ...\n");
    if (mqtt_client.connect(hostname, mqtt_username, mqtt_password, topic_will, 0, 0, "unexpected exit") == true) {
      DEBUG_print("Connected to mqtt server\n");
      mqtt_publish("events", "Connected to mqtt server");
      mqtt_client.subscribe(topic_set);
      if (!mqtt_client.subscribe(topic_set)) { ERROR_print("Cannot subscribe to topic\n"); }
      mqtt_lastReconnectAttemptTime = 0;
      mqtt_reconnectAttempts = 0;
    } else {
      DEBUG_print("Cannot connect to mqtt server (consecutive failures=#%u)\n", mqtt_reconnectAttempts);
      if (mqtt_reconnectAttempts < mqtt_max_incremental_backoff) {
        mqtt_reconnectAttempts++;
      }
    }
  }
  return mqtt_client.connected();
}

void mqtt_callback(char* topic, byte* data, unsigned int length) {
  //DEBUG_print("Message arrived [%s]: %s\n", topic, (const char *)data);
  char topic_str[255];
  os_memcpy(topic_str, topic, sizeof(topic_str));
  topic_str[255] = '\0';
  char data_str[length+1];
  os_memcpy(data_str, data, length);
  data_str[length] = '\0';
  //DEBUG_print("MQTT: %s = %s\n", topic_str, data_str);
  mqtt_parse(topic_str, data_str);
}

/* ------------------------------ */
#elif (MQTT_ENABLE == 2)
bool mqtt_working() {
  return ((MQTT_ENABLE >0) && (wifi_working()));
}

bool mqtt_publish(char* reading, char* payload) {
  if (!MQTT_ENABLE || force_offline || mqtt_disabled_temporary) return true;
  char topic[MQTT_MAX_PUBLISH_SIZE];
  snprintf(topic, MQTT_MAX_PUBLISH_SIZE, "%s%s/%s", mqtt_topic_prefix, hostname, reading);
  if (!mqtt_working()) { return false; }
  if (strlen(topic) + strlen(payload) >= MQTT_MAX_PUBLISH_SIZE) {
    ERROR_print("mqtt_publish() wants to send too much data (len=%u)\n", strlen(topic) + strlen(payload));
    return false;
  } else {
    unsigned long currentMillis = millis();
    if (currentMillis > mqtt_dontPublishUntilTime) {
      bool ret = MQTT_local_publish((unsigned char*)&topic, (unsigned char*)payload, strlen(payload), 1, 1);
      if (ret == false) {
        mqtt_dontPublishUntilTime = millis() + mqtt_dontPublishBackoffTime;
         ERROR_print("Error on publish. Wont publish the next %ul ms\n", mqtt_dontPublishBackoffTime);
        //mqtt_client.disconnect();
        //MQTT_server_cleanupClientCons();
      }
      return ret;
    } else { //TODO test this code block later (faking an error)
      ERROR_print("Data not published (still for the next %ul ms)\n", mqtt_dontPublishUntilTime - currentMillis);
      return false;
    }
  }
}

bool mqtt_reconnect(bool force_connect = false) { return true; }

void mqtt_callback(uint32_t *client, const char* topic, uint32_t topic_len, const char *data, uint32_t length) {
  char topic_str[topic_len+1];
  os_memcpy(topic_str, topic, topic_len);
  topic_str[topic_len] = '\0';
  char data_str[length+1];
  os_memcpy(data_str, data, length);
  data_str[length] = '\0';
  //DEBUG_print("MQTT: %s = %s\n", topic_str, data_str);
  mqtt_parse(topic_str, data_str);
}

#endif

void mqtt_parse(char* topic_str, char* data_str) {
  char topic_pattern[255];
  char configVar[120];
  char cmd[64];
  double data_double;
  int data_int;

  //DEBUG_print("mqtt_parse(%s, %s)\n", topic_str, data_str);
  snprintf(topic_pattern, sizeof(topic_pattern), "%s%s/%%[^\\/]/%%[^\\/]", mqtt_topic_prefix, hostname);
  //DEBUG_print("topic_pattern=%s\n",topic_pattern);
  if ( (sscanf( topic_str, topic_pattern , &configVar, &cmd) != 2) || (strcmp(cmd, "set") != 0) ) {
    //DEBUG_print("Ignoring topic (%s)\n", topic_str);
    return;
  }
  if (strcmp(configVar, "brewtime") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (data_double != brewtime) {
      DEBUG_print("setting brewtime=%s\n", data_str);
      brewtime = data_double;
      mqtt_publish("brewtime", data_str);
      Blynk.virtualWrite(V8, String(brewtime, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  if (strcmp(configVar, "starttemp") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (data_double != starttemp) {
      DEBUG_print("setting starttemp=%s\n", data_str);
      starttemp = data_double;
      mqtt_publish("starttemp", data_str);
      Blynk.virtualWrite(V12, String(starttemp, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  if (strcmp(configVar, "setPoint") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (data_double != setPoint) {
      DEBUG_print("setting setPoint=%s\n", data_str);
      setPoint = data_double;
      mqtt_publish("setPoint", data_str);
      Blynk.virtualWrite(V7, String(setPoint, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  if (strcmp(configVar, "preinfusion") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (data_double != preinfusion) {
      DEBUG_print("setting preinfusion=%s\n", data_str);
      preinfusion = data_double;
      mqtt_publish("preinfusion", data_str);
      Blynk.virtualWrite(V9, String(preinfusion, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  if (strcmp(configVar, "preinfusionpause") == 0) {
    sscanf(data_str, "%lf", &preinfusionpause);
    if (data_double != preinfusionpause) {
      DEBUG_print("setting preinfusionpause=%s\n", data_str);
      preinfusionpause = data_double;
      mqtt_publish("preinfusionpause", data_str);
      Blynk.virtualWrite(V10, String(preinfusionpause, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  if (strcmp(configVar, "pidON") == 0) {
    sscanf(data_str, "%d", &data_int);
    if (data_int != pidON) {
      DEBUG_print("setting pidON=%s\n", data_str);
      pidON = data_int;
      mqtt_publish("pidON", data_str);
      Blynk.virtualWrite(V13, String(pidON, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  if (strcmp(configVar, "brewDetectionSensitivity") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (data_double != brewDetectionSensitivity) {
      DEBUG_print("setting brewDetectionSensitivity=%s\n", data_str);
      brewDetectionSensitivity = data_double;
      mqtt_publish("brewDetectionSensitivity", data_str);
      Blynk.virtualWrite(V34, String(brewDetectionSensitivity, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  if (strcmp(configVar, "brewDetectionPower") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (data_double != brewDetectionPower) {
      DEBUG_print("setting brewDetectionPower=%s\n", data_str);
      brewDetectionPower = data_double;
      mqtt_publish("brewDetectionPower", data_str);
      Blynk.virtualWrite(V36, String(brewDetectionPower, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  
  if (strcmp(configVar, "steadyPower") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (data_double != steadyPower) {
      DEBUG_print("setting steadyPower=%s\n", data_str);
      steadyPower = data_double;
      mqtt_publish("steadyPower", data_str);
      Blynk.virtualWrite(V41, String(steadyPower, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  if (strcmp(configVar, "steadyPowerOffset") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (data_double != steadyPowerOffset) {
      DEBUG_print("setting steadyPowerOffset=%s\n", data_str);
      steadyPowerOffset = data_double;
      mqtt_publish("steadyPowerOffset", data_str);
      Blynk.virtualWrite(V42, String(steadyPowerOffset, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  if (strcmp(configVar, "steadyPowerOffsetTime") == 0) {
    sscanf(data_str, "%d", &data_int);
    if (data_double != steadyPowerOffsetTime) {
      DEBUG_print("setting steadyPowerOffsetTime=%s\n", data_str);
      steadyPowerOffsetTime = data_double;
      mqtt_publish("steadyPowerOffsetTime", data_str);
      Blynk.virtualWrite(V43, String(steadyPowerOffsetTime, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  
  if (strcmp(configVar, "aggKp") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (data_double != aggKp) {
      DEBUG_print("setting aggKp=%s\n", data_str);
      aggKp = data_double;
      mqtt_publish("aggKp", data_str);
      Blynk.virtualWrite(V4, String(aggKp, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  if (strcmp(configVar, "aggTn") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (data_double != aggTn) {
      DEBUG_print("setting aggTn=%s\n", data_str);
      aggTn = data_double;
      mqtt_publish("aggTn", data_str);
      Blynk.virtualWrite(V5, String(aggTn, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  if (strcmp(configVar, "aggTv") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (data_double != aggTv) {
      DEBUG_print("setting aggTv=%s\n", data_str);
      aggTv = data_double;
      mqtt_publish("aggTv", data_str);
      Blynk.virtualWrite(V6, String(aggTv, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  if (strcmp(configVar, "aggoKp") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (data_double != aggoKp) {
      DEBUG_print("setting aggoKp=%s\n", data_str);
      aggoKp = data_double;
      mqtt_publish("aggoKp", data_str);
      Blynk.virtualWrite(V30, String(aggoKp, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  if (strcmp(configVar, "aggoTn") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (data_double != aggoTn) {
      DEBUG_print("setting aggoTn=%s\n", data_str);
      aggoTn = data_double;
      mqtt_publish("aggoTnTn", data_str);
      Blynk.virtualWrite(V31, String(aggoTn, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
  if (strcmp(configVar, "aggoTv") == 0) {
    sscanf(data_str, "%lf", &data_double);
    if (data_double != aggoTv) {
      DEBUG_print("setting aggoTv=%s\n", data_str);
      aggoTv = data_double;
      mqtt_publish("aggoTv", data_str);
      Blynk.virtualWrite(V32, String(aggoTv, 1));
      force_eeprom_sync = millis();
    }
    return;
  }
}


void mqtt_publish_settings() {
  mqtt_publish("brewtime", number2string(brewtime));
  mqtt_publish("starttemp", number2string(starttemp));
  mqtt_publish("setPoint", number2string(setPoint));
  mqtt_publish("preinfusion", number2string(preinfusion));
  mqtt_publish("preinfusionpause", number2string(preinfusionpause));
  mqtt_publish("pidON", number2string(pidON));
  mqtt_publish("brewDetectionSensitivity", number2string(brewDetectionSensitivity));
  mqtt_publish("brewDetectionPower", number2string(brewDetectionPower));
  mqtt_publish("steadyPower", number2string(steadyPower));
  mqtt_publish("steadyPowerOffset", number2string(steadyPowerOffset));
  mqtt_publish("steadyPowerOffsetTime", number2string(steadyPowerOffsetTime));
  mqtt_publish("aggKp", number2string(aggKp));
  mqtt_publish("aggTn", number2string(aggTn));
  mqtt_publish("aggTv", number2string(aggTv));
  mqtt_publish("aggoKp", number2string(aggoKp));
  mqtt_publish("aggoTn", number2string(aggoTn));
  mqtt_publish("aggoTv", number2string(aggoTv));
}
