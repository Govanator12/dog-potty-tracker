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

  // First sync time without DST to get accurate current time
  configTime(TIMEZONE_OFFSET * 3600, 0, NTP_SERVER1, NTP_SERVER2);

  // Wait for initial time sync (non-blocking check)
  int attempts = 0;
  while (time(nullptr) < 1000000000 && attempts < 50) {
    delay(100);
    attempts++;
  }

  // Now calculate DST offset based on synced time
  int dstOffset = calculateDSTOffset();

  DEBUG_PRINT("WiFiManager: DST offset = ");
  DEBUG_PRINTLN(dstOffset);

  // Reconfigure time with correct DST offset
  configTime(TIMEZONE_OFFSET * 3600, dstOffset * 3600, NTP_SERVER1, NTP_SERVER2);
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

bool WiFiManager::triggerVoiceMonkey(const char* token, const char* monkey) {
  // Check if we're connected to WiFi
  if (!isConnected()) {
    DEBUG_PRINTLN("WiFiManager: Cannot trigger Voice Monkey - not connected to WiFi");
    return false;
  }

  // Check if token is configured
  if (token == nullptr || strlen(token) == 0) {
    DEBUG_PRINTLN("WiFiManager: Voice Monkey token not configured");
    return false;
  }

  // Check if monkey name is configured
  if (monkey == nullptr || strlen(monkey) == 0) {
    DEBUG_PRINTLN("WiFiManager: Voice Monkey monkey name not configured");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();  // Skip certificate validation

  HTTPClient http;

  // Voice Monkey Trigger API endpoint (free tier)
  String url = "https://api-v2.voicemonkey.io/trigger";

  DEBUG_PRINTLN("WiFiManager: Triggering Voice Monkey routine...");
  DEBUG_PRINT("WiFiManager: Monkey: ");
  DEBUG_PRINTLN(monkey);

  // Begin HTTP connection
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json; charset=utf-8");

  // Build JSON payload for trigger
  String payload = "{\"token\":\"";
  payload += token;
  payload += "\",\"monkey\":\"";
  payload += monkey;
  payload += "\"}";

  // Send POST request
  int httpResponseCode = http.POST(payload);

  if (httpResponseCode >= 200 && httpResponseCode < 300) {
    DEBUG_PRINT("WiFiManager: Voice Monkey trigger sent successfully (HTTP ");
    DEBUG_PRINT(httpResponseCode);
    DEBUG_PRINTLN(")");
    DEBUG_PRINTLN("  -> Alexa routine will trigger announcement!");
    http.end();
    return true;
  } else if (httpResponseCode == 401 || httpResponseCode == 403) {
    DEBUG_PRINT("WiFiManager: Voice Monkey trigger FAILED (HTTP ");
    DEBUG_PRINT(httpResponseCode);
    DEBUG_PRINTLN(" - Unauthorized/Forbidden)");
    DEBUG_PRINTLN("  -> Check your VOICE_MONKEY_TOKEN in secrets.h");
    DEBUG_PRINTLN("  -> Get your token from https://voicemonkey.io/dashboard");
    http.end();
    return false;
  } else if (httpResponseCode > 0) {
    DEBUG_PRINT("WiFiManager: Voice Monkey trigger failed (HTTP ");
    DEBUG_PRINT(httpResponseCode);
    DEBUG_PRINTLN(")");
    http.end();
    return false;
  } else {
    DEBUG_PRINT("WiFiManager: Voice Monkey trigger failed (Error: ");
    DEBUG_PRINT(httpResponseCode);
    DEBUG_PRINTLN(")");
    http.end();
    return false;
  }
}

// Calculate DST offset based on US DST rules
// DST starts: Second Sunday in March at 2:00 AM
// DST ends: First Sunday in November at 2:00 AM
int WiFiManager::calculateDSTOffset() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);

  int year = timeinfo->tm_year + 1900;
  int month = timeinfo->tm_mon + 1;  // tm_mon is 0-11
  int day = timeinfo->tm_mday;
  int hour = timeinfo->tm_hour;

  // Calculate second Sunday in March
  int marchSecondSunday = 0;
  struct tm marchFirst = {0};
  marchFirst.tm_year = year - 1900;
  marchFirst.tm_mon = 2;  // March (0-based)
  marchFirst.tm_mday = 1;
  mktime(&marchFirst);  // Normalize the struct

  int firstDayOfWeek = marchFirst.tm_wday;  // 0=Sunday
  marchSecondSunday = (firstDayOfWeek == 0) ? 8 : (15 - firstDayOfWeek);

  // Calculate first Sunday in November
  int novemberFirstSunday = 0;
  struct tm novemberFirst = {0};
  novemberFirst.tm_year = year - 1900;
  novemberFirst.tm_mon = 10;  // November (0-based)
  novemberFirst.tm_mday = 1;
  mktime(&novemberFirst);  // Normalize the struct

  firstDayOfWeek = novemberFirst.tm_wday;
  novemberFirstSunday = (firstDayOfWeek == 0) ? 1 : (8 - firstDayOfWeek);

  // Check if we're currently in DST
  bool isDST = false;

  if (month > 3 && month < 11) {
    // Between April and October - always DST
    isDST = true;
  } else if (month == 3) {
    // March - check if after second Sunday at 2 AM
    if (day > marchSecondSunday) {
      isDST = true;
    } else if (day == marchSecondSunday && hour >= 2) {
      isDST = true;
    }
  } else if (month == 11) {
    // November - check if before first Sunday at 2 AM
    if (day < novemberFirstSunday) {
      isDST = true;
    } else if (day == novemberFirstSunday && hour < 2) {
      isDST = true;
    }
  }

  return isDST ? 1 : 0;
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
