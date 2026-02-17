#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <unordered_map>
#include <Preferences.h>
#include <string>

struct ArduinoStringHash {
    size_t operator()(const String& s) const {
        return std::hash<std::string>{}(s.c_str());
    }
};

class WiFiManager {
public:
  WiFiManager();
  void begin(String ssid, String pass);
  bool isConnected();
  bool addWiFi(String ssid, String pass);
  bool removeWiFi(String ssid);
  String stringWiFi();
  bool chooseSSID();
  void connectWiFi();
  String getCurSsid();

private:
  std::unordered_map<String, String, ArduinoStringHash> WiFiCredentials;

  // Flash memory handle
  Preferences prefs;

  String cur_ssid;
  String cur_pass;

  String ssid_(int i);
  String pass_(int i);
};

#endif
