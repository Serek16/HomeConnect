#include "MyCredentials.h"
#include "WiFiManager.h"
#include "MQTTManager.h"
#include "PCPower.h"

#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include <USBHIDKeyboard.h>
#include <ArduinoOTA.h>

const char* MQTT_COMMAND_TOPIC       = "ssh-home-cmd";
const char* MQTT_RESPONSE_TOPIC      = "ssh-home-res";
const char* PC_ON_MESSAGE            = "pc-on";
const char* PC_ON_BNUM_MESSAGE       = "pc-on_";
const char* PC_OFF_MESSAGE           = "pc-off";
const char* PC_CHECK_MESSAGE         = "pc-check";
const char* MQTT_COMMAND_ACK_MESSAGE = "ack";
const char* ESP_CHECK_MESSAGE        = "esp-check";
const char* WIFI_ADD_MESSAGE         = "wifi-add_";
const char* WIFI_REMOVE_MESSAGE      = "wifi-remove_";
const char* WIFI_LIST_MESSAGE        = "wifi-list";
const char* WIFI_CURRENT_MESSAGE     = "wifi-current";
const char* PC_WOL_MESSAGE           = "pc-wol";
const char* PC_WOL_BNUM_MESSAGE      = "pc-wol_";

WiFiManager wifi;
MQTTManager mqtt(MQTT_BROKER_ADDRESS, MQTT_PORT, MQTT_CLIENT_ID, MQTT_PSK_IDENT, MQTT_PSK);

const uint8_t POWER_SW_PIN  = 18;
const uint8_t POWER_LED_PIN = 33;
PCPower pc(POWER_SW_PIN, POWER_LED_PIN);

WiFiUDP UDP;
WakeOnLan WOL(UDP);

USBHIDKeyboard Keyboard;

void setup() {
  Serial.begin(115200);
  delay(1000);       // it helps the first message
  Serial.println();  //  not to be trimmed

  wifi.begin(WIFI_SSID, WIFI_PASS);

  mqtt.begin();
  mqtt.setMessageCallback(onMQTTMessage);
  mqtt.subscribe(MQTT_COMMAND_TOPIC);

  WOL.setRepeat(3, 100); // Repeat the packet three times with 100 ms delay between
  WOL.setBroadcastAddress(WOL_BROADCAST_IP);

  Keyboard.begin();

  setupOTA();

  pinMode(POWER_SW_PIN, OUTPUT);
  pinMode(POWER_LED_PIN, INPUT);
}

void loop() {
  if (wifi.isConnected())
    mqtt.tick();
  else
    wifi.connectWiFi();
  delay(10);
}

void mqttResponse(const char* message) {
  mqtt.publish(MQTT_RESPONSE_TOPIC, message);
}

void mqttReportPCState() {
  bool state = pc.isOn();
  Serial.println(state ? "PC is ON." : "PC is OFF.");
  mqttResponse(state ? "PC is ON." : "PC is OFF.");
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
      mqttResponse("PC is already ON.");
      return;
    }
    Serial.println("PC is turning on...");
    mqttResponse("PC is turning on...");
    pc.pressShort();
  }
  else if (command.startsWith(PC_ON_BNUM_MESSAGE)) {
    if (pc.isOn()) {
      Serial.println("PC is already ON.");
      mqttResponse("PC is already ON.");
      return;
    }
    Serial.println("PC is turning on...");
    mqttResponse("PC is turning on...");
    pc.pressShort();
    bootOS(readBootNum(command));
  }
  else if (command == PC_OFF_MESSAGE) {
    if (!pc.isOn()) {
      Serial.println("PC is already OFF.");
      mqttResponse("PC is already OFF.");
      return;
    }
    Serial.println("PC is turning off...");
    mqttResponse("PC is turning off...");
    pc.pressLong();
  }
  else if (command == PC_CHECK_MESSAGE) {
    mqttReportPCState();
  }
  else if (command == ESP_CHECK_MESSAGE) {
    Serial.println("ESP is ON.");
    mqttResponse("ESP is ON.");
    mqttResponse(MQTT_COMMAND_ACK_MESSAGE);
  }
  else if (command.startsWith(WIFI_ADD_MESSAGE)) {
    int sep = command.indexOf('\n');
    if (sep == -1) {
      Serial.println("Couldn't add WiFi");
      mqttResponse("Couldn't add WiFi");
    }
    int cmdSep = strlen(WIFI_ADD_MESSAGE);
    String ssid = command.substring(cmdSep, sep);
    String pass = command.substring(sep + 1);
    bool res = wifi.addWiFi(ssid, pass);
    Serial.println(res ? "WiFi added" : "Couldn't add WiFi");
    mqttResponse(res ? "WiFi added" : "Couldn't add WiFi");
  }
  else if (command.startsWith(WIFI_REMOVE_MESSAGE)) {
    int cmdSep = strlen(WIFI_REMOVE_MESSAGE);
    String ssid = command.substring(cmdSep);
    bool res = wifi.removeWiFi(ssid);
    Serial.println(res ? "WiFi deleted" : "Couldn't delete WiFi");
    mqttResponse(res ? "WiFi deleted" : "Couldn't delete WiFi");
  }
  else if (command == WIFI_LIST_MESSAGE) {
    String res = wifi.stringWiFi();
    Serial.println(res);
    mqttResponse(res.c_str());
  }
  else if (command == WIFI_CURRENT_MESSAGE) {
    String curSsid = wifi.getCurSsid();
    Serial.println(curSsid);
    mqttResponse(curSsid.c_str());
  }
  else if (command == PC_WOL_MESSAGE) {
    WOL.sendMagicPacket(PC_MAC_ADDRESS);
    Serial.println("Wake-on-LAN Magic Packet was sent.");
    mqttResponse("Wake-on-LAN Magic Packet was sent.");
  }
  else if (command.startsWith(PC_WOL_BNUM_MESSAGE)) {
    WOL.sendMagicPacket(PC_MAC_ADDRESS);
    Serial.println("Wake-on-LAN Magic Packet was sent.");
    mqttResponse("Wake-on-LAN Magic Packet was sent.");
    bootOS(readBootNum(command));
  }
}

void bootOS(int bootNumber) {
  Serial.printf("Booting OS No. $d from UEFI Boot Entry List.\n", bootNumber);
  mqttResponse(String("Booting OS No. " + String(bootNumber)).c_str());
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

void setupOTA() {
  // Hostname for OTA (will appear as "ESP32-PC-Controller" on network)
  ArduinoOTA.setHostname("ESP32-S3-SSH-HOME");

  // Password for OTA (OPTIONAL but RECOMMENDED for security)
  ArduinoOTA.setPassword("X^7:F8\"GjN,n^PA");  // Uncomment and set password

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);

    // Turn off keyboard and other peripherals during update
    digitalWrite(LED_BUILTIN, HIGH);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    digitalWrite(LED_BUILTIN, LOW);
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    // Blink LED during update
    digitalWrite(LED_BUILTIN, (millis() / 250) % 2);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
    digitalWrite(LED_BUILTIN, LOW);
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}
