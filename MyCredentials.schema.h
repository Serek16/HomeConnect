// MyCredentials.h
#ifndef MYCREDENTIALS_H
#define MYCREDENTIALS_H

const char* SSID = "";
const char* PASS = "";

//const char* MQTT_BROKER_ADRRESS = "test.mosquitto.org";
const char* MQTT_BROKER_ADRRESS = "";
const int   MQTT_PORT           = ;
const char* MQTT_CLIENT_ID      = "";
const char* MQTT_USERNAME       = ""; // not necessary
const char* MQTT_PASSWORD       = ""; // not necessary

const char* CA_CERT = R"(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)";

const char* CLIENT_CERT = R"(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)";

const char* CLIENT_KEY = R"(
-----BEGIN PRIVATE KEY-----
...
-----END PRIVATE KEY-----
)";

const char* PUBLISH_TOPIC   = "alive";
const char* SUBSCRIBE_TOPIC = "wake-on-lan";

const char* PC_MAC_ADDRESS = "";

#endif
