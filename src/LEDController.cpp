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
  // Get elapsed times in minutes
  unsigned long peeMinutes = timerManager->getElapsed(TIMER_PEE) / 60;
  unsigned long poopMinutes = timerManager->getElapsed(TIMER_POOP) / 60;

  // Check RED condition first (highest priority)
  // Red: Pee > 2.5hrs OR Poop > 5hrs
  if (peeMinutes > RED_PEE_THRESHOLD || poopMinutes > RED_POOP_THRESHOLD) {
    setLED(LED_RED_STATUS, HIGH);
    setLED(LED_YELLOW_STATUS, LOW);
    setLED(LED_GREEN_STATUS, LOW);
  }
  // Check YELLOW condition
  // Yellow: Pee > 90min OR Poop > 2.5hrs
  else if (peeMinutes > YELLOW_PEE_THRESHOLD || poopMinutes > YELLOW_POOP_THRESHOLD) {
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
  nightMode = enabled;
  if (enabled) {
    // Turn all LEDs off
    setLED(LED_GREEN_STATUS, LOW);
    setLED(LED_YELLOW_STATUS, LOW);
    setLED(LED_RED_STATUS, LOW);
    DEBUG_PRINTLN("LEDController: Night mode ON");
  } else {
    DEBUG_PRINTLN("LEDController: Night mode OFF");
  }
}

void LEDController::test() {
  DEBUG_PRINTLN("LEDController: Testing LEDs...");

  // Test each LED sequentially
  setLED(LED_GREEN_STATUS, HIGH);
  delay(300);
  setLED(LED_GREEN_STATUS, LOW);

  setLED(LED_YELLOW_STATUS, HIGH);
  delay(300);
  setLED(LED_YELLOW_STATUS, LOW);

  setLED(LED_RED_STATUS, HIGH);
  delay(300);
  setLED(LED_RED_STATUS, LOW);

  DEBUG_PRINTLN("LEDController: Test complete");
}
