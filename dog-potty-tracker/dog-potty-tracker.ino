/*
 * Dog Potty Tracker
 *
 * A hardware device to track your dog's potty activities with three
 * independent timers (Outside, Pee, Poop) displayed on an OLED screen
 * with visual LED status indicators.
 *
 * Hardware:
 * - WeMos D1 Mini (ESP8266 ESP-12F)
 * - 0.96" OLED Display (SSD1306, I2C)
 * - 3x Tactile Buttons
 * - 3x Status LEDs (Green, Yellow, Red)
 *
 * Author: Created with Claude Code
 * Date: 2025-10-22
 */

#include <Wire.h>
#include "config.h"
#include "secrets.h"
#include "TimerManager.h"
#include "ButtonHandler.h"
#include "DisplayManager.h"
#include "WiFiManager.h"
#include "LEDController.h"
#include "Storage.h"

// Global instances
TimerManager timerManager;
ButtonHandler buttonHandler;
DisplayManager displayManager;
WiFiManager wifiManager;
LEDController ledController;
Storage storage;

// State tracking
unsigned long lastEEPROMSave = 0;
unsigned long nightModeWakeUntil = 0;
bool temporaryWake = false;
unsigned long lastRedNotificationTime = 0;
unsigned long lastYellowNotificationTime = 0;
bool redLEDWasOn = false;
bool yellowLEDWasOn = false;
bool redAlertActive = false;
bool wasInNightMode = false;
bool startupNotificationSent = false;

// Runtime LED thresholds (can be modified via Telegram commands)
unsigned int yellowThreshold = YELLOW_THRESHOLD;
unsigned int redThreshold = RED_THRESHOLD;

// Function prototypes
void onButtonShortPress(Button button);
bool isNightMode();
bool isQuietHours();
void handleNightMode();
void saveToEEPROM();
void checkAndSendNotification();
void sendStartupNotification();
void handleTelegramCommand(String chatId, String command);

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  delay(100);
  DEBUG_PRINTLN("\n\n=== Dog Potty Tracker ===");
  DEBUG_PRINTLN("Initializing...\n");

  // Initialize storage
  storage.begin();

  // Initialize display
  DEBUG_PRINTLN("About to initialize display...");
  if (!displayManager.begin()) {
    DEBUG_PRINTLN("ERROR: Display initialization failed!");
    // Continue anyway - device can still function
  }
  DEBUG_PRINTLN("Display initialization attempt complete");

  // Configure display mode
  displayManager.setDisplayMode(DISPLAY_MODE, DISPLAY_CYCLE_SECONDS);

  // Show startup message
  DEBUG_PRINTLN("Showing startup message...");
  displayManager.showStartup();
  DEBUG_PRINTLN("Startup message complete");

  // Initialize button handler
  buttonHandler.begin();
  buttonHandler.setCallback(BTN_OUTSIDE, onButtonShortPress);
  buttonHandler.setCallback(BTN_PEE, onButtonShortPress);
  buttonHandler.setCallback(BTN_POOP, onButtonShortPress);

  // Initialize LED controller
  ledController.begin();
  ledController.test();  // Test LEDs on startup

  // Load saved timer data from EEPROM
  if (storage.load(&timerManager)) {
    DEBUG_PRINTLN("Restored timer data from EEPROM");
    displayManager.showFeedback("Data Loaded", 1500);
  } else {
    DEBUG_PRINTLN("No valid saved data, starting fresh");
  }

  // Start WiFi connection (non-blocking)
  wifiManager.begin(WIFI_SSID, WIFI_PASSWORD);

  // Set up Telegram command handler
  wifiManager.setTelegramCommandCallback(handleTelegramCommand);

  DEBUG_PRINTLN("\nSetup complete!\n");
}

