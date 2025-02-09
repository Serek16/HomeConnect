#include <WiFi.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include "MyCredentials.h"

const int PUBLISH_INTERVAL = 5*60*1000; // 5 minutes
const char* WAKE_ON_LAN_TOPIC   = "wake-on-lan";
const char* WAKE_ON_LAN_MESSAGE = "on";

WiFiClient network;
MQTTClient mqtt = MQTTClient(256);

WiFiUDP UDP;
WakeOnLan WOL(UDP);

unsigned long lastPublishTime = 0;

void setup() {
  Serial.begin(115200);
  printf("\n");

  WiFi.begin(SSID, PASS);
  printf("Connecting to WiFi \"%s\"", SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    printf(".");
  }
  printf("\nSuccessfully connected\n");

  WOL.setRepeat(3, 100); // Repeat the packet three times with 100ms delay between
  WOL.setBroadcastAddress("192.168.1.255");

  connectToMQTT();
}

void loop() {
  mqtt.loop();

  if (millis() - lastPublishTime > PUBLISH_INTERVAL) {
    sendToMQTT();
    lastPublishTime = millis();
  }
}

void connectToMQTT() {
  // Connect to the MQTT broker
  mqtt.begin(MQTT_BROKER_ADRRESS, MQTT_PORT, network);

  // Create a handler for incoming messages
  mqtt.onMessage(messageHandler);

  Serial.print("ESP32 - Connecting to MQTT broker");

  while (!mqtt.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  if (!mqtt.connected()) {
    Serial.println("ESP32 - MQTT broker Timeout!");
    return;
  }

  // Subscribe to a topic, the incoming messages are processed by messageHandler() function
  if (mqtt.subscribe(SUBSCRIBE_TOPIC))
    Serial.print("ESP32 - Subscribed to the topic: ");
  else
    Serial.print("ESP32 - Failed to subscribe to the topic: ");

  Serial.println(SUBSCRIBE_TOPIC);
  Serial.println("ESP32 - MQTT broker Connected!");
}

void sendToMQTT() {
  StaticJsonDocument<200> message;
  message["timestamp"] = millis();
  message["data"] = analogRead(0);  // Or you can read data from other sensors
  char messageBuffer[512];
  serializeJson(message, messageBuffer);

  mqtt.publish(PUBLISH_TOPIC, messageBuffer);

  Serial.println("ESP32 - sent to MQTT:");
  Serial.print("- topic: ");
  Serial.println(PUBLISH_TOPIC);
  Serial.print("- payload:");
  Serial.println(messageBuffer);
}

void messageHandler(String &topic, String &payload) {
  // Serial.println("ESP32 - received from MQTT:");
  // Serial.println("- topic: " + topic);
  // Serial.println("- payload:");
  // Serial.println(payload);
  if (topic == WAKE_ON_LAN_TOPIC) {
    wakeOnLan(payload);
  }
}

void wakeOnLan(String &message) {
  if (message == WAKE_ON_LAN_MESSAGE) {
    WOL.sendMagicPacket(PC_MAC_ADDRESS);
  }
}