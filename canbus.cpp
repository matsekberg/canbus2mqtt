#include "config.h"

//#include "hal/can_types.h"
#include "canbus.h"
#include "queuemgr.h"

#include <string>

#include "config.h"

#include <esp_log.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <mqtt_client.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/can.h"
#include "driver/twai.h"

static const char* TAG = "CAN";

#define CAN_RX_PIN 19  // Set the CAN RX pin
#define CAN_TX_PIN 23  // Set the CAN TX pin

bool crunning = false;

void can_do(QueuedMessage* message);


static esp_mqtt_client_handle_t mqtt_client;

// prototypes
void subscribe(esp_mqtt_client_handle_t client);

esp_mqtt_client_handle_t* mqtt_get_client() {
  return &mqtt_client;
}

/***************************************************************
                            CAN BUS
****************************************************************/


uint32_t msgcount = 0;

//#define CANBUS_LOG

//
// Task to check CAN bus and queue messages for MQTT
//
void listenToCanTask(void* parameter) {
  ESP_LOGI(TAG, "listenToCanTask started");
  // Receive CAN message
  can_message_t rx_msg;
  char topic[8];

  while (1) {
    esp_err_t err = twai_receive(&rx_msg, pdMS_TO_TICKS(1));
    if (err == ESP_OK) {

      /*
      char logmsg[100];
      sprintf(logmsg, "%ld,%03X,%d", millis(), rx_msg.identifier, rx_msg.data_length_code);
      for (uint8_t i = 0; i < rx_msg.data_length_code; i++) {
        uint8_t beg = strlen(logmsg);
        sprintf(logmsg + beg, ",%02X", rx_msg.data[i]);
      }
      ESP_LOGI("CANLOG", "%s", logmsg);
*/

      uint8_t instance = 0;
      if (rx_msg.data_length_code > 0) {
        // the first byte is always an instance no
        instance = rx_msg.data[0];
      }

      // create a "topic" for the CAN-id
      sprintf(topic, "@%03X.%02X", rx_msg.identifier, instance);

      // create a message object
      QueuedMessage message = QueuedMessage(topic, rx_msg.data, rx_msg.data_length_code);

      // add MQTT message to queue object
      can_do(&message);

      // find the first slot in buffer and get the index of it
      uint16_t qindex = put_msg_to_buffer(&message);
      if (qindex) {
        // put the buffer index in a queue if there was space
        queue_mqtt_output(qindex);
        ESP_LOGV(TAG, "Stack %d", uxTaskGetStackHighWaterMark(NULL));
      } else {
        ESP_LOGW(TAG, "Dropped CAN-message, buffer full");
      }
      // Delay to reduce CPU usage
      vTaskDelay(pdMS_TO_TICKS(5));

    } else if (err == ESP_ERR_TIMEOUT) {
    } else {
      ESP_LOGE(TAG, "Failed to receive message: %d", err);
    }
  }
}




//
// Initialize CAN bus at a 500 kbps bitrate
//
void can_begin() {

  // CAN bus GPIOs
  const gpio_num_t TWAI_TX_GPIO = (const gpio_num_t)CAN_TX_PIN;
  const gpio_num_t TWAI_RX_GPIO = (const gpio_num_t)CAN_RX_PIN;

  // Setup CAN configuration
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TWAI_TX_GPIO, TWAI_RX_GPIO, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // Install TWAI driver
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    ESP_LOGD(TAG, "TWAI Driver installed");
  } else {
    ESP_LOGE(TAG, "Failed to install TWAI driver");
    return;
  }

  // Start TWAI driver
  if (twai_start() == ESP_OK) {
    ESP_LOGD(TAG, "TWAI Driver started");
  } else {
    ESP_LOGE(TAG, "Failed to start TWAI driver");
    return;
  }

  // Start task to handle CAN messages
  xTaskCreatePinnedToCore(
    listenToCanTask,   /* Function to implement the task */
    "listenToCanTask", /* Name of the task */
    6000,              /* Stack size in words */
    NULL,              /* Task input parameter */
    19,                /* Priority of the task */
    NULL,              /* Task handle. */
    1);                /* Core where the task should run */




  ESP_LOGI(TAG, "CAN bus initialized successfully!");
  crunning = true;
}



/***********************************************************
                HANDLE INCOMING CAN MESSAGE
************************************************************/

#define LOG_ELAPSED(XX) \
  ESP_LOGI(TAG, "%s -> %u mS", XX, millis() - start); \
  start = millis();
//#define LOG_ELAPSED(XX)

