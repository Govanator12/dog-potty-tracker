#include "DisplayManager.h"

DisplayManager::DisplayManager() :
  display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
  currentView(VIEW_ELAPSED),
  lastViewSwitch(0),
  feedbackUntil(0),
  showingFeedback(false),
  nightMode(false),
  displayOn(true),
  displayMode(2),  // Default to cycle mode
  cycleInterval(5000),  // Default to 5 seconds
  currentTimer(0)  // Start with first timer (Outside)
{
}

bool DisplayManager::begin() {
  // Initialize I2C with custom pins
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

  Serial.println("DisplayManager: Initializing I2C...");
  Serial.print("DisplayManager: SDA=GPIO");
  Serial.print(PIN_OLED_SDA);
  Serial.print(", SCL=GPIO");
  Serial.println(PIN_OLED_SCL);

  // Scan I2C bus to find devices
  Serial.println("DisplayManager: Scanning I2C bus...");
  byte error, address;
  int nDevices = 0;
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("DisplayManager: I2C device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }
  if (nDevices == 0) {
    Serial.println("DisplayManager: No I2C devices found!");
  } else {
    Serial.print("DisplayManager: Found ");
    Serial.print(nDevices);
    Serial.println(" I2C device(s)");
  }

  Serial.print("DisplayManager: Trying I2C address 0x");
  Serial.println(OLED_ADDRESS, HEX);

  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("DisplayManager: SSD1306 allocation FAILED!");
    Serial.println("DisplayManager: Check wiring and I2C address");
    Serial.println("DisplayManager: Try changing OLED_ADDRESS in config.h to 0x3D");
    return false;
  }

  // Rotate display 180 degrees
  display.setRotation(2);

  Serial.println("DisplayManager: Display initialized successfully!");

  // Clear display
  display.clearDisplay();
  display.display();

  return true;
}

void DisplayManager::setDisplayMode(int mode, float cycleSeconds) {
  displayMode = mode;
  cycleInterval = (unsigned long)(cycleSeconds * 1000.0);  // Convert seconds to milliseconds

  // Set initial view based on mode
  if (displayMode == 0) {
    currentView = VIEW_ELAPSED;
  } else if (displayMode == 1) {
    currentView = VIEW_TIMESTAMP;
  } else if (displayMode == 3) {
    // Mode 3: Initialize the timer rotation
    currentTimer = 0;
    lastViewSwitch = millis();  // Start the cycle timer now
  }

  DEBUG_PRINT("Display mode set to: ");
  DEBUG_PRINTLN(displayMode);
  DEBUG_PRINT("Cycle interval: ");
  DEBUG_PRINT(cycleSeconds);
  DEBUG_PRINTLN(" seconds");
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

  // Render current view based on display mode
  if (displayMode == 3) {
    // Mode 3: Show single timer with large text
    renderSingleTimerView(timerManager, currentTimer);
  } else if (currentView == VIEW_ELAPSED || !timeSynced) {
    renderElapsedView(timerManager);
  } else {
    renderTimestampView(timerManager);
  }
}

void DisplayManager::rotateView(bool timeSynced) {
  // Handle fixed display modes (0 = elapsed only, 1 = timestamps only)
  if (displayMode == 0) {
    currentView = VIEW_ELAPSED;
    return;
  } else if (displayMode == 1) {
    // Timestamps mode requires time sync
    if (!timeSynced) {
      currentView = VIEW_ELAPSED;  // Fall back to elapsed if no time sync
    } else {
      currentView = VIEW_TIMESTAMP;
    }
    return;
  } else if (displayMode == 3) {
    // Mode 3: Rotate through individual timers
    if (millis() - lastViewSwitch >= cycleInterval) {
      currentTimer = (currentTimer + 1) % 3;  // Cycle through 0, 1, 2 (Outside, Pee, Poop)
      lastViewSwitch = millis();
      DEBUG_PRINT("Timer switched to: ");
      DEBUG_PRINTLN(currentTimer == 0 ? "OUTSIDE" : (currentTimer == 1 ? "PEE" : "POOP"));
    }
    return;
  }

  // Mode 2: Cycle between views
  // Only rotate if time is synced (otherwise always show elapsed)
  if (!timeSynced) {
    currentView = VIEW_ELAPSED;
    return;
  }

  if (millis() - lastViewSwitch >= cycleInterval) {
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
  display.print(getCurrentTimeString());

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
  display.print(getCurrentTimeString());

  display.display();
}

void DisplayManager::renderSingleTimerView(TimerManager* timerManager, int timerIndex) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Determine which timer to show
  Timer timer;
  const char* label;

  switch(timerIndex) {
    case 0:
      timer = TIMER_OUTSIDE;
      label = "OUTSIDE";
      break;
    case 1:
      timer = TIMER_PEE;
      label = "PEE";
      break;
    case 2:
      timer = TIMER_POOP;
      label = "POOP";
      break;
    default:
      timer = TIMER_OUTSIDE;
      label = "OUTSIDE";
      break;
  }

  // Line 1: Timer label (size 1 - keep small so elapsed time can be huge)
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(label);

  // Get elapsed time string and remove " ago" suffix (last 4 characters)
  String elapsed = timerManager->getElapsedFormatted(timer);

  // Remove " ago" by truncating the string (remove last 4 characters)
  if (elapsed.endsWith(" ago")) {
    elapsed = elapsed.substring(0, elapsed.length() - 4);
  }

  // Line 2: Elapsed time (size 3 - EXTRA LARGE for easy reading from distance)
  display.setTextSize(3);
  display.setCursor(0, 12);
  display.print(elapsed);

  // Line 3: Timestamp (size 1 - small)
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("At: ");
  display.print(timerManager->getTimestampFormatted(timer));

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
      // Entering night mode - turn display off
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      displayOn = false;
      DEBUG_PRINTLN("Display: Night mode ON");
    } else if (!enabled) {
      // Exiting night mode - always turn display on
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
