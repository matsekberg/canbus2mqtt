#ifndef CONFIG_H
#define CONFIG_H

// what wifi to use
#define HUSBIL

// test mode or not
//#define TEST_MODE

//#define NO_OUTPUT
//#define NO_PROCESSING
#define CANBUS_TO_MQTT

// disable code execution
#define NO_WRENCH

// number of message buffers
#define MSGS_IN_BUFFER 200

// Input and output queues
#define MAX_IN_QUEUE_SIZE    MSGS_IN_BUFFER/2
#define MAX_OUT_QUEUE_SIZE   MSGS_IN_BUFFER/2


extern const char* ssid;
extern const char* password;
extern const char* mqtt_server;
extern int mqtt_port;

/*
const char* ssid = "husbil-5g";
const char* password = "husbil6969";
const char* mqtt_server = "10.0.1.50";
*/

/*
const char* ssid = "VAJFAJ";
const char* password = "CKAbp7DKFaSDCpmn";
const char* mqtt_server = "10.0.1.50";
*/


#endif /* CONFIG_H */
