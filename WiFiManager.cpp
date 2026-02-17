#include "WiFiManager.h"
#include <WiFi.h>

WiFiManager::WiFiManager() {
}

void WiFiManager::begin(String ssid, String pass) {
  prefs.begin("my-app", false);

  int count = prefs.getInt("count", 0);
  for (int i = 0; i < count; i++) {
    String epssid = prefs.getString(ssid_(i).c_str(), "");
    String eppass = prefs.getString(pass_(i).c_str(), "");
    if (!epssid.isEmpty()) {
      WiFiCredentials.insert(std::make_pair(epssid, eppass));
    }
  }

  if (!ssid.isEmpty()) {
    addWiFi(ssid, pass);
  }

  connectWiFi();
}

bool WiFiManager::chooseSSID() {
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("Couldn't find any network.");
    return false;
  } else {
    for (int i = 0; i < n; i++) {
      auto it = WiFiCredentials.find(WiFi.SSID(i));
      if (it != WiFiCredentials.end()) {
        cur_ssid = it->first;
        cur_pass = it->second;
        return true;
      }
    }
  }
  if (cur_ssid == "") return false;
}

void WiFiManager::connectWiFi() {
  bool foundSSID = chooseSSID();
  if (foundSSID) {
    int attempts = 0;
    WiFi.begin(cur_ssid, cur_pass);
    Serial.printf("Connecting to WiFi \"%s\"", cur_ssid.c_str());
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
      Serial.println("\nWiFi connection failed!");
    }
  } else {
    Serial.println("Couldn't find any available WiFi");
  }
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool WiFiManager::addWiFi(String ssid, String pass) {
  if (ssid.isEmpty() || prefs.freeEntries() == 0) {
    return false;
  }

  WiFiCredentials.insert({ssid, pass});

  prefs.clear();

  int i = 0;
  for(auto e : WiFiCredentials) {
    prefs.putString(ssid_(i).c_str(), e.first);
    prefs.putString(pass_(i).c_str(), e.second);
    i++;
  }
  prefs.putInt("count", i);

  return true;
}

bool WiFiManager::removeWiFi(String ssid) {
  if (ssid.isEmpty()) {
    return false;
  }

  if (WiFiCredentials.erase(ssid)) {
    prefs.clear();

    int i = 0;
    for(auto e : WiFiCredentials) {
      prefs.putString(ssid_(i).c_str(), e.first);
      prefs.putString(pass_(i).c_str(), e.second);
      i++;
    }
    prefs.putInt("count", i);

    return true;
  }
  return false;
}

String WiFiManager::stringWiFi() {
  String result = "";
  int i = 1;
  for(auto e : WiFiCredentials) {
    if (i > 1) result += "\n";
    result += String(i) + ". " + e.first;
    i++;
  }
  return result;
}

String WiFiManager::getCurSsid() {
  return cur_ssid;
}

String WiFiManager::ssid_(int i) {
  return String("ssid_") + i;
}

String WiFiManager::pass_(int i) {
  return String("pass_") + i;
}