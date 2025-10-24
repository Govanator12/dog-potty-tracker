#include "WiFiManager.h"

WiFiManager::WiFiManager() :
  timeSynced(false),
  nextReconnectAttempt(0),
  reconnectAttemptCount(0),
  connecting(false)
{
}

void WiFiManager::begin(const char* ssid, const char* password) {
  wifiSsid = ssid;
  wifiPassword = password;

  DEBUG_PRINTLN("WiFiManager: Starting connection...");
  DEBUG_PRINT("SSID: ");
  DEBUG_PRINTLN(ssid);

  // Set WiFi mode
  WiFi.mode(WIFI_STA);

  // Start connection
  WiFi.begin(wifiSsid, wifiPassword);
  connecting = true;
}

void WiFiManager::update() {
  // Check if we're connected
  if (WiFi.status() == WL_CONNECTED) {
    // If we just connected
    if (reconnectAttemptCount > 0 || connecting) {
      DEBUG_PRINTLN("WiFiManager: Connected!");
      DEBUG_PRINT("IP address: ");
      DEBUG_PRINTLN(WiFi.localIP());

      reconnectAttemptCount = 0;
      connecting = false;

      // Start time sync
      syncTime();
    }

    // Check if time sync is complete
    if (!timeSynced) {
      if (checkTimeSync()) {
        timeSynced = true;
        DEBUG_PRINTLN("WiFiManager: Time synced!");
      }
    }
  }
  // Not connected - attempt reconnection
  else {
    // Check if it's time to attempt reconnection
    if (millis() >= nextReconnectAttempt) {
      attemptReconnect();
    }
  }
}

void WiFiManager::attemptReconnect() {
  DEBUG_PRINTLN("WiFiManager: Attempting reconnection...");

  WiFi.disconnect();
  WiFi.begin(wifiSsid, wifiPassword);

  reconnectAttemptCount++;
  updateReconnectBackoff();
}

void WiFiManager::updateReconnectBackoff() {
  // Exponential backoff: 1s, 2s, 4s, 8s, 16s, 32s, 60s (cap)
  unsigned long backoffTime = min((unsigned long)pow(2, reconnectAttemptCount) * 1000, (unsigned long)WIFI_RECONNECT_MAX_BACKOFF);
  nextReconnectAttempt = millis() + backoffTime;

  DEBUG_PRINT("WiFiManager: Next attempt in ");
  DEBUG_PRINT(backoffTime / 1000);
  DEBUG_PRINTLN(" seconds");
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool WiFiManager::isTimeSynced() {
  return timeSynced;
}

void WiFiManager::syncTime() {
  DEBUG_PRINTLN("WiFiManager: Syncing time with NTP...");

  // Configure time with timezone offset
  configTime(TIMEZONE_OFFSET * 3600, DST_OFFSET * 3600, NTP_SERVER1, NTP_SERVER2);
}

bool WiFiManager::checkTimeSync() {
  time_t now = time(nullptr);
  // Valid time is after year 2001 (timestamp > 1000000000)
  return now > 1000000000;
}

bool WiFiManager::sendTelegramNotification(const char* botToken, const char* chatID, const char* message) {
  // Check if we're connected to WiFi
  if (!isConnected()) {
    DEBUG_PRINTLN("WiFiManager: Cannot send Telegram notification - not connected to WiFi");
    return false;
  }

  // Check if bot token and chat ID are configured
  if (botToken == nullptr || strlen(botToken) == 0 || chatID == nullptr || strlen(chatID) == 0) {
    DEBUG_PRINTLN("WiFiManager: Telegram bot token or chat ID not configured");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();  // Skip certificate validation (necessary for ESP8266)

  HTTPClient http;

  // Build Telegram API URL
  String url = "https://api.telegram.org/bot";
  url += botToken;
  url += "/sendMessage?chat_id=";
  url += chatID;
  url += "&text=";
  url += urlencode(message);

  DEBUG_PRINTLN("WiFiManager: Sending Telegram notification...");

  // Begin HTTP connection
  http.begin(client, url);

  // Send GET request
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    DEBUG_PRINT("WiFiManager: Telegram notification sent successfully (HTTP ");
    DEBUG_PRINT(httpResponseCode);
    DEBUG_PRINTLN(")");
    http.end();
    return true;
  } else {
    DEBUG_PRINT("WiFiManager: Telegram notification failed (Error: ");
    DEBUG_PRINT(httpResponseCode);
    DEBUG_PRINTLN(")");
    http.end();
    return false;
  }
}

bool WiFiManager::sendNotifyMeNotification(const char* accessCode, const char* message) {
  // Check if we're connected to WiFi
  if (!isConnected()) {
    DEBUG_PRINTLN("WiFiManager: Cannot send Notify Me notification - not connected to WiFi");
    return false;
  }

  // Check if access code is configured
  if (accessCode == nullptr || strlen(accessCode) == 0) {
    DEBUG_PRINTLN("WiFiManager: Notify Me access code not configured");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();  // Skip certificate validation

  HTTPClient http;

  // Notify Me API endpoint
  String url = "https://api.notifymyecho.com/v1/NotifyMe";

  DEBUG_PRINTLN("WiFiManager: Sending Notify Me (Alexa) notification...");

  // Begin HTTP connection
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  // Build JSON payload
  String payload = "{\"notification\":\"";
  payload += message;
  payload += "\",\"accessCode\":\"";
  payload += accessCode;
  payload += "\"}";

  // Send POST request
  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    DEBUG_PRINT("WiFiManager: Notify Me notification sent successfully (HTTP ");
    DEBUG_PRINT(httpResponseCode);
    DEBUG_PRINTLN(")");
    http.end();
    return true;
  } else {
    DEBUG_PRINT("WiFiManager: Notify Me notification failed (Error: ");
    DEBUG_PRINT(httpResponseCode);
    DEBUG_PRINTLN(")");
    http.end();
    return false;
  }
}

// URL encode helper function
String WiFiManager::urlencode(const char* str) {
  String encoded = "";
  char c;
  char code0;
  char code1;

  for (int i = 0; i < strlen(str); i++) {
    c = str[i];
    if (c == ' ') {
      encoded += '+';
    } else if (isalnum(c)) {
      encoded += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encoded += '%';
      encoded += code0;
      encoded += code1;
    }
  }
  return encoded;
}
