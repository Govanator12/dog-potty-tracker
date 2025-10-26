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

  // Set display mode configuration
  void setDisplayMode(int mode, float cycleSeconds);

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

  // Display mode configuration
  int displayMode;           // 0 = elapsed only, 1 = timestamps only, 2 = cycle, 3 = large rotating
  unsigned long cycleInterval;  // Milliseconds between view changes in cycle mode
  int currentTimer;          // For mode 3: which timer to show (0=outside, 1=pee, 2=poop)

  // Render views
  void renderElapsedView(TimerManager* timerManager);
  void renderTimestampView(TimerManager* timerManager);
  void renderSingleTimerView(TimerManager* timerManager, int timerIndex);
  void renderFeedback();

  // Helper functions
  String getCurrentTimeString();
  void rotateView(bool timeSynced);
};

#endif
