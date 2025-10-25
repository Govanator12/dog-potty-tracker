#include "WiFiManager.h"

WiFiManager::WiFiManager() :
  timeSynced(false),
  nextReconnectAttempt(0),
  reconnectAttemptCount(0),
  connecting(false),
  commandCallback(nullptr),
  lastTelegramCheck(0),
  telegramCheckInterval(5000),  // Check every 5 seconds (reduced to prevent SSL exhaustion)
  replyPending(false),
  replyPendingSince(0)
{
  updateOffsets[0] = 0;
  updateOffsets[1] = 0;
  updateOffsets[2] = 0;
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

  // Build Telegram API URL
  String url = "https://api.telegram.org/bot";
  url += botToken;
  url += "/sendMessage?chat_id=";
  url += chatID;
  url += "&text=";
  url += urlencode(message);

  DEBUG_PRINTLN("WiFiManager: Sending Telegram notification...");
  DEBUG_PRINT("WiFiManager: Message length: ");
  DEBUG_PRINTLN(strlen(message));
  DEBUG_PRINT("WiFiManager: URL length: ");
  DEBUG_PRINTLN(url.length());
  DEBUG_PRINT("WiFiManager: Free heap before: ");
  DEBUG_PRINTLN(ESP.getFreeHeap());

  // Single attempt with long delay to allow SSL stack to recover
  DEBUG_PRINTLN("WiFiManager: Waiting 5 seconds for SSL stack to clear...");
  delay(5000);

  DEBUG_PRINT("WiFiManager: Free heap after delay: ");
  DEBUG_PRINTLN(ESP.getFreeHeap());

  for (int attempt = 1; attempt <= 1; attempt++) {  // Only 1 attempt

    // Create fresh client for each attempt
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(10000);

    HTTPClient http;
    http.setTimeout(10000);
    http.setReuse(false);

    if (!http.begin(client, url)) {
      DEBUG_PRINTLN("WiFiManager: Failed to begin HTTP connection");
      client.stop();
      continue;  // Try again
    }

    int httpResponseCode = http.GET();
    http.end();
    client.stop();

    if (httpResponseCode > 0) {
      DEBUG_PRINT("WiFiManager: Telegram notification sent successfully (HTTP ");
      DEBUG_PRINT(httpResponseCode);
      DEBUG_PRINTLN(")");
      return true;
    } else {
      DEBUG_PRINT("WiFiManager: Attempt ");
      DEBUG_PRINT(attempt);
      DEBUG_PRINT(" failed (Error: ");
      DEBUG_PRINT(httpResponseCode);
      DEBUG_PRINTLN(")");
    }
  }

  // All retries failed
  DEBUG_PRINTLN("WiFiManager: All retry attempts failed");
  return false;
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

// Set callback for handling Telegram commands
void WiFiManager::setTelegramCommandCallback(TelegramCommandCallback callback) {
  commandCallback = callback;
}

// Mark that a reply is pending (prevents polling during reply)
void WiFiManager::setReplyPending(bool pending) {
  replyPending = pending;
  if (pending) {
    replyPendingSince = millis();
    DEBUG_PRINTLN("WiFiManager: Reply pending - pausing polling");
  } else {
    DEBUG_PRINTLN("WiFiManager: Reply complete - resuming polling");
  }
}

// Poll for incoming Telegram messages
void WiFiManager::pollTelegramMessages(const char* botToken1, const char* chatID1,
                                       const char* botToken2, const char* chatID2,
                                       const char* botToken3, const char* chatID3) {
  // Only poll if connected to WiFi
  if (!isConnected()) {
    return;
  }

  // Don't poll if a reply is pending (prevents SSL connection exhaustion)
  if (replyPending) {
    unsigned long now = millis();
    // Auto-clear flag after 30 seconds in case something went wrong
    if (now - replyPendingSince > 30000) {
      DEBUG_PRINTLN("WiFiManager: Reply timeout - clearing pending flag");
      replyPending = false;
    } else {
      return;  // Skip polling while reply is being sent
    }
  }

  // Rate limit checks to every 5 seconds
  unsigned long now = millis();
  if (now - lastTelegramCheck < telegramCheckInterval) {
    return;
  }
  lastTelegramCheck = now;

  // Poll each configured bot for messages
  if (botToken1 != nullptr && strlen(botToken1) > 0 && chatID1 != nullptr && strlen(chatID1) > 0) {
    checkBotForMessages(botToken1, chatID1, 0);
  }

  if (botToken2 != nullptr && strlen(botToken2) > 0 && chatID2 != nullptr && strlen(chatID2) > 0) {
    checkBotForMessages(botToken2, chatID2, 1);
  }

  if (botToken3 != nullptr && strlen(botToken3) > 0 && chatID3 != nullptr && strlen(chatID3) > 0) {
    checkBotForMessages(botToken3, chatID3, 2);
  }
}

// Helper function to check a specific bot for messages
void WiFiManager::checkBotForMessages(const char* botToken, const char* chatID, int botIndex) {
  WiFiClientSecure client;
  client.setInsecure();  // Skip certificate validation

  HTTPClient http;

  // Build Telegram API URL to get updates
  // Use long polling with offset to only get new messages
  String url = "https://api.telegram.org/bot";
  url += botToken;
  url += "/getUpdates?offset=";
  url += String(updateOffsets[botIndex]);
  url += "&timeout=0";  // Don't wait, return immediately

  DEBUG_PRINT("WiFiManager: Checking for Telegram messages (bot ");
  DEBUG_PRINT(botIndex + 1);
  DEBUG_PRINTLN(")");

  // Begin HTTP connection
  http.begin(client, url);

  // Send GET request
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    String response = http.getString();
    DEBUG_PRINTLN("WiFiManager: Got Telegram response");

    // Parse JSON response manually (simple parsing since we only need a few fields)
    // Look for "update_id", "chat":{"id"}, and "text"

    int updateIdPos = response.indexOf("\"update_id\":");
    if (updateIdPos > 0) {
      // Extract update_id
      int updateIdStart = updateIdPos + 12;  // Length of "update_id":
      int updateIdEnd = response.indexOf(',', updateIdStart);
      if (updateIdEnd < 0) updateIdEnd = response.indexOf('}', updateIdStart);
      String updateIdStr = response.substring(updateIdStart, updateIdEnd);
      unsigned long newUpdateId = updateIdStr.toInt() + 1;

      // Update offset to avoid processing same message twice
      if (newUpdateId > updateOffsets[botIndex]) {
        updateOffsets[botIndex] = newUpdateId;

        // Extract chat ID
        int chatIdPos = response.indexOf("\"chat\":{\"id\":");
        if (chatIdPos > 0) {
          int chatIdStart = chatIdPos + 13;  // Length of "chat":{"id":
          int chatIdEnd = response.indexOf(',', chatIdStart);
          if (chatIdEnd < 0) chatIdEnd = response.indexOf('}', chatIdStart);
          String chatIdStr = response.substring(chatIdStart, chatIdEnd);

          // Only process if message is from the authorized chat ID
          if (chatIdStr == String(chatID)) {
            // Extract text/command
            int textPos = response.indexOf("\"text\":\"");
            if (textPos > 0) {
              int textStart = textPos + 8;  // Length of "text":"
              int textEnd = response.indexOf("\"", textStart);
              String command = response.substring(textStart, textEnd);

              DEBUG_PRINT("WiFiManager: Received command: ");
              DEBUG_PRINTLN(command);

              // Call the callback with chat ID and command
              if (commandCallback != nullptr) {
                commandCallback(chatIdStr, command);
              }
            }
          } else {
            DEBUG_PRINTLN("WiFiManager: Message from unauthorized chat ID - ignoring");
          }
        }
      }
    }
  } else if (httpResponseCode < 0) {
    DEBUG_PRINT("WiFiManager: Telegram polling failed (Error: ");
    DEBUG_PRINT(httpResponseCode);
    DEBUG_PRINTLN(")");
  }

  http.end();
}

