#ifndef QUEUEMGR_H
#define QUEUEMGR_H

#include <string>
#include <cstring>
#include <esp_log.h>

#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
#include "freertos/queue.h"

const TickType_t SLEEP_1MS = pdMS_TO_TICKS(1);

struct QueuedMessage {
  uint32_t can_id;
  uint8_t can_datalen;
  uint8_t can_data[8];

  char topic[64];
  char payload[256];

  // Default constructor
  QueuedMessage() {
    ESP_LOGV("MAP", "Empty constructor");
  }

/*
  // Constructor for MQTT payload
  QueuedMessage(const char* itopic, const char* ipayload)
    : can_msg(false) {
    if (strlen(itopic) > sizeof(topic))
      ESP_LOGE("MAP", "Topic length overflow: %d", strlen(itopic));
    if (strlen(ipayload) > sizeof(payload))
      ESP_LOGE("MAP", "Payload length overflow: %d", strlen(ipayload));
    strncpy(topic, itopic, strlen(itopic) + 1);
    topic[sizeof(topic) - 1] = '\0';  // Ensure null termination

    strncpy(payload, ipayload, strlen(ipayload) + 1);
    payload[sizeof(payload) - 1] = '\0';  // Ensure null termination
    //ESP_LOGI("XXX", "'%s' from '%s'", payload, ipayload);
    //ESP_LOGV("XXX", "Created MQTT message '%s'", topic);
  }
*/
  // Constructor for CAN payload
  QueuedMessage(const char* itopic, const uint8_t ipayload[8], const uint8_t length)
    : can_datalen(length) {
    if (strlen(itopic) > sizeof(topic))
      ESP_LOGE("MAP", "Topic length overflow: %d", strlen(itopic));
    strncpy(topic, itopic, strlen(itopic) + 1);  // Copy string with boundary
    topic[strlen(itopic)] = '\0';                // Ensure null termination
    for (uint8_t i = 0; i < length && i < 8; i++) {
      can_data[i] = ipayload[i];
    }
    //ESP_LOGI("XXX", "Created CAN message %d bytes '%s'", length, topic);
  }
};



extern QueuedMessage message_array[];
extern bool mrunning;
extern QueueHandle_t outputQueue;

void queue_begin(bool mqtt, bool canbus);

// queue an incoming message (MQTT or CanBus) for mapping/processing
void queue_mqtt_output(uint16_t qindex);

void print_msg(QueuedMessage* msg);
void print_msg(char* text, QueuedMessage* msg);

uint16_t get_free_msg_buffer_index();
QueuedMessage * get_msg_from_buffer(uint16_t qindex);
uint16_t put_msg_to_buffer(QueuedMessage * message);
void free_msg_from_buffer(uint16_t qindex);
void purge_buffer();

#endif