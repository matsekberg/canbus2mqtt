// ESP32 Devmodule,


#include <cstdint>

#include "config.h"


#include <WiFi.h>
//#include <ArduinoOTA.h>
#include <ESPmDNS.h>

#include <WiFiClient.h>

//#include <vector>
#include <esp_log.h>

//#include <iostream>
//#include <sstream>
//#include <string>

#include "canbus.h"

#include "/etc/mconfig/wifi.h"
#include "/etc/mconfig/mqtt.h"


#ifdef VAJFAJ
const char* ssid = ssid_ap_vajfaj;
const char* password = pass_ap_vajfaj;
const char* mqtt_server = "mqtt://10.0.1.50";
#endif

#ifdef IOT
const char* ssid = ssid_ap_iot;
const char* password = pass_ap_iot;
const char* mqtt_server = "mqtt://10.0.1.50";
#endif

#ifdef HUSBIL
const char* ssid = ssid_ap_husbil2g;
const char* password = pass_ap_husbil2g;
const char* mqtt_server = "mqtt://192.168.8.1";
#endif

int mqtt_port = 1883;

static const char* TAG = "MM";
static const char* TAG_MAP = "onm";

const char* host = "can2mqtt";


/***************************************************************
                             SETUP
****************************************************************/

void setup() {

  Serial.begin(115200);
  ESP_LOGI(TAG, "Initializing mqttmpr...");
  WiFi.disconnect(true);

  //test_testmode(TEST_MODE);

  vTaskDelay(pdMS_TO_TICKS(1000));

  // connecting to wifi

  ESP_LOGI(TAG, "Connecting to %s", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(host);
  WiFi.begin(ssid, password);
  while (WiFi.isConnected() == false) {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
  }

  if (MDNS.begin(host)) {  // this will be the mDNS name i.e. mydevice.local
    ESP_LOGI(TAG, "mDNS responder started");
    MDNS.addService("mqtt", "tcp", 1883);  // advertise as an MQTT service
  } else {
    ESP_LOGI(TAG, "Error setting up MDNS responder!");
  }

  /*
  // Setup OTA update
  ArduinoOTA.setHostname("mqttmpr");  // Optional: Set a custom device name
  ArduinoOTA.setPassword("admin");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
*/

  can_begin();

  ESP_LOGI(TAG, "Setup done");
}






/***************************************************************
                             LOOP 
****************************************************************/

void loop() {

  if (WiFi.status() == WL_CONNECTED) {
    //ArduinoOTA.handle();
  } else {
    check_and_reconnect_mqtt();
  }
}





/***************************************************************
                        WIFI AND MQTT
****************************************************************/

void reset_wifi() {
  WiFi.disconnect(true);
  WiFi.reconnect();
}

bool check_and_reconnect_mqtt() {
  if (!WiFi.isConnected()) {
    WiFi.begin();
    uint8_t count = 10;
    ESP_LOGI(TAG, "Connect to WiFi...");
    //WiFi.disconnect(true);
    //WiFi.reconnect();
    //WiFi.begin(ssid, password);
    while (count-- && !WiFi.isConnected()) delay(200);
    if (WiFi.isConnected()) {
      ESP_LOGI(TAG, "WiFi connected to %s at RSSI %d", WiFi.localIP().toString(), WiFi.RSSI());
    } else {
      return false;
    }
  }


  return /*mqttclient.connected() &&*/ WiFi.status() == WL_CONNECTED;
}