void loop() {
  unsigned long currentMillis = millis();

  // Update WiFi (handles reconnection)
  wifiManager.update();

  // Poll for incoming Telegram commands
  wifiManager.pollTelegramMessages(TELEGRAM_BOT_TOKEN_1, TELEGRAM_CHAT_ID_1,
                                   TELEGRAM_BOT_TOKEN_2, TELEGRAM_CHAT_ID_2,
                                   TELEGRAM_BOT_TOKEN_3, TELEGRAM_CHAT_ID_3);

  // Send startup notification once WiFi and time are ready
  if (!startupNotificationSent && wifiManager.isConnected() && wifiManager.isTimeSynced()) {
    sendStartupNotification();
    startupNotificationSent = true;
  }

  // Check for button presses
  buttonHandler.update();

  // Check night mode
  bool nightMode = isNightMode();

  // Check if we just exited night mode
  if (wasInNightMode && !nightMode) {
    // Just exited night mode - reset notification cooldown timers
    DEBUG_PRINTLN("Exited night mode - resetting notification timers");
    lastRedNotificationTime = millis();  // Prevent immediate red alert
    lastYellowNotificationTime = millis();  // Prevent immediate yellow alert
    redLEDWasOn = false;  // Reset LED tracking
    yellowLEDWasOn = false;  // Reset LED tracking
  }

  wasInNightMode = nightMode;  // Track for next loop iteration

  // Note: Night mode no longer turns off display or LEDs
  // It only suppresses notifications during quiet hours

  // Update LED status based on timers (always called, but LEDs are managed by nightMode flag internally)
  ledController.update(&timerManager);

  // Check if we should send notifications (yellow/red LED status changes)
  checkAndSendNotification();

  // Update display (handles view rotation)
  displayManager.update(&timerManager, wifiManager.isTimeSynced());

  // Periodic EEPROM save (every 5 minutes)
  if (currentMillis - lastEEPROMSave >= EEPROM_SAVE_INTERVAL) {
    saveToEEPROM();
    lastEEPROMSave = currentMillis;
  }

  // Small delay to prevent overwhelming the loop
  delay(10);
}

void onButtonShortPress(Button button) {
  DEBUG_PRINT("Short press: ");
  DEBUG_PRINTLN(button);

  // Process button action
  switch (button) {
    case BTN_OUTSIDE:
      timerManager.resetOutside();
      displayManager.showFeedback("Outside!", 1500);
      break;

    case BTN_PEE:
      timerManager.resetPee();
      displayManager.showFeedback("Pee!", 1500);
      break;

    case BTN_POOP:
      timerManager.resetPoop();
      displayManager.showFeedback("Poop!", 1500);
      break;
  }

  // Save to EEPROM immediately after button press
  saveToEEPROM();
}

bool isNightMode() {
  // Check if night mode is disabled
  if (NIGHT_MODE_START_HOUR == -1 || NIGHT_MODE_END_HOUR == -1) {
    return false;
  }

  // Only enable night mode if time is synced
  if (!wifiManager.isTimeSynced()) {
    return false;
  }

  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  int hour = t->tm_hour;

  // Check if current hour is within night mode range
  return (hour >= NIGHT_MODE_START_HOUR || hour < NIGHT_MODE_END_HOUR);
}

bool isQuietHours() {
  // Only check quiet hours if time is synced
  if (!wifiManager.isTimeSynced()) {
    return false;
  }

  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  int hour = t->tm_hour;

  // Quiet hours: 10pm (22) to 7am
  return (hour >= NOTIFICATION_QUIET_START_HOUR || hour < NOTIFICATION_QUIET_END_HOUR);
}

void handleNightMode() {
  // Check if temporary wake has expired
  if (temporaryWake && millis() > nightModeWakeUntil) {
    temporaryWake = false;
    displayManager.setDisplayOn(false);
    ledController.setNightMode(true);
    DEBUG_PRINTLN("Night mode: Display sleep");
  }

  // If not temporarily awake, ensure display and LEDs are off
  if (!temporaryWake) {
    displayManager.setNightMode(true);
    ledController.setNightMode(true);
  }
}

void saveToEEPROM() {
  storage.save(&timerManager);
}

