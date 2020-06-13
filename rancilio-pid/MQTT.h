#ifndef _mqtt_H
#define _mqtt_H

bool mqtt_reconnect(bool);
bool mqtt_publish(char*, char*);
bool mqtt_working();
void mqtt_publish_settings();

#endif
