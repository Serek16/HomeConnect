#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <Arduino.h>

// Callback signature: void commandHandler(String& message)
typedef void (*MQTTMessageCallback)(String topic, String payload);

class MQTTManager {
public:
  MQTTManager(const char* brokerAddress, int port, const char* clientId,
             const char* pskIdent, const char* psk,
             int bufferSize = 256);

  // One-time setup: init WiFiClientSecure + MQTTClient, connect, subscribe
  void begin();

  // Call this every loop iteration. Handles reconnection internally.
  void tick();

  // Publish a message to a topic
  void publish(const char* topic, const char* message);

  // Subscribe to a topic. Can be called multiple times before or after begin().
  // Returns true if already connected and subscription succeeded immediately,
  // false if it will be (re)applied on next connect.
  bool subscribe(const char* topic);

  // Set the callback invoked when a message arrives on any subscribed topic
  void setMessageCallback(MQTTMessageCallback cb);

  bool isConnected();

private:
  // Connection params (stored as pointers — caller must keep them alive)
  const char* brokerAddress;
  int         port;
  const char* clientId;
  const char* pskIdent;
  const char* psk;

  WiFiClientSecure wifiClient;
  MQTTClient       mqttClient;

  // User callback
  MQTTMessageCallback messageCallback = nullptr;

  // Subscription list — topics are re-subscribed after every reconnect
  static constexpr int MAX_SUBSCRIPTIONS = 8;
  const char* subscriptions[MAX_SUBSCRIPTIONS];
  int         subscriptionCount = 0;

  // --- internal ---
  bool connectToBroker();
  void resubscribeAll();

  // Static trampoline for MQTTClient callback
  static void staticMessageHandler(String topic, String message);
  static MQTTManager* currentInstance;  // points to the live instance for the trampoline
};

#endif
