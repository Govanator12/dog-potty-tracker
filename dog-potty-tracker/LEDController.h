#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include "config.h"
#include "TimerManager.h"

enum LED {
  LED_GREEN_STATUS = 0,
  LED_YELLOW_STATUS = 1,
  LED_RED_STATUS = 2
};

class LEDController {
public:
  LEDController();

  // Initialize LED pins
  void begin();

  // Update LED status based on timer values
  void update(TimerManager* timerManager);

  // Set night mode (all LEDs off)
  void setNightMode(bool enabled);

  // Test all LEDs (startup sequence)
  void test();

private:
  bool nightMode;

  // Set individual LED state
  void setLED(LED led, bool state);

  // Evaluate timer status and set LEDs
  void evaluateStatus(TimerManager* timerManager);

  // Get GPIO pin for LED
  uint8_t getPin(LED led);
};

#endif