void checkAndSendNotification() {
  // Don't send notifications during quiet hours (10pm-7am)
  if (isQuietHours()) {
    DEBUG_PRINTLN("Quiet hours active - notifications suppressed");
    return;
  }

  // Get pee timer in minutes
  unsigned long peeMinutes = timerManager.getElapsed(TIMER_PEE) / 60;
  bool yellowLEDIsOn = (peeMinutes > yellowThreshold);
  bool redLEDIsOn = (peeMinutes > redThreshold);

  // Track if we sent any notifications this cycle
  int successCount = 0;
  String feedbackMessage = "";

  // === YELLOW LED NOTIFICATION (All users) ===
  if (yellowLEDIsOn && !yellowLEDWasOn) {
    // Yellow LED just turned on - send to all configured users
    unsigned long timeSinceLastYellowNotification = millis() - lastYellowNotificationTime;

    if (timeSinceLastYellowNotification >= TELEGRAM_NOTIFICATION_COOLDOWN || lastYellowNotificationTime == 0) {
      // Build notification message with actual time
      String message = String(DOG_NAME) + " should go out soon (last pee at ";
      message += timerManager.getTimestampFormatted(TIMER_PEE);
      message += ")";

      int yellowSuccessCount = 0;

      // Send to user 1
      if (strlen(TELEGRAM_BOT_TOKEN_1) > 0 && strlen(TELEGRAM_CHAT_ID_1) > 0) {
        DEBUG_PRINTLN("Sending yellow alert to user 1...");
        if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_1, TELEGRAM_CHAT_ID_1, message.c_str())) {
          yellowSuccessCount++;
        }
      }

      // Send to user 2
      if (strlen(TELEGRAM_BOT_TOKEN_2) > 0 && strlen(TELEGRAM_CHAT_ID_2) > 0) {
        DEBUG_PRINTLN("Sending yellow alert to user 2...");
        if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_2, TELEGRAM_CHAT_ID_2, message.c_str())) {
          yellowSuccessCount++;
        }
      }

      // Send to user 3
      if (strlen(TELEGRAM_BOT_TOKEN_3) > 0 && strlen(TELEGRAM_CHAT_ID_3) > 0) {
        DEBUG_PRINTLN("Sending yellow alert to user 3...");
        if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_3, TELEGRAM_CHAT_ID_3, message.c_str())) {
          yellowSuccessCount++;
        }
      }

      // Trigger Voice Monkey (Alexa) yellow alert if configured
      if (strlen(VOICE_MONKEY_TOKEN) > 0 && strlen(VOICE_MONKEY_DEVICE_YELLOW) > 0) {
        DEBUG_PRINTLN("Triggering Voice Monkey yellow alert...");
        wifiManager.triggerVoiceMonkey(VOICE_MONKEY_TOKEN, VOICE_MONKEY_DEVICE_YELLOW);
      }

      // Update status
      if (yellowSuccessCount > 0) {
        lastYellowNotificationTime = millis();
        successCount += yellowSuccessCount;
        feedbackMessage = "Yellow Alert Sent (";
        feedbackMessage += yellowSuccessCount;
        feedbackMessage += ")";
        DEBUG_PRINT("Yellow notifications sent successfully to ");
        DEBUG_PRINT(yellowSuccessCount);
        DEBUG_PRINTLN(" recipient(s)!");
      }
    } else {
      DEBUG_PRINTLN("Yellow notification cooldown active - skipping");
    }
  }

  // === RED LED NOTIFICATION (All users) ===
  if (redLEDIsOn && !redLEDWasOn) {
    // Red LED just turned on - send to all configured users
    unsigned long timeSinceLastRedNotification = millis() - lastRedNotificationTime;

    if (timeSinceLastRedNotification >= TELEGRAM_NOTIFICATION_COOLDOWN || lastRedNotificationTime == 0) {
      // Build notification message with actual time
      String message = String(DOG_NAME) + " needs to pee NOW! (last pee at ";
      message += timerManager.getTimestampFormatted(TIMER_PEE);
      message += ")";

      int redSuccessCount = 0;

      // Send to user 1
      if (strlen(TELEGRAM_BOT_TOKEN_1) > 0 && strlen(TELEGRAM_CHAT_ID_1) > 0) {
        DEBUG_PRINTLN("Sending red alert to user 1...");
        if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_1, TELEGRAM_CHAT_ID_1, message.c_str())) {
          redSuccessCount++;
        }
      }

      // Send to user 2
      if (strlen(TELEGRAM_BOT_TOKEN_2) > 0 && strlen(TELEGRAM_CHAT_ID_2) > 0) {
        DEBUG_PRINTLN("Sending red alert to user 2...");
        if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_2, TELEGRAM_CHAT_ID_2, message.c_str())) {
          redSuccessCount++;
        }
      }

      // Send to user 3
      if (strlen(TELEGRAM_BOT_TOKEN_3) > 0 && strlen(TELEGRAM_CHAT_ID_3) > 0) {
        DEBUG_PRINTLN("Sending red alert to user 3...");
        if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_3, TELEGRAM_CHAT_ID_3, message.c_str())) {
          redSuccessCount++;
        }
      }

      // Trigger Voice Monkey (Alexa) red alert if configured
      if (strlen(VOICE_MONKEY_TOKEN) > 0 && strlen(VOICE_MONKEY_DEVICE_RED) > 0) {
        DEBUG_PRINTLN("Triggering Voice Monkey red alert...");
        wifiManager.triggerVoiceMonkey(VOICE_MONKEY_TOKEN, VOICE_MONKEY_DEVICE_RED);
      }

      // Update status
      if (redSuccessCount > 0) {
        lastRedNotificationTime = millis();
        redAlertActive = true;  // Track that red alert is active
        successCount += redSuccessCount;
        feedbackMessage = "Red Alert Sent (";
        feedbackMessage += redSuccessCount;
        feedbackMessage += ")";
        DEBUG_PRINT("Red notifications sent successfully to ");
        DEBUG_PRINT(redSuccessCount);
        DEBUG_PRINTLN(" recipient(s)!");
      }
    } else {
      DEBUG_PRINTLN("Red notification cooldown active - skipping");
    }
  }

  // === RED LED TURNED OFF (All Clear notification) ===
  if (!redLEDIsOn && redLEDWasOn && redAlertActive) {
    // Red LED just turned off after being on - send "all clear" to all users
    String message = "All clear! " + String(DOG_NAME) + " has peed.";

    int clearSuccessCount = 0;

    // Send to user 1
    if (strlen(TELEGRAM_BOT_TOKEN_1) > 0 && strlen(TELEGRAM_CHAT_ID_1) > 0) {
      DEBUG_PRINTLN("Sending all clear to user 1...");
      if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_1, TELEGRAM_CHAT_ID_1, message.c_str())) {
        clearSuccessCount++;
      }
    }

    // Send to user 2
    if (strlen(TELEGRAM_BOT_TOKEN_2) > 0 && strlen(TELEGRAM_CHAT_ID_2) > 0) {
      DEBUG_PRINTLN("Sending all clear to user 2...");
      if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_2, TELEGRAM_CHAT_ID_2, message.c_str())) {
        clearSuccessCount++;
      }
    }

    // Send to user 3
    if (strlen(TELEGRAM_BOT_TOKEN_3) > 0 && strlen(TELEGRAM_CHAT_ID_3) > 0) {
      DEBUG_PRINTLN("Sending all clear to user 3...");
      if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_3, TELEGRAM_CHAT_ID_3, message.c_str())) {
        clearSuccessCount++;
      }
    }

    // Update status
    if (clearSuccessCount > 0) {
      redAlertActive = false;  // Clear the alert state
      successCount += clearSuccessCount;
      feedbackMessage = "All Clear Sent (";
      feedbackMessage += clearSuccessCount;
      feedbackMessage += ")";
      DEBUG_PRINT("All clear notifications sent successfully to ");
      DEBUG_PRINT(clearSuccessCount);
      DEBUG_PRINTLN(" recipient(s)!");
    }
  }

  // Show feedback on display if any notifications were sent
  if (successCount > 0 && feedbackMessage.length() > 0) {
    displayManager.showFeedback(feedbackMessage.c_str(), 2000);
  }

  // Update LED states for next iteration
  yellowLEDWasOn = yellowLEDIsOn;
  redLEDWasOn = redLEDIsOn;
}

