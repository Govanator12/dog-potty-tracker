#include "LEDController.h"

LEDController::LEDController() : nightMode(false) {
}

void LEDController::begin() {
  // Set LED pins as OUTPUT
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  // Turn all LEDs off initially
  setLED(LED_GREEN_STATUS, LOW);
  setLED(LED_YELLOW_STATUS, LOW);
  setLED(LED_RED_STATUS, LOW);

  DEBUG_PRINTLN("LEDController initialized");
}

void LEDController::update(TimerManager* timerManager) {
  if (nightMode) {
    // All LEDs off during night mode
    setLED(LED_GREEN_STATUS, LOW);
    setLED(LED_YELLOW_STATUS, LOW);
    setLED(LED_RED_STATUS, LOW);
    return;
  }

  evaluateStatus(timerManager);
}

void LEDController::evaluateStatus(TimerManager* timerManager) {
  // Get elapsed time in minutes for pee timer only
  unsigned long peeMinutes = timerManager->getElapsed(TIMER_PEE) / 60;

  // Check RED condition first (highest priority)
  // Red: Pee > 3hrs (180 minutes)
  if (peeMinutes > 180) {
    setLED(LED_RED_STATUS, HIGH);
    setLED(LED_YELLOW_STATUS, LOW);
    setLED(LED_GREEN_STATUS, LOW);
  }
  // Check YELLOW condition
  // Yellow: Pee > 90min
  else if (peeMinutes > 90) {
    setLED(LED_RED_STATUS, LOW);
    setLED(LED_YELLOW_STATUS, HIGH);
    setLED(LED_GREEN_STATUS, LOW);
  }
  // Default: GREEN (all good)
  else {
    setLED(LED_RED_STATUS, LOW);
    setLED(LED_YELLOW_STATUS, LOW);
    setLED(LED_GREEN_STATUS, HIGH);
  }
}

void LEDController::setLED(LED led, bool state) {
  uint8_t pin = getPin(led);
  digitalWrite(pin, state ? HIGH : LOW);
}

uint8_t LEDController::getPin(LED led) {
  switch (led) {
    case LED_GREEN_STATUS:
      return PIN_LED_GREEN;
    case LED_YELLOW_STATUS:
      return PIN_LED_YELLOW;
    case LED_RED_STATUS:
      return PIN_LED_RED;
    default:
      return PIN_LED_GREEN;
  }
}

void LEDController::setNightMode(bool enabled) {
  // Only log if state is actually changing
  if (nightMode != enabled) {
    nightMode = enabled;
    if (enabled) {
      // Turn all LEDs off when entering night mode
      setLED(LED_GREEN_STATUS, LOW);
      setLED(LED_YELLOW_STATUS, LOW);
      setLED(LED_RED_STATUS, LOW);
      DEBUG_PRINTLN("LEDController: Night mode ON");
    } else {
      // Exiting night mode - LEDs will be restored on next update() call
      DEBUG_PRINTLN("LEDController: Night mode OFF");
    }
  } else {
    nightMode = enabled;
  }
}

void LEDController::test() {
  DEBUG_PRINTLN("LEDController: Testing LEDs...");

  // Test each LED sequentially
  DEBUG_PRINT("Testing GREEN LED on pin ");
  DEBUG_PRINTLN(PIN_LED_GREEN);
  setLED(LED_GREEN_STATUS, HIGH);
  delay(300);
  setLED(LED_GREEN_STATUS, LOW);

  DEBUG_PRINT("Testing YELLOW LED on pin ");
  DEBUG_PRINTLN(PIN_LED_YELLOW);
  setLED(LED_YELLOW_STATUS, HIGH);
  delay(300);
  setLED(LED_YELLOW_STATUS, LOW);

  DEBUG_PRINT("Testing RED LED on pin ");
  DEBUG_PRINTLN(PIN_LED_RED);
  setLED(LED_RED_STATUS, HIGH);
  delay(300);
  setLED(LED_RED_STATUS, LOW);

  DEBUG_PRINTLN("LEDController: Test complete");
}
