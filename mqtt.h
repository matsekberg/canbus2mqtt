#ifndef MQTT_H
#define MQTT_H

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <mqtt_client.h>
//#include <freertos/queue.h>
#include "queuemgr.h"


void mqtt_begin();

bool mqtt_publish(QueuedMessage* message);
esp_mqtt_client_handle_t* mqtt_get_client();

#endif