// Trigger Voice Monkey device (Alexa routine)
bool WiFiManager::triggerVoiceMonkey(const char* token, const char* device) {
  // Check if we're connected to WiFi
  if (!isConnected()) {
    DEBUG_PRINTLN("WiFiManager: Cannot trigger Voice Monkey - not connected to WiFi");
    return false;
  }

  // Check if token and device are configured
  if (token == nullptr || strlen(token) == 0 || device == nullptr || strlen(device) == 0) {
    DEBUG_PRINTLN("WiFiManager: Voice Monkey token or device not configured");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();  // Skip certificate validation

  HTTPClient http;

  // Build Voice Monkey API URL with GET parameters
  // Convert device name to lowercase (Voice Monkey API requires lowercase)
  String deviceLower = String(device);
  deviceLower.toLowerCase();

  String url = "https://api-v2.voicemonkey.io/trigger?token=";
  url += token;
  url += "&device=";
  url += deviceLower;

  DEBUG_PRINT("WiFiManager: Triggering Voice Monkey device: ");
  DEBUG_PRINT(device);
  DEBUG_PRINT(" (lowercased to: ");
  DEBUG_PRINT(deviceLower);
  DEBUG_PRINTLN(")");
  DEBUG_PRINT("WiFiManager: Full URL: ");
  DEBUG_PRINTLN(url);

  // Begin HTTP connection
  http.begin(client, url);

  // Send GET request
  int httpResponseCode = http.GET();

  if (httpResponseCode >= 200 && httpResponseCode < 300) {
    DEBUG_PRINT("WiFiManager: Voice Monkey triggered successfully (HTTP ");
    DEBUG_PRINT(httpResponseCode);
    DEBUG_PRINTLN(")");
    http.end();
    return true;
  } else {
    DEBUG_PRINT("WiFiManager: Voice Monkey trigger failed (HTTP ");
    DEBUG_PRINT(httpResponseCode);
    DEBUG_PRINTLN(")");
    http.end();
    return false;
  }
}
