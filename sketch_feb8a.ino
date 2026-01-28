#include <WiFi.h>
#include <MQTTClient.h>
#include <WiFiClientSecure.h>
#include <USBHIDKeyboard.h>

#include "MyCredentials.h"
#include "PCPower.h"
#include "MQTTManager.h"

const char* MQTT_COMMAND_TOPIC       = "ssh-home";
const char* PC_ON_MESSAGE            = "pc-on";
const char* PC_ON_BNUM_MESSAGE       = "pc-on_";
const char* PC_OFF_MESSAGE           = "pc-off";
const char* PC_CHECK_MESSAGE         = "pc-check";
const char* MQTT_COMMAND_ACK_MESSAGE = "ack";
const char* ESP_CHECK_MESSAGE        = "esp-check";

USBHIDKeyboard Keyboard;

const uint8_t POWER_SW_PIN  = 18;
const uint8_t POWER_LED_PIN = 33;
PCPower pc(POWER_SW_PIN, POWER_LED_PIN);

#include "MQTTManager.h"
MQTTManager mqtt(MQTT_BROKER_ADDRESS, MQTT_PORT, MQTT_CLIENT_ID, MQTT_PSK_IDENT, MQTT_PSK);

void setup() {
  Serial.begin(115200);
  delay(1000);       // it helps the first message
  Serial.println();  //  not to be trimmed

  connectWiFi();

  mqtt.begin();
  mqtt.setMessageCallback(onMQTTMessage);
  mqtt.subscribe(MQTT_COMMAND_TOPIC);

  Keyboard.begin();

  pinMode(POWER_SW_PIN, OUTPUT);
  pinMode(POWER_LED_PIN, INPUT);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED)
    mqtt.tick();
  else
    connectWiFi();
  delay(10);
}

void connectWiFi() {
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed! Retrying in 5 seconds.");
    delay(5000);
    connectWiFi();
  }
}

void mqttReportPCState() {
  bool state = pc.isOn();
  Serial.println(state ? "PC is ON." : "PC is OFF.");
  mqtt.publish(MQTT_COMMAND_TOPIC, state ? "PC is ON." : "PC is OFF.");
}

int readBootNum(String str) {
  int i = str.lastIndexOf("_");
  if (i == -1) return 0;
  String sInt = str.substring(i+1);
  if (sInt.isEmpty()) return 0;
  return sInt.toInt();
}

void onMQTTMessage(String topic, String payload) {
  if (topic == MQTT_COMMAND_TOPIC)
    handleCommand(payload);
}

void handleCommand(String &command) {
  if (command == PC_ON_MESSAGE) {
    if (pc.isOn()) {
      Serial.println("PC is already ON.");
      mqtt.publish(MQTT_COMMAND_TOPIC, "PC is already ON.");
      return;
    }
    Serial.println("PC is turning on...");
    mqtt.publish(MQTT_COMMAND_TOPIC, "PC is turning on...");
    pc.pressShort();
  }
  else if (command.startsWith(PC_ON_BNUM_MESSAGE)) {
    if (pc.isOn()) {
      Serial.println("PC is already ON.");
      mqtt.publish(MQTT_COMMAND_TOPIC, "PC is already ON.");
      return;
    }
    Serial.println("PC is turning on...");
    mqtt.publish(MQTT_COMMAND_TOPIC, "PC is turning on...");
    pc.pressShort();
    bootOS(readBootNum(command));
  }
  else if (command == PC_OFF_MESSAGE) {
    if (!pc.isOn()) {
      Serial.println("PC is already OFF.");
      mqtt.publish(MQTT_COMMAND_TOPIC, "PC is already OFF.");
      return;
    }
    Serial.println("PC is turning off...");
    mqtt.publish(MQTT_COMMAND_TOPIC, "PC is turning off...");
    pc.pressLong();
    delay(500);
    mqttReportPCState();
  }
  else if (command == PC_CHECK_MESSAGE) {
    mqttReportPCState();
  }
  else if (command == ESP_CHECK_MESSAGE) {
    Serial.println("ESP is ON.");
    mqtt.publish(MQTT_COMMAND_TOPIC, "ESP is ON.");
    mqtt.publish(MQTT_COMMAND_TOPIC, MQTT_COMMAND_ACK_MESSAGE);
  }
}

void bootOS(int bootNumber) {
  Serial.printf("Booting OS No. $d from UEFI Boot Entry List.\n", bootNumber);
  mqtt.publish(MQTT_COMMAND_TOPIC, String("Booting OS No. " + String(bootNumber)).c_str());
  // Start spamming boot menu key - F12
  uint32_t start = millis();
  while (millis() - start < 15000) {
    Keyboard.press(KEY_F12);
    delay(50);
    Keyboard.releaseAll();
    delay(50);
  }

  // Use down arrow to choose specific boot entry from the list
  for (int i = 0; i < bootNumber; i++) {
    Keyboard.press(KEY_DOWN_ARROW);
    delay(50);
    Keyboard.releaseAll();
    delay(50);
  }

  // Accept UEFI boot mntry and GRUB default boot entry
  start = millis();
  while (millis() - start < 5000) {
    // Accept using Enter
    Keyboard.press(KEY_KP_ENTER);
    delay(50);
    Keyboard.releaseAll();
    delay(50);
  }
}
