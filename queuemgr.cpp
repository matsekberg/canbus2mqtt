#include "config.h"

#include "queuemgr.h"

//#include "mqtt.h"
//#include "canbus.h"
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "freertos/queue.h"


/*
<in-topic>/<in-canid> | <selecctor> | <expression> | <out-topic>/<out-canid> | <formatter> | <comment>

<in-topic> - standard MQTT topic selector; hasp/panel/#
<in-canid> - id and first byte selector; @321.10
<selector> - read the payload, evaluate and push on stasck
             - %s payload as a string
             - %d payload as an integer
             - %f payload as a float
             - {attr-name} a named JSON attribute from a JSON payload
<expression>
<out-topic>
<out-canid>
<formatter> - %-expression

*/

static const char* TAG = "QUE";

// buffer with QueuedMessage
QueuedMessage message_array[MSGS_IN_BUFFER];
bool message_array_free[MSGS_IN_BUFFER];

// Queues
#define QUEUE_ITEM_SIZE sizeof(uint16_t)
QueueHandle_t outputQueue;


bool mrunning = false;

static const char* TAG_MAP = "MAP";



/***********************************************************
                              TASKS
************************************************************/



void queue_begin() {

  // empty buffer
  purge_buffer();

  outputQueue = xQueueCreate(MAX_OUT_QUEUE_SIZE, QUEUE_ITEM_SIZE);
  if (outputQueue == NULL) {
    ESP_LOGE(TAG, "Failed to create outputQueue");
    // handle error
  }

  mrunning = true;
  ESP_LOGI(TAG, "Mapper running");
}


/***********************************************************
                        HANDLE QUEUES
************************************************************/

uint32_t queueQueueFull = 0;

//
// queue an outgoing message
//
void queue_mqtt_output(uint16_t qindex) {
  if (mrunning) {
    if (uxQueueSpacesAvailable(outputQueue) > 0) {
      if (xQueueSend(outputQueue, &qindex, SLEEP_1MS) == pdTRUE) {
        ESP_LOGD(TAG, "Queued buffer %d", qindex);
      } else {
        ESP_LOGW(TAG, "Failed to queue processed message in the output queue. Queue is full.");
        //delete out_message;
      }
    } else {
      ESP_LOGW(TAG, "Output queue full (%d). Dropping message.", ++queueQueueFull);
    }
  }
}


void print_msg(char* text, QueuedMessage* msg) {
  //esp_log_level_t level = esp_log_level_get(TAG);
  //if (level == ESP_LOG_DEBUG) {
  char buff[128];
  sprintf(buff, "%s - CAN message: %s ", text, msg->topic);
  for (uint8_t i = 0; i < msg->can_datalen; i++) {
    sprintf(buff, "%s:%02X", buff, msg->payload[i]);
  }
  ESP_LOGD(TAG, "%s", buff);
  //}
}

void print_msg(QueuedMessage* msg) {
  print_msg("", msg);
}


//
// Find first free buffer index, 1..MSGS_IN_BUFFER. 0 = no space
//
uint16_t get_free_msg_buffer_index() {
  for (uint16_t i = 0; i < MSGS_IN_BUFFER; i++) {
    if (message_array_free[i]) return i + 1;
  }
  ESP_LOGE(TAG_MAP, "No free buffers");
  return 0;
}

//
// Get a pointer to message by index 1..MSGS_IN_BUFFER
//
QueuedMessage* get_msg_from_buffer(uint16_t qindex) {
  if (qindex > 0 && qindex <= MSGS_IN_BUFFER) {
    if (message_array_free[qindex - 1] == true) {
      ESP_LOGE(TAG_MAP, "No message in buffer slot %d", qindex);
      return 0;
    } else {
      ESP_LOGV(TAG_MAP, "Got message with topic '%s' from buffer %d", message_array[qindex - 1].topic, qindex);
      return &message_array[qindex - 1];
    }
  } else {
    ESP_LOGE(TAG, "Buffer index out of bounds: %d", qindex);
  }
}

//
// Put a message in the buffer and return index
//
uint16_t put_msg_to_buffer(QueuedMessage* message) {
  uint16_t qindex = get_free_msg_buffer_index();
  if (qindex) {
    //memcpy((void*)(&message_array[qindex - 1]), (const void*)message, sizeof(QueuedMessage));
    message_array[qindex - 1] = *message;
    message_array_free[qindex - 1] = false;
    ESP_LOGV(TAG_MAP, "Put message with topic '%s' to buffer %d", message->topic, qindex);
    return qindex;
  } else {
    ESP_LOGW(TAG_MAP, "No free message buffer slots");
    return 0;
  }
}

//
// Mark a buffer entry as free to use
//
void free_msg_from_buffer(uint16_t qindex) {
  if (qindex > 0 && qindex <= MSGS_IN_BUFFER) {
    message_array_free[qindex - 1] = true;
  }
}

void purge_buffer() {
  for (uint16_t i = 1; i <= MSGS_IN_BUFFER; i++) {
    free_msg_from_buffer(i);
  }
}