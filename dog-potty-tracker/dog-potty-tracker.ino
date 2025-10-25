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

// Function prototypes
void onButtonShortPress(Button button);
bool isNightMode();
bool isQuietHours();
void handleNightMode();
void saveToEEPROM();
void checkAndSendNotification();
void sendStartupNotification();

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

  DEBUG_PRINTLN("\nSetup complete!\n");
}

void loop() {
  unsigned long currentMillis = millis();

  // Update WiFi (handles reconnection)
  wifiManager.update();

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

  if (nightMode) {
    handleNightMode();
  } else {
    // Normal operation - ensure display and LEDs are on
    if (temporaryWake) {
      temporaryWake = false;
    }
    displayManager.setNightMode(false);
    ledController.setNightMode(false);
  }

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

  // Handle night mode wake
  bool nightMode = isNightMode();
  if (nightMode) {
    // Wake display for 10 seconds
    temporaryWake = true;
    nightModeWakeUntil = millis() + NIGHT_MODE_WAKE_DURATION;
    displayManager.setDisplayOn(true);
    ledController.setNightMode(false);  // Show LEDs briefly
  }

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
  bool yellowLEDIsOn = (peeMinutes > 90);   // Yellow LED turns on at 90 minutes
  bool redLEDIsOn = (peeMinutes > 180);     // Red LED turns on at 3 hours (180 minutes)

  // Track if we sent any notifications this cycle
  int successCount = 0;
  String feedbackMessage = "";

  // === YELLOW LED NOTIFICATION (All users) ===
  if (yellowLEDIsOn && !yellowLEDWasOn) {
    // Yellow LED just turned on - send to all configured users
    unsigned long timeSinceLastYellowNotification = millis() - lastYellowNotificationTime;

    if (timeSinceLastYellowNotification >= TELEGRAM_NOTIFICATION_COOLDOWN || lastYellowNotificationTime == 0) {
      // Build notification message
      String message = String(DOG_NAME) + " should go out soon (";
      message += peeMinutes / 60;
      message += "h ";
      message += peeMinutes % 60;
      message += "m since last pee)";

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

      // Send to Alexa via Voice Monkey (if configured)
      if (strlen(VOICE_MONKEY_TOKEN) > 0 && strlen(VOICE_MONKEY_YELLOW) > 0) {
        DEBUG_PRINTLN("Triggering yellow alert on Alexa...");
        if (wifiManager.triggerVoiceMonkey(VOICE_MONKEY_TOKEN, VOICE_MONKEY_YELLOW)) {
          DEBUG_PRINTLN("Alexa yellow routine triggered successfully!");
        }
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
      // Build notification message
      String message = String(DOG_NAME) + " needs to pee NOW! (";
      message += peeMinutes / 60;
      message += "h ";
      message += peeMinutes % 60;
      message += "m since last pee)";

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

      // Send to Alexa via Voice Monkey (if configured)
      if (strlen(VOICE_MONKEY_TOKEN) > 0 && strlen(VOICE_MONKEY_RED) > 0) {
        DEBUG_PRINTLN("Triggering red alert on Alexa...");
        if (wifiManager.triggerVoiceMonkey(VOICE_MONKEY_TOKEN, VOICE_MONKEY_RED)) {
          DEBUG_PRINTLN("Alexa red routine triggered successfully!");
        }
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

  // Send to Alexa via Voice Monkey (if configured)
  if (strlen(VOICE_MONKEY_TOKEN) > 0 && strlen(VOICE_MONKEY_STARTUP) > 0) {
    DEBUG_PRINTLN("Triggering startup announcement on Alexa...");
    if (wifiManager.triggerVoiceMonkey(VOICE_MONKEY_TOKEN, VOICE_MONKEY_STARTUP)) {
      DEBUG_PRINTLN("Alexa startup routine triggered successfully!");
    }
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
