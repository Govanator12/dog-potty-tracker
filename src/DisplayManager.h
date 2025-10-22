#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "TimerManager.h"

enum DisplayView {
  VIEW_ELAPSED = 0,
  VIEW_TIMESTAMP = 1
};

class DisplayManager {
public:
  DisplayManager();

  // Initialize display
  bool begin();

  // Update display (handles view rotation)
  void update(TimerManager* timerManager, bool timeSynced);

  // Show startup message
  void showStartup();

  // Show button feedback message
  void showFeedback(const char* message, unsigned long duration = 2000);

  // Set night mode (display off)
  void setNightMode(bool enabled);

  // Turn display on/off
  void setDisplayOn(bool on);

private:
  Adafruit_SSD1306 display;
  DisplayView currentView;
  unsigned long lastViewSwitch;
  unsigned long feedbackUntil;
  bool showingFeedback;
  bool nightMode;
  bool displayOn;
  String feedbackMessage;

  // Render views
  void renderElapsedView(TimerManager* timerManager);
  void renderTimestampView(TimerManager* timerManager);
  void renderFeedback();

  // Helper functions
  String getCurrentTimeString();
  void rotateView(bool timeSynced);
};

#endif
