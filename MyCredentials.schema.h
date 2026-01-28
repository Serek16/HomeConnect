// MyCredentials.h
#ifndef MYCREDENTIALS_H
#define MYCREDENTIALS_H

// WiFi Configuration
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

// MQTT Broker settings
const char* MQTT_BROKER_ADDRESS = "";
const int   MQTT_PORT           = ;
const char* MQTT_CLIENT_ID      = "";

// MQTT PSK credentials (for TLS-PSK authentication)
const char* MQTT_PSK_IDENT = "";
const char* MQTT_PSK       = "";

// Wake-on-LAN settings
const char* PC_MAC_ADDRESS  = "";
const int   WOL_PORT        = ;
const char* PC_BROADCAST_IP = "";

#endif
