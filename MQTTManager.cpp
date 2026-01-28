#include "MQTTManager.h"

// Trampoline target — set before connect so the static callback can forward
MQTTManager* MQTTManager::currentInstance = nullptr;

MQTTManager::MQTTManager(const char* brokerAddress, int port, const char* clientId,
                       const char* pskIdent, const char* psk, int bufferSize)
  : brokerAddress(brokerAddress),
    port(port),
    clientId(clientId),
    pskIdent(pskIdent),
    psk(psk),
    mqttClient(bufferSize)
{
}

void MQTTManager::begin() {
  wifiClient.setPreSharedKey(pskIdent, psk);
  mqttClient.begin(brokerAddress, port, wifiClient);

  currentInstance = this;
  mqttClient.onMessage(staticMessageHandler);

  connectToBroker();
  resubscribeAll();
}

void MQTTManager::tick() {
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Disconnected, reconnecting...");
    if (!connectToBroker()) {
      return;
    }
    resubscribeAll();
  }
  mqttClient.loop();
}

bool MQTTManager::connectToBroker() {
  Serial.printf("[MQTT] Connecting to broker \"%s\"...", brokerAddress);
  int attempts = 0;
  const int maxAttempts = 50;  // 5 seconds max (50 * 100ms)
  while (!mqttClient.connect(clientId) && attempts < maxAttempts) {
    delay(100);
    Serial.print(".");
    attempts++;
  }
  if (mqttClient.connected()) {
    Serial.println(" connected.");
    return true;
  } else {
    Serial.println(" connection failed!");
    return false;
  }
}

void MQTTManager::resubscribeAll() {
  for (int i = 0; i < subscriptionCount; i++) {
    if (mqttClient.subscribe(subscriptions[i])) {
      Serial.printf("[MQTT] subscribed to \"%s\".\n", subscriptions[i]);
    } else {
      Serial.printf("[MQTT] failed to subscribe to \"%s\".\n", subscriptions[i]);
    }
  }
}

bool MQTTManager::subscribe(const char* topic) {
  // Store for re-subscription after reconnects
  if (subscriptionCount < MAX_SUBSCRIPTIONS) {
    subscriptions[subscriptionCount++] = topic;
  } else {
    Serial.printf("[MQTT] WARNING: max subscriptions (%d) reached, \"%s\" not added.\n",
                  MAX_SUBSCRIPTIONS, topic);
  }

  // If already connected, subscribe immediately
  if (mqttClient.connected()) {
    if (mqttClient.subscribe(topic)) {
      Serial.printf("[MQTT] subscribed to \"%s\".\n", topic);
      return true;
    } else {
      Serial.printf("[MQTT] failed to subscribe to \"%s\".\n", topic);
      return false;
    }
  }

  // Not connected yet — will be applied in resubscribeAll() after connect
  return false;
}

void MQTTManager::publish(const char* topic, const char* message) {
  mqttClient.publish(topic, message);
  Serial.printf("[MQTT] sent:\n"
                "  topic:   \"%s\"\n"
                "  message: \"%s\"\n",
                topic, message);
}

void MQTTManager::setMessageCallback(MQTTMessageCallback cb) {
  messageCallback = cb;
}

void MQTTManager::staticMessageHandler(String topic, String message) {
    if (currentInstance && currentInstance->messageCallback) {
        currentInstance->messageCallback(topic, message);
    }
}

bool MQTTManager::isConnected() {
  return mqttClient.connected();
}
