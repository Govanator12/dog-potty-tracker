#include "DisplayManager.h"

DisplayManager::DisplayManager() :
  display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
  currentView(VIEW_ELAPSED),
  lastViewSwitch(0),
  feedbackUntil(0),
  showingFeedback(false),
  nightMode(false),
  displayOn(true)
{
}

bool DisplayManager::begin() {
  // Initialize I2C with custom pins
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    DEBUG_PRINTLN("SSD1306 allocation failed");
    return false;
  }

  DEBUG_PRINTLN("DisplayManager initialized");

  // Clear display
  display.clearDisplay();
  display.display();

  return true;
}

void DisplayManager::update(TimerManager* timerManager, bool timeSynced) {
  // Check if we should stop showing feedback
  if (showingFeedback && millis() > feedbackUntil) {
    showingFeedback = false;
  }

  // If showing feedback, render that instead
  if (showingFeedback) {
    renderFeedback();
    return;
  }

  // Don't update display if in night mode and display is off
  if (nightMode && !displayOn) {
    return;
  }

  // Rotate view every VIEW_ROTATION_INTERVAL
  rotateView(timeSynced);

  // Render current view
  if (currentView == VIEW_ELAPSED || !timeSynced) {
    renderElapsedView(timerManager);
  } else {
    renderTimestampView(timerManager);
  }
}

void DisplayManager::rotateView(bool timeSynced) {
  // Only rotate if time is synced (otherwise always show elapsed)
  if (!timeSynced) {
    currentView = VIEW_ELAPSED;
    return;
  }

  if (millis() - lastViewSwitch >= VIEW_ROTATION_INTERVAL) {
    // Toggle view
    currentView = (currentView == VIEW_ELAPSED) ? VIEW_TIMESTAMP : VIEW_ELAPSED;
    lastViewSwitch = millis();
    DEBUG_PRINT("View switched to: ");
    DEBUG_PRINTLN(currentView == VIEW_ELAPSED ? "ELAPSED" : "TIMESTAMP");
  }
}

void DisplayManager::renderElapsedView(TimerManager* timerManager) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Line 1: Outside timer (Yellow section - top 16px)
  display.setCursor(0, 0);
  display.print("OUT: ");
  display.print(timerManager->getElapsedFormatted(TIMER_OUTSIDE));

  // Line 2: Pee timer
  display.setCursor(0, 16);
  display.print("PEE: ");
  display.print(timerManager->getElapsedFormatted(TIMER_PEE));

  // Line 3: Poop timer (Blue section - bottom 48px)
  display.setCursor(0, 32);
  display.print("POO: ");
  display.print(timerManager->getElapsedFormatted(TIMER_POOP));

  // Line 4: Current time or status
  display.setCursor(0, 48);
  display.print("[");
  display.print(getCurrentTimeString());
  display.print("]");

  display.display();
}

void DisplayManager::renderTimestampView(TimerManager* timerManager) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Line 1: Outside timestamp (Yellow section)
  display.setCursor(0, 0);
  display.print("OUT: ");
  display.print(timerManager->getTimestampFormatted(TIMER_OUTSIDE));

  // Line 2: Pee timestamp
  display.setCursor(0, 16);
  display.print("PEE: ");
  display.print(timerManager->getTimestampFormatted(TIMER_PEE));

  // Line 3: Poop timestamp (Blue section)
  display.setCursor(0, 32);
  display.print("POO: ");
  display.print(timerManager->getTimestampFormatted(TIMER_POOP));

  // Line 4: Current time
  display.setCursor(0, 48);
  display.print("[");
  display.print(getCurrentTimeString());
  display.print("]");

  display.display();
}

void DisplayManager::renderFeedback() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  // Center the feedback message
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(feedbackMessage.c_str(), 0, 0, &x1, &y1, &w, &h);

  int x = (SCREEN_WIDTH - w) / 2;
  int y = (SCREEN_HEIGHT - h) / 2;

  display.setCursor(x, y);
  display.print(feedbackMessage);

  display.display();
}

String DisplayManager::getCurrentTimeString() {
  time_t now = time(nullptr);

  // Check if time is synced
  if (now < 1000000000) {
    return "No WiFi";
  }

  struct tm* timeinfo = localtime(&now);

  // Format as 12-hour time with AM/PM
  int hour = timeinfo->tm_hour;
  bool isPM = hour >= 12;
  if (hour > 12) hour -= 12;
  if (hour == 0) hour = 12;

  char buffer[10];
  snprintf(buffer, sizeof(buffer), "%d:%02d %s",
           hour, timeinfo->tm_min, isPM ? "PM" : "AM");

  return String(buffer);
}

void DisplayManager::showStartup() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("Dog Potty");
  display.println("Tracker");

  display.setTextSize(1);
  display.setCursor(0, 48);
  display.print("Starting...");

  display.display();
  delay(2000);
}

void DisplayManager::showFeedback(const char* message, unsigned long duration) {
  feedbackMessage = String(message);
  feedbackUntil = millis() + duration;
  showingFeedback = true;
  DEBUG_PRINT("Feedback: ");
  DEBUG_PRINTLN(message);
}

void DisplayManager::setNightMode(bool enabled) {
  // Only change state and log if actually changing
  if (nightMode != enabled) {
    nightMode = enabled;
    if (enabled && displayOn) {
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      DEBUG_PRINTLN("Display: Night mode ON");
    } else if (!enabled && !displayOn) {
      display.ssd1306_command(SSD1306_DISPLAYON);
      displayOn = true;
      DEBUG_PRINTLN("Display: Night mode OFF");
    }
  } else {
    nightMode = enabled;
  }
}

void DisplayManager::setDisplayOn(bool on) {
  if (on && !displayOn) {
    display.ssd1306_command(SSD1306_DISPLAYON);
    displayOn = true;
    DEBUG_PRINTLN("Display: ON");
  } else if (!on && displayOn) {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    displayOn = false;
    DEBUG_PRINTLN("Display: OFF");
  }
}
