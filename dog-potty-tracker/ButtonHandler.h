#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>
#include "config.h"

enum Button {
  BTN_OUTSIDE = 0,
  BTN_PEE = 1,
  BTN_POOP = 2
};

// Callback function types
typedef void (*ButtonCallback)(Button button);

class ButtonHandler {
public:
  ButtonHandler();

  // Initialize button pins
  void begin();

  // Update button states (call every loop iteration)
  void update();

  // Set callback for button press
  void setCallback(Button button, ButtonCallback callback);

private:
  // Button state tracking
  bool buttonState[3];
  bool lastButtonState[3];
  unsigned long lastDebounceTime[3];

  // Callbacks
  ButtonCallback callback[3];

  // Check individual button
  void checkButton(Button button);

  // Read button pin
  bool isPressed(uint8_t pin);

  // Get GPIO pin for button
  uint8_t getPin(Button button);
};

#endif
