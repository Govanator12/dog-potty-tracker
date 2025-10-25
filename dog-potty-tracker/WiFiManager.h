#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include "config.h"

// Callback type for handling incoming Telegram commands
typedef void (*TelegramCommandCallback)(String chatId, String command);

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

  // Poll for incoming Telegram messages (call in loop)
  void pollTelegramMessages(const char* botToken1, const char* chatID1,
                            const char* botToken2, const char* chatID2,
                            const char* botToken3, const char* chatID3);

  // Set callback for handling Telegram commands
  void setTelegramCommandCallback(TelegramCommandCallback callback);

private:
  const char* wifiSsid;
  const char* wifiPassword;
  bool timeSynced;
  unsigned long nextReconnectAttempt;
  unsigned int reconnectAttemptCount;
  bool connecting;

  // Telegram command handling
  TelegramCommandCallback commandCallback;
  unsigned long lastTelegramCheck;
  unsigned long telegramCheckInterval;
  unsigned long updateOffsets[3];  // Track update_id for each bot

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

  // Check a specific bot for messages
  void checkBotForMessages(const char* botToken, const char* chatID, int botIndex);
};

#endif