void sendStartupNotification() {
  DEBUG_PRINTLN("Sending startup notifications...");

  String message = String(DOG_NAME) + " tracker is online!";
  int successCount = 0;

  // Send to all configured Telegram users
  if (strlen(TELEGRAM_BOT_TOKEN_1) > 0 && strlen(TELEGRAM_CHAT_ID_1) > 0) {
    DEBUG_PRINTLN("Sending startup notification to Telegram user 1...");
    if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_1, TELEGRAM_CHAT_ID_1, message.c_str())) {
      successCount++;
    }
  }

  if (strlen(TELEGRAM_BOT_TOKEN_2) > 0 && strlen(TELEGRAM_CHAT_ID_2) > 0) {
    DEBUG_PRINTLN("Sending startup notification to Telegram user 2...");
    if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_2, TELEGRAM_CHAT_ID_2, message.c_str())) {
      successCount++;
    }
  }

  if (strlen(TELEGRAM_BOT_TOKEN_3) > 0 && strlen(TELEGRAM_CHAT_ID_3) > 0) {
    DEBUG_PRINTLN("Sending startup notification to Telegram user 3...");
    if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_3, TELEGRAM_CHAT_ID_3, message.c_str())) {
      successCount++;
    }
  }

  // Trigger Voice Monkey (Alexa) startup alert if configured
  if (strlen(VOICE_MONKEY_TOKEN) > 0 && strlen(VOICE_MONKEY_DEVICE_STARTUP) > 0) {
    DEBUG_PRINTLN("Triggering Voice Monkey startup alert...");
    wifiManager.triggerVoiceMonkey(VOICE_MONKEY_TOKEN, VOICE_MONKEY_DEVICE_STARTUP);
  } else {
    DEBUG_PRINTLN("Voice Monkey not configured - skipping startup alert");
  }

  // Show feedback on display
  if (successCount > 0) {
    String feedbackMessage = "Startup Sent (";
    feedbackMessage += successCount;
    feedbackMessage += ")";
    displayManager.showFeedback(feedbackMessage.c_str(), 2000);
    DEBUG_PRINT("Startup notifications sent successfully to ");
    DEBUG_PRINT(successCount);
    DEBUG_PRINTLN(" recipient(s)!");
  } else {
    DEBUG_PRINTLN("No notification recipients configured");
  }
}

