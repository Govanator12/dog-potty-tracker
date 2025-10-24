#include "ButtonHandler.h"

ButtonHandler::ButtonHandler() {
  // Initialize arrays
  for (int i = 0; i < 3; i++) {
    buttonState[i] = false;
    lastButtonState[i] = false;
    lastDebounceTime[i] = 0;
    callback[i] = nullptr;
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

      // Button just pressed (rising edge)
      if (buttonState[button]) {
        DEBUG_PRINT("Button pressed: ");
        DEBUG_PRINTLN(button);

        // Call the callback immediately on press
        if (callback[button] != nullptr) {
          callback[button](button);
        }
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

void ButtonHandler::setCallback(Button button, ButtonCallback cb) {
  callback[button] = cb;
}
