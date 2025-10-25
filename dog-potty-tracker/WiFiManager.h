#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
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

  // Send Telegram notification
  bool sendTelegramNotification(const char* botToken, const char* chatID, const char* message);

  // Send Voice Monkey (Alexa) announcement
  bool sendVoiceMonkeyAnnouncement(const char* token, const char* device, const char* announcement);

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

  // Calculate DST offset based on US DST rules
  int calculateDSTOffset();

  // URL encode helper function for Telegram
  String urlencode(const char* str);
};

#endif