void handleTelegramCommand(String chatId, String command) {
  DEBUG_PRINT("Handling Telegram command from ");
  DEBUG_PRINT(chatId);
  DEBUG_PRINT(": ");
  DEBUG_PRINTLN(command);

  // Convert command to lowercase for case-insensitive matching
  command.toLowerCase();

  // Remove any bot username suffix (e.g., /pee@yourbot -> /pee)
  int atPos = command.indexOf('@');
  if (atPos > 0) {
    command = command.substring(0, atPos);
  }

  // Remove leading slash if present (accept both "/pee" and "pee")
  if (command.startsWith("/")) {
    command = command.substring(1);
  }

  String response = "";
  bool commandRecognized = false;

  // Handle commands (without slash)
  if (command == "pee") {
    timerManager.resetPee();
    displayManager.showFeedback("Pee! (Remote)", 1500);
    saveToEEPROM();
    response = "Pee timer reset!";
    commandRecognized = true;
    DEBUG_PRINTLN("Remote pee command executed");
  }
  else if (command == "poo" || command == "poop") {
    timerManager.resetPoop();
    displayManager.showFeedback("Poop! (Remote)", 1500);
    saveToEEPROM();
    response = "Poop timer reset!";
    commandRecognized = true;
    DEBUG_PRINTLN("Remote poop command executed");
  }
  else if (command == "out" || command == "outside") {
    timerManager.resetOutside();
    displayManager.showFeedback("Outside! (Remote)", 1500);
    saveToEEPROM();
    response = "Outside timer reset!";
    commandRecognized = true;
    DEBUG_PRINTLN("Remote outside command executed");
  }
  // Note: /status command removed - cannot send replies on ESP8266 due to SSL limitations
  // For status with replies, upgrade to Raspberry Pi Pico W (see micropython-pico-w branch)
  else if (command.startsWith("setpee ")) {
    // Format: setpee 90 (minutes ago)
    int minutes = command.substring(7).toInt();
    if (minutes > 0) {
      time_t now = time(nullptr);
      time_t targetTime = now - (minutes * 60);
      timerManager.setTimestamp(TIMER_PEE, targetTime);
      saveToEEPROM();
      response = "Pee timer set to " + String(minutes) + " minutes ago";
      displayManager.showFeedback("Pee Set (Remote)", 1500);
      commandRecognized = true;
      DEBUG_PRINT("Pee timer manually set to ");
      DEBUG_PRINT(minutes);
      DEBUG_PRINTLN(" minutes ago");
    } else {
      response = "Invalid format. Use: setpee <minutes>\nExample: setpee 90";
    }
  }
  else if (command.startsWith("setpoo ") || command.startsWith("setpoop ")) {
    // Format: setpoo 120 (minutes ago)
    int startPos = command.startsWith("setpoo ") ? 7 : 8;
    int minutes = command.substring(startPos).toInt();
    if (minutes > 0) {
      time_t now = time(nullptr);
      time_t targetTime = now - (minutes * 60);
      timerManager.setTimestamp(TIMER_POOP, targetTime);
      saveToEEPROM();
      response = "Poop timer set to " + String(minutes) + " minutes ago";
      displayManager.showFeedback("Poop Set (Remote)", 1500);
      commandRecognized = true;
      DEBUG_PRINT("Poop timer manually set to ");
      DEBUG_PRINT(minutes);
      DEBUG_PRINTLN(" minutes ago");
    } else {
      response = "Invalid format. Use: setpoo <minutes>\nExample: setpoo 120";
    }
  }
  else if (command.startsWith("setout ") || command.startsWith("setoutside ")) {
    // Format: setout 45 (minutes ago)
    int startPos = command.startsWith("setout ") ? 7 : 11;
    int minutes = command.substring(startPos).toInt();
    if (minutes > 0) {
      time_t now = time(nullptr);
      time_t targetTime = now - (minutes * 60);
      timerManager.setTimestamp(TIMER_OUTSIDE, targetTime);
      saveToEEPROM();
      response = "Outside timer set to " + String(minutes) + " minutes ago";
      displayManager.showFeedback("Outside Set (Remote)", 1500);
      commandRecognized = true;
      DEBUG_PRINT("Outside timer manually set to ");
      DEBUG_PRINT(minutes);
      DEBUG_PRINTLN(" minutes ago");
    } else {
      response = "Invalid format. Use: setout <minutes>\nExample: setout 45";
    }
  }
  else if (command.startsWith("setall ")) {
    // Format: setall 60 (sets all timers to 60 minutes ago)
    int minutes = command.substring(7).toInt();
    if (minutes > 0) {
      time_t now = time(nullptr);
      time_t targetTime = now - (minutes * 60);
      timerManager.setTimestamp(TIMER_OUTSIDE, targetTime);
      timerManager.setTimestamp(TIMER_PEE, targetTime);
      timerManager.setTimestamp(TIMER_POOP, targetTime);
      saveToEEPROM();
      response = "All timers set to " + String(minutes) + " minutes ago";
      displayManager.showFeedback("All Set (Remote)", 1500);
      commandRecognized = true;
      DEBUG_PRINT("All timers manually set to ");
      DEBUG_PRINT(minutes);
      DEBUG_PRINTLN(" minutes ago");
    } else {
      response = "Invalid format. Use: setall <minutes>\nExample: setall 60";
    }
  }
  else if (command.startsWith("setyellow ")) {
    // Format: setyellow 150 (sets yellow threshold to 150 minutes)
    int minutes = command.substring(10).toInt();
    if (minutes > 0 && minutes < 1440) {  // Max 24 hours
      yellowThreshold = minutes;
      response = "Yellow alert threshold set to " + String(minutes) + " minutes (until reboot)";
      displayManager.showFeedback("Yellow Set", 1500);
      commandRecognized = true;
      DEBUG_PRINT("Yellow threshold set to ");
      DEBUG_PRINT(minutes);
      DEBUG_PRINTLN(" minutes");
    } else {
      response = "Invalid format. Use: setyellow <minutes> (1-1439)\nExample: setyellow 150";
    }
  }
  else if (command.startsWith("setred ")) {
    // Format: setred 240 (sets red threshold to 240 minutes)
    int minutes = command.substring(7).toInt();
    if (minutes > 0 && minutes < 1440) {  // Max 24 hours
      redThreshold = minutes;
      response = "Red alert threshold set to " + String(minutes) + " minutes (until reboot)";
      displayManager.showFeedback("Red Set", 1500);
      commandRecognized = true;
      DEBUG_PRINT("Red threshold set to ");
      DEBUG_PRINT(minutes);
      DEBUG_PRINTLN(" minutes");
    } else {
      response = "Invalid format. Use: setred <minutes> (1-1439)\nExample: setred 240";
    }
  }
  // Note: /help command removed - set up commands via @BotFather instead (see secrets.h.example)
  // This avoids SSL connection failures and provides better UI in Telegram

  // REPLIES DISABLED: ESP8266 hardware limitation
  // The ESP8266 has only ~11KB free heap and cannot handle HTTPS polling + replies
  // Even with 5+ second delays, the SSL/TLS stack remains exhausted (heap doesn't recover)
  // Commands still execute successfully - verify via:
  //   1. Display shows "Pee! (Remote)" or similar feedback
  //   2. Timer values update on screen
  //   3. EEPROM saves (check serial output)
  //
  // Alternative solutions:
  //   - Upgrade to ESP32 (32KB+ heap, better SSL stack)
  //   - Raspberry Pi Pico W (much more memory)
  //   - Disable Telegram polling and only send alerts (no remote commands)
  //   - Use MQTT instead of HTTPS for bidirectional communication

  DEBUG_PRINT("Command executed successfully: ");
  DEBUG_PRINTLN(response);
  DEBUG_PRINTLN("(Reply disabled - ESP8266 SSL limitation)");
}
