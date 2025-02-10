#include <WiFi.h>
#include <MQTTClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include "MyCredentials.h"

const int PUBLISH_INTERVAL = 5*60*1000; // 5 minutes

const char* WAKE_ON_LAN_TOPIC   = "wake-on-lan";
const char* WAKE_ON_LAN_MESSAGE = "on";

WiFiClientSecure wifiClient;
MQTTClient mqttClient = MQTTClient(256);

WiFiUDP UDP;
WakeOnLan WOL(UDP);

unsigned long lastPublishTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.begin(SSID, PASS);
  printf("Connecting to WiFi \"%s\"", SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    printf(".");
  }
  printf("\nSuccessfully connected\n");

  wifiClient.setCACert(CA_CERT);
  wifiClient.setCertificate(CLIENT_CERT);
  wifiClient.setPrivateKey(CLIENT_KEY);
  
  connectToMQTT();

  WOL.setRepeat(3, 100); // Repeat the packet three times with 100ms delay between
  WOL.setBroadcastAddress("192.168.1.255");
}

void loop() {
  mqttClient.loop();

  if (millis() - lastPublishTime > PUBLISH_INTERVAL) {
    sendToMQTT();
    lastPublishTime = millis();
  }
}

void connectToMQTT() {
  // Connect to the MQTT broker
  mqttClient.begin(MQTT_BROKER_ADRRESS, MQTT_PORT, wifiClient);

  // Create a handler for incoming messages
  mqttClient.onMessage(messageHandler);

  printf("Connecting to MQTT broker with address %s", MQTT_BROKER_ADRRESS);
  while (!mqttClient.connect(MQTT_CLIENT_ID)) {
    delay(100);
    printf(".");
  }

  printf("\nMQTT broker Connected\n");

  // Subscribe to a topic, the incoming messages are processed by messageHandler() function
  if (mqttClient.subscribe(SUBSCRIBE_TOPIC))
    printf("Subscribed to the topic: \"%s\"\n", SUBSCRIBE_TOPIC);
  else
    printf("Failed to subscribe to the topic: \"%s\"\n", SUBSCRIBE_TOPIC);
}

void sendToMQTT() {
  StaticJsonDocument<200> message;
  message["timestamp"] = millis();
  message["data"] = analogRead(0);  // Or you can read data from other sensors
  char messageBuffer[512];
  serializeJson(message, messageBuffer);

  mqttClient.publish(PUBLISH_TOPIC, messageBuffer);

  printf("sent to MQTT:\n");
  printf("- topic: \"%s\"\n", PUBLISH_TOPIC);
  printf("- payload: \"%s\"\n", messageBuffer);
}

void messageHandler(String &topic, String &message) {
  printf("received from MQTT:\n");
  printf("- topic: \"%s\"\n", topic);
  printf("- message: \"%s\"\n", message);
  if (topic == WAKE_ON_LAN_TOPIC) {
    wakeOnLan(message);
  }
}

void wakeOnLan(String &message) {
  if (message == WAKE_ON_LAN_MESSAGE) {
    WOL.sendMagicPacket(PC_MAC_ADDRESS);
    printf("Wake-on-LAN. Magic Packet was sent\n");
  }
}
