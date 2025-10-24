#include "ButtonHandler.h"

ButtonHandler::ButtonHandler() {
  // Initialize arrays
  for (int i = 0; i < 3; i++) {
    buttonState[i] = false;
    lastButtonState[i] = false;
    lastDebounceTime[i] = 0;
    buttonPressTime[i] = 0;
    longPressFired[i] = false;
    shortPressCallback[i] = nullptr;
    longPressCallback[i] = nullptr;
  }
}

void ButtonHandler::begin() {
  // Set button pins as INPUT (with external pull-down resistors)
  pinMode(PIN_BTN_OUTSIDE, INPUT);
  pinMode(PIN_BTN_PEE, INPUT);
  pinMode(PIN_BTN_POOP, INPUT);

  DEBUG_PRINTLN("ButtonHandler initialized");
}

void ButtonHandler::update() {
  checkButton(BTN_OUTSIDE);
  checkButton(BTN_PEE);
  checkButton(BTN_POOP);
}

void ButtonHandler::checkButton(Button button) {
  uint8_t pin = getPin(button);
  bool reading = isPressed(pin);

  // Check if button state changed
  if (reading != lastButtonState[button]) {
    // Reset debounce timer
    lastDebounceTime[button] = millis();
  }

  // Check if debounce delay has passed
  if ((millis() - lastDebounceTime[button]) > DEBOUNCE_DELAY) {
    // If button state has changed after debounce
    if (reading != buttonState[button]) {
      buttonState[button] = reading;

      // Button just pressed
      if (buttonState[button]) {
        buttonPressTime[button] = millis();
        longPressFired[button] = false;
        DEBUG_PRINT("Button pressed: ");
        DEBUG_PRINTLN(button);
      }
      // Button just released
      else {
        unsigned long pressDuration = millis() - buttonPressTime[button];

        // If it was a short press (and long press wasn't fired)
        if (pressDuration < LONG_PRESS_DURATION && !longPressFired[button]) {
          if (shortPressCallback[button] != nullptr) {
            DEBUG_PRINT("Short press callback: ");
            DEBUG_PRINTLN(button);
            shortPressCallback[button](button);
          }
        }
      }
    }
  }

  // Check for long press while button is held
  if (buttonState[button] && !longPressFired[button]) {
    unsigned long pressDuration = millis() - buttonPressTime[button];
    if (pressDuration >= LONG_PRESS_DURATION) {
      longPressFired[button] = true;
      if (longPressCallback[button] != nullptr) {
        DEBUG_PRINT("Long press callback: ");
        DEBUG_PRINTLN(button);
        longPressCallback[button](button);
      }
    }
  }

  lastButtonState[button] = reading;
}

bool ButtonHandler::isPressed(uint8_t pin) {
  // Buttons are active HIGH (pressed = HIGH, released = LOW)
  return digitalRead(pin) == HIGH;
}

uint8_t ButtonHandler::getPin(Button button) {
  switch (button) {
    case BTN_OUTSIDE:
      return PIN_BTN_OUTSIDE;
    case BTN_PEE:
      return PIN_BTN_PEE;
    case BTN_POOP:
      return PIN_BTN_POOP;
    default:
      return PIN_BTN_OUTSIDE;
  }
}

void ButtonHandler::setCallback(Button button, ButtonCallback callback) {
  shortPressCallback[button] = callback;
}

void ButtonHandler::setLongPressCallback(Button button, ButtonCallback callback) {
  longPressCallback[button] = callback;
}
