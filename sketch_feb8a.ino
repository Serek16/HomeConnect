#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <WiFi.h>
#include <MQTTClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "MyCredentials.h"

const int PUBLISH_INTERVAL = 5*60*1000; // 5 minutes

const char* WAKE_ON_LAN_TOPIC       = "wake-on-lan";
const char* WAKE_ON_LAN_TOPIC_BNUM  = "wake-on-lan-boot-number";
const char* WAKE_ON_LAN_MESSAGE     = "on";
const char* WAKE_ON_LAN_MESSAGE_ACK = "ack";
const char* ALIVE_CHECK_TOPIC       = "alive";

WiFiClientSecure wifiClient;
MQTTClient mqttClient = MQTTClient(256);

WiFiUDP UDP;
WakeOnLan WOL(UDP);

USBHIDKeyboard Keyboard;

unsigned long lastPublishTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();

  WiFi.begin(SSID, PASS);
  Serial.printf("Connecting to WiFi \"%s\"", SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.printf(".");
  }
  Serial.printf("\nSuccessfully connected\n");
  wifiClient.setPreSharedKey(PSK_IDENT, PSK);
  
  connectToMQTT();

  WOL.setRepeat(3, 100); // Repeat the packet three times with 100ms delay between
  WOL.setBroadcastAddress("192.168.1.255");

  Keyboard.begin();

  pinMode(LED_BUILTIN, OUTPUT);
}

uint32_t lastMqttCheck = millis();
uint32_t mqttCheckInterval = 5*1*1000; // 5 seconds
void loop() {
  mqttClient.loop();

  if (millis() - lastMqttCheck >= mqttCheckInterval) {
    lastMqttCheck = millis();
    if (!mqttClient.connected()) {
      Serial.println("Had to reconnect");
      connectToMQTT();
    }
  }

  if (millis() - lastPublishTime > PUBLISH_INTERVAL) {
    StaticJsonDocument<200> message;
    message["timestamp"] = millis();
    message["data"] = analogRead(0);  // Or you can read data from other sensors
    char messageBuffer[512];
    serializeJson(message, messageBuffer);
    sendToMQTT(ALIVE_CHECK_TOPIC, messageBuffer);
    lastPublishTime = millis();
  }

  delay(5000); // 5 seconds
}

void connectToMQTT() {
  // Connect to the MQTT broker
  mqttClient.begin(MQTT_BROKER_ADRRESS, MQTT_PORT, wifiClient);

  // Create a handler for incoming messages
  mqttClient.onMessage(messageHandler);

  Serial.printf("Connecting to MQTT broker with address %s", MQTT_BROKER_ADRRESS);
  while (!mqttClient.connect(MQTT_CLIENT_ID)) {
    delay(100);
    Serial.printf(".");
  }
  Serial.printf("\nMQTT broker Connected\n");

  // Subscribe to a topic, the incoming messages are processed by messageHandler() function
  if (mqttClient.subscribe(WAKE_ON_LAN_TOPIC))
    Serial.printf("Subscribed to the topic: \"%s\"\n", WAKE_ON_LAN_TOPIC);
  else
    Serial.printf("Failed to subscribe to the topic: \"%s\"\n", WAKE_ON_LAN_TOPIC);

  if (mqttClient.subscribe(WAKE_ON_LAN_TOPIC_BNUM))
    Serial.printf("Subscribed to the topic: \"%s\"\n", WAKE_ON_LAN_TOPIC_BNUM);
  else
    Serial.printf("Failed to subscribe to the topic: \"%s\"\n", WAKE_ON_LAN_TOPIC_BNUM);
}

// Send a message on given topic using mosquitto client
void sendToMQTT(const char* topic, const char* message) {
  mqttClient.publish(topic, message);
  Serial.printf("sent to MQTT:\n"
                "- topic: \"%s\"\n"
                "- payload: \"%s\"\n",
                 topic, message);
}

// Subscription message handeler passed to mosquitto client
void messageHandler(String &topic, String &message) {
  Serial.printf("received from MQTT:\n"
                "- topic: \"%s\"\n"
                "- message: \"%s\"\n",
                topic, message);

  // Use default second UEFI boot entry
  if (topic == WAKE_ON_LAN_TOPIC) {
    if (message == WAKE_ON_LAN_MESSAGE) {
      sendToMQTT(WAKE_ON_LAN_TOPIC, WAKE_ON_LAN_MESSAGE_ACK); // Send ack back
      WOL.sendMagicPacket(PC_MAC_ADDRESS);                    // WoL - power on computer
      Serial.println("Wake-on-LAN Magic Packet was sent.");
      bootLinux(1); //second boot entry                       // Try to boot to Linux immediately after pc is powered on
      if (!mqttClient.connected()) {
        connectToMQTT();
      }
    }
  }

  // Use given number of UEFI boot entry
  else if(topic == WAKE_ON_LAN_TOPIC_BNUM) {
    long bootNumber;
    if (parseIntWithSscanf(message, bootNumber)) {
      sendToMQTT(WAKE_ON_LAN_TOPIC_BNUM, WAKE_ON_LAN_MESSAGE_ACK); // Send ack back
      WOL.sendMagicPacket(PC_MAC_ADDRESS);                         // WoL - power on computer
      Serial.println("Wake-on-LAN Magic Packet was sent.");
      bootLinux(bootNumber);                                       // Try to boot to Linux immediately after pc is powered on
      if (!mqttClient.connected()) {
        connectToMQTT();
      }
    }
  }
}

void bootLinux(int bootNumber) {
  uint32_t f12_begin = millis();
  uint32_t f12_duration = 15*1000; // 15 seconds

  // Spam F12 to enter UEFI boot entry list
  Serial.println("Entering UEFI boot entry list");
  while (millis() - f12_begin < f12_duration) {
    Keyboard.press(KEY_F12);
    digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN));
    delay(100);
    Keyboard.releaseAll();
  }

  // Choose UEFI boot entry on given position
  Serial.printf("Choosing %i boot entry.", bootNumber);
  for (int i = 0; i < bootNumber; i++) {
    Keyboard.press(KEY_DOWN_ARROW);
    digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN));
    delay(100);
    Keyboard.releaseAll();
  }

  uint32_t enter_begin = millis();
  uint32_t enter_duration = 3*1000; // 15 seconds

  // Spam Enter to get past GRUB quicker
  Serial.println("Enter spamming...");
  while (millis() - enter_begin < enter_duration) {
    Keyboard.press(KEY_KP_ENTER);
    digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN));
    delay(100);
    Keyboard.releaseAll();
  }

  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Linux ?maybe? successfully booted!");
}


bool parseIntWithSscanf(const String &s, long &out) {
  // make a NUL-terminated copy
  int len = s.length();
  if (len == 0) return false;
  char buf[len + 1];
  s.toCharArray(buf, len + 1);

  char extra;
  int matched = sscanf(buf, " %ld %c", &out, &extra);
  return (matched == 1);
}