void can_do(QueuedMessage* message) {
  //print_msg("do_mapper)", message);
  if (!mrunning) {
    ESP_LOGD(TAG, "do nothing");
    return;
  }

  // publish CAN message on MQTT
  //print_msg("can2mqtt", message);

  char top[24];
  sprintf(top, "can2mqtt/%s", &message->topic[1]);
  //print_msg(top, message);

  char pl[128];
  pl[0] = 0;
  for (uint8_t i = 0; i < message->can_datalen; i++) {
    sprintf(pl, "%s%02X", pl, message->payload[i]);
  }
  //print_msg(pl, message);

  ESP_LOGI(TAG, "CAN to MQTT '%s' = '%s'", outmsg.topic, outmsg.payload);
  uint16_t qindex = put_msg_to_buffer(message);
  if (qindex) {
    queue_mqtt_output(qindex);
  }
  //print_msg("can2mqtt 222", message);

}




/***********************************************************
                HANDLE INCOMING MQTT MESSAGE
************************************************************/

void on_message(std::string input_topic, std::string payload) {

  //digitalWrite(LED_BUILTIN, HIGH);

  if (input_topic == "can2mqtt/command") {
    if (payload == "restart") {
      ESP_LOGI(TAG_MQT, "Rebooting ordered by MQTT");
      vTaskDelay(pdMS_TO_TICKS(500));
      ESP.restart();
    } else if (payload == "mapping1") {
    }
    return;
  }
  /*
  QueuedMessage qmsg = QueuedMessage(input_topic.c_str(), payload.c_str());
  uint16_t qindex = put_msg_to_buffer(&qmsg);
  if (qindex) {
    mapper_queue_input(qindex);
  }
*/
}


//
// Task: Publish messages from the output queue
//
void publishMessagesTask(void* parameter) {
  ESP_LOGI(TAG, "publishMessagesTask started");

  // Check if the parameter is NULL before dereferencing
  while (!parameter) {
    ESP_LOGE(TAG, "No mqtt_client ready yet, waiting...");
    vTaskDelay(pdMS_TO_TICKS(2000));  // Adjust delay as per your requirements
  }

  esp_mqtt_client_handle_t mqtt_client = *(esp_mqtt_client_handle_t*)parameter;

  uint16_t qindex;
  QueuedMessage* in_message;
  while (true) {
    if (mrunning) {
      if (xQueueReceive(outputQueue, &qindex, SLEEP_1MS) == pdTRUE) {
        in_message = get_msg_from_buffer(qindex);
        if (in_message) {
          esp_mqtt_client_publish(mqtt_client, in_message->topic, in_message->payload, 0, 0, 0);
          free_msg_from_buffer(qindex);
          ESP_LOGD(TAG, "MQTT '%s' = '%s'", in_message->topic, in_message->payload);
        }
      }
    }
  }
  vTaskDelay(SLEEP_1MS);  // Adjust delay as per your requirements
}




//
// MQTT event handler
//
static esp_err_t mqttEventHandler(esp_mqtt_event_handle_t event) {
  esp_mqtt_client_handle_t client = event->client;
  int msg_id;

  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      ESP_LOGI(TAG_MQT, "Connected to MQTT broker");
      vTaskDelay(pdMS_TO_TICKS(500));
      subscribe(mqtt_client);
      break;
    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGE(TAG_MQT, "Disconnected from MQTT broker");
      esp_mqtt_client_start(client);
      break;
    case MQTT_EVENT_SUBSCRIBED:
      ESP_LOGD(TAG_MQT, "MQTT_EVENT_SUBSCRIBED");
      break;
    case MQTT_EVENT_DATA:
      on_message(std::string(event->topic, event->topic_len), std::string(event->data, event->data_len));
      break;
    default:
      break;
  }
  return ESP_OK;
}


//
// Connect to MQTT broker
//
void connectToMQTT(std::string mqtt_server, int mqtt_port) {
  esp_mqtt_client_config_t mqtt_cfg = {
    .event_handle = mqttEventHandler,
    .uri = mqtt_server.c_str(),
    .port = mqtt_port
  };

  // Initialize the global mqtt_client
  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_start(mqtt_client);
  vTaskDelay(pdMS_TO_TICKS(500));
}




//
// setup MQTT
//
void mqtt_begin() {
  // Start task for publishing messages
  xTaskCreatePinnedToCore(
    publishMessagesTask,
    "PublishMessagesTask",
    8000,
    mqtt_get_client(),
    17,
    NULL,
    1);

  vTaskDelay(pdMS_TO_TICKS(1000));
  connectToMQTT(mqtt_server, mqtt_port);
}


//
// subscribe to configured topics
//
void subscribe(esp_mqtt_client_handle_t client) {

  int msg_id = esp_mqtt_client_subscribe(client, "can2mqtt/command", 0);
}

