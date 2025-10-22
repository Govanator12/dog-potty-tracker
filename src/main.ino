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

// Function prototypes
void onButtonShortPress(Button button);
void onButtonLongPress(Button button);
bool isNightMode();
void handleNightMode();
void saveToEEPROM();

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  delay(100);
  DEBUG_PRINTLN("\n\n=== Dog Potty Tracker ===");
  DEBUG_PRINTLN("Initializing...\n");

  // Initialize storage
  storage.begin();

  // Initialize display
  if (!displayManager.begin()) {
    DEBUG_PRINTLN("ERROR: Display initialization failed!");
    // Continue anyway - device can still function
  }

  // Show startup message
  displayManager.showStartup();

  // Initialize button handler
  buttonHandler.begin();
  buttonHandler.setCallback(BTN_OUTSIDE, onButtonShortPress);
  buttonHandler.setCallback(BTN_PEE, onButtonShortPress);
  buttonHandler.setCallback(BTN_POOP, onButtonShortPress);
  buttonHandler.setLongPressCallback(BTN_OUTSIDE, onButtonLongPress);
  buttonHandler.setLongPressCallback(BTN_PEE, onButtonLongPress);
  buttonHandler.setLongPressCallback(BTN_POOP, onButtonLongPress);

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

void onButtonLongPress(Button button) {
  DEBUG_PRINT("Long press: ");
  DEBUG_PRINTLN(button);

  // Reset only the specific timer
  switch (button) {
    case BTN_OUTSIDE:
      timerManager.reset(TIMER_OUTSIDE);
      displayManager.showFeedback("Reset Out", 1500);
      break;

    case BTN_PEE:
      timerManager.reset(TIMER_PEE);
      displayManager.showFeedback("Reset Pee", 1500);
      break;

    case BTN_POOP:
      timerManager.reset(TIMER_POOP);
      displayManager.showFeedback("Reset Poop", 1500);
      break;
  }

  // Save to EEPROM immediately after reset
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
