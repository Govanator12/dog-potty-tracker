#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <time.h>
#include "config.h"

class WiFiManager {
public:
  WiFiManager();

  // Begin WiFi connection
  void begin(const char* ssid, const char* password);

  // Update WiFi state (call every loop iteration)
  void update();

  // Check if connected to WiFi
  bool isConnected();

  // Check if time has been synced
  bool isTimeSynced();

  // Force time sync
  void syncTime();

  // Send IFTTT notification
  bool sendIFTTTNotification(const char* webhookKey, const char* eventName, const char* value1);

private:
  const char* wifiSsid;
  const char* wifiPassword;
  bool timeSynced;
  unsigned long nextReconnectAttempt;
  unsigned int reconnectAttemptCount;
  bool connecting;

  // Attempt reconnection to WiFi
  void attemptReconnect();

  // Update reconnection backoff
  void updateReconnectBackoff();

  // Check if time sync is complete
  bool checkTimeSync();
};

#endif
