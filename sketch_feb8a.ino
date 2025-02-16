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

const char* WAKE_ON_LAN_TOPIC   = "wake-on-lan";
const char* WAKE_ON_LAN_MESSAGE = "on";

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

  wifiClient.setCACert(CA_CERT);
  wifiClient.setCertificate(CLIENT_CERT);
  wifiClient.setPrivateKey(CLIENT_KEY);
  
  connectToMQTT();

  WOL.setRepeat(3, 100); // Repeat the packet three times with 100ms delay between
  WOL.setBroadcastAddress("192.168.1.255");

  Keyboard.begin();

  pinMode(LED_BUILTIN, OUTPUT);
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

  Serial.printf("Connecting to MQTT broker with address %s", MQTT_BROKER_ADRRESS);
  while (!mqttClient.connect(MQTT_CLIENT_ID)) {
    delay(100);
    Serial.printf(".");
  }
  Serial.printf("\nMQTT broker Connected\n");

  // Subscribe to a topic, the incoming messages are processed by messageHandler() function
  if (mqttClient.subscribe(SUBSCRIBE_TOPIC))
    Serial.printf("Subscribed to the topic: \"%s\"\n", SUBSCRIBE_TOPIC);
  else
    Serial.printf("Failed to subscribe to the topic: \"%s\"\n", SUBSCRIBE_TOPIC);
}

void sendToMQTT() {
  StaticJsonDocument<200> message;
  message["timestamp"] = millis();
  message["data"] = analogRead(0);  // Or you can read data from other sensors
  char messageBuffer[512];
  serializeJson(message, messageBuffer);

  mqttClient.publish(PUBLISH_TOPIC, messageBuffer);

  Serial.printf("sent to MQTT:\n"
                "- topic: \"%s\"\n"
                "- payload: \"%s\"\n",
                 PUBLISH_TOPIC, messageBuffer);
}

void messageHandler(String &topic, String &message) {
  Serial.printf("received from MQTT:\n"
                "- topic: \"%s\"\n"
                "- message: \"%s\"\n",
                topic, message);
  if (topic == WAKE_ON_LAN_TOPIC) {
    if (wakeOnLan(message)) {
      bootLinux();
      if (!mqttClient.connected()) {
        connectToMQTT();
      }
    }
  }
}

bool wakeOnLan(String &message) {
  if (message == WAKE_ON_LAN_MESSAGE) {
    WOL.sendMagicPacket(PC_MAC_ADDRESS);
    Serial.printf("Wake-on-LAN. Magic Packet was sent\n");
    return true;
  }
  return false;
}

void bootLinux() {
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

  // Choose UEFI 6th boot entry from
  Serial.println("Choosing boot entry");
  for (int i = 0; i < 5; i++) {
    Keyboard.press(KEY_DOWN_ARROW);
    digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN));
    delay(100);
    Keyboard.releaseAll();
  }

  uint32_t enter_begin = millis();
  uint32_t enter_duration = 3*1000; // 15 seconds

  // Spam Enter to get past GRUB quicker
  Serial.println("Enter spam");
  while (millis() - enter_begin < enter_duration) {
    Keyboard.press(KEY_KP_ENTER);
    digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN));
    delay(100);
    Keyboard.releaseAll();
  }

  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Linux maybe successfully booted?");
}
