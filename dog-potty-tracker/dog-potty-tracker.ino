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

// Function prototypes
void onButtonShortPress(Button button);
bool isNightMode();
bool isQuietHours();
void handleNightMode();
void saveToEEPROM();
void checkAndSendNotification();

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

  // Check for button presses
  buttonHandler.update();

  // Check night mode
  bool nightMode = isNightMode();
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

  // Update LED status based on timers
  ledController.update(&timerManager);

  // Check if we should send IFTTT notification (red LED turned on)
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
  // Only enable night mode if time is synced
  if (!wifiManager.isTimeSynced()) {
    return false;
  }

  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  int hour = t->tm_hour;

  // Night mode: 11pm (23) to 5am
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

  // === YELLOW LED NOTIFICATION (Webhook 1 only) ===
  if (yellowLEDIsOn && !yellowLEDWasOn) {
    // Yellow LED just turned on - send to webhook 1 only
    unsigned long timeSinceLastYellowNotification = millis() - lastYellowNotificationTime;

    if (timeSinceLastYellowNotification >= TELEGRAM_NOTIFICATION_COOLDOWN || lastYellowNotificationTime == 0) {
      // Build notification message
      String message = "Fish should go out soon (";
      message += peeMinutes / 60;
      message += "h ";
      message += peeMinutes % 60;
      message += "m since last pee)";

      // Send to person 1 only (yellow notifications only go to you)
      if (strlen(TELEGRAM_BOT_TOKEN_1) > 0 && strlen(TELEGRAM_CHAT_ID_1) > 0) {
        DEBUG_PRINTLN("Sending yellow alert to person 1...");
        if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_1, TELEGRAM_CHAT_ID_1, message.c_str())) {
          successCount++;
          feedbackMessage = "Yellow Alert Sent";
          lastYellowNotificationTime = millis();
          DEBUG_PRINTLN("Yellow notification sent successfully!");
        }
      }

      // Send to Alexa via Notify Me (if configured)
      if (strlen(NOTIFY_ME_ACCESS_CODE) > 0) {
        DEBUG_PRINTLN("Sending yellow alert to Alexa...");
        if (wifiManager.sendNotifyMeNotification(NOTIFY_ME_ACCESS_CODE, "Fish should go outside soon.", ALEXA_DEVICE_NAME)) {
          DEBUG_PRINTLN("Alexa yellow notification sent successfully!");
        }
      }
    } else {
      DEBUG_PRINTLN("Yellow notification cooldown active - skipping");
    }
  }

  // === RED LED NOTIFICATION (All webhooks) ===
  if (redLEDIsOn && !redLEDWasOn) {
    // Red LED just turned on - send to all configured webhooks
    unsigned long timeSinceLastRedNotification = millis() - lastRedNotificationTime;

    if (timeSinceLastRedNotification >= TELEGRAM_NOTIFICATION_COOLDOWN || lastRedNotificationTime == 0) {
      // Build notification message
      String message = "Fish needs to pee NOW! (";
      message += peeMinutes / 60;
      message += "h ";
      message += peeMinutes % 60;
      message += "m since last pee)";

      int redSuccessCount = 0;

      // Send to person 1
      if (strlen(TELEGRAM_BOT_TOKEN_1) > 0 && strlen(TELEGRAM_CHAT_ID_1) > 0) {
        DEBUG_PRINTLN("Sending red alert to person 1...");
        if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_1, TELEGRAM_CHAT_ID_1, message.c_str())) {
          redSuccessCount++;
        }
      }

      // Send to person 2
      if (strlen(TELEGRAM_BOT_TOKEN_2) > 0 && strlen(TELEGRAM_CHAT_ID_2) > 0) {
        DEBUG_PRINTLN("Sending red alert to person 2...");
        if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_2, TELEGRAM_CHAT_ID_2, message.c_str())) {
          redSuccessCount++;
        }
      }

      // Send to person 3
      if (strlen(TELEGRAM_BOT_TOKEN_3) > 0 && strlen(TELEGRAM_CHAT_ID_3) > 0) {
        DEBUG_PRINTLN("Sending red alert to person 3...");
        if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_3, TELEGRAM_CHAT_ID_3, message.c_str())) {
          redSuccessCount++;
        }
      }

      // Send to Alexa via Notify Me (if configured)
      if (strlen(NOTIFY_ME_ACCESS_CODE) > 0) {
        DEBUG_PRINTLN("Sending red alert to Alexa...");
        if (wifiManager.sendNotifyMeNotification(NOTIFY_ME_ACCESS_CODE, "Fish needs to pee right now!", ALEXA_DEVICE_NAME)) {
          DEBUG_PRINTLN("Alexa red notification sent successfully!");
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
    // Red LED just turned off after being on - send "all clear" to all webhooks
    String message = "All clear! Fish has peed.";

    int clearSuccessCount = 0;

    // Send to person 1
    if (strlen(TELEGRAM_BOT_TOKEN_1) > 0 && strlen(TELEGRAM_CHAT_ID_1) > 0) {
      DEBUG_PRINTLN("Sending all clear to person 1...");
      if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_1, TELEGRAM_CHAT_ID_1, message.c_str())) {
        clearSuccessCount++;
      }
    }

    // Send to person 2
    if (strlen(TELEGRAM_BOT_TOKEN_2) > 0 && strlen(TELEGRAM_CHAT_ID_2) > 0) {
      DEBUG_PRINTLN("Sending all clear to person 2...");
      if (wifiManager.sendTelegramNotification(TELEGRAM_BOT_TOKEN_2, TELEGRAM_CHAT_ID_2, message.c_str())) {
        clearSuccessCount++;
      }
    }

    // Send to person 3
    if (strlen(TELEGRAM_BOT_TOKEN_3) > 0 && strlen(TELEGRAM_CHAT_ID_3) > 0) {
      DEBUG_PRINTLN("Sending all clear to person 3...");
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
