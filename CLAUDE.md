# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Dog Potty Tracker - A hardware device to track your dog's potty activities with three independent timers (Outside, Pee, Poop) displayed on an OLED screen with visual LED status indicators.

### Hardware Components

**Microcontroller:**
- WeMos D1 Mini (ESP8266 ESP-12F) - 16 pins, micro USB, WiFi-enabled
- Compact size: ~34mm x 26mm
- 4MB flash memory

**Display:**
- UCTRONICS 0.96" OLED Module (SSD1306 driver)
- Resolution: 128x64 pixels
- I2C interface (4 pins: VCC, GND, SCL, SDA)
- Yellow top section (16px) + Blue bottom section (48px)
- Voltage: 3.3V-5V DC

**Input:**
- 3x tactile push buttons (Outside, Pee, Poop)
- Standard momentary push buttons

**Output:**
- 3x LEDs (Green, Yellow, Red) for status indication
- Standard 5mm LEDs

**Resistors:**
- 3x 10k-ohm resistors (button pull-down)
- 3x 220-330 ohm resistors (LED current limiting)

**Optional:**
- Breadboard: 170-point minimum, 400-point recommended for comfortable working space
- Full 830-point breadboard is overkill but works fine

### Power Requirements

- USB powered (micro USB on WeMos D1 Mini)
- No battery operation
- Always-on device (~80mA draw)

## Pin Mapping

### WeMos D1 Mini Pinout

```
Left Side:              Right Side:
RST                     TX (GPIO1)
A0 (Analog)            RX (GPIO3)
D0 (GPIO16)            D1 (GPIO5) - I2C SCL
D5 (GPIO14)            D2 (GPIO4) - I2C SDA
D6 (GPIO12)            D3 (GPIO0)
D7 (GPIO13)            D4 (GPIO2) - Built-in LED (NOT USED)
D8 (GPIO15)            GND
3V3                    5V
```

### Project Pin Assignment

**OLED Display (I2C):**
- VCC -> 3V3
- GND -> GND
- SCL -> D1 (GPIO5)
- SDA -> D2 (GPIO4)

**Buttons (Active HIGH with pull-down resistors):**
- Button 1 (Outside) -> D5 (GPIO14)
- Button 2 (Pee) -> D6 (GPIO12)
- Button 3 (Poop) -> D7 (GPIO13)
- Each button: One leg to pin, other leg to 3V3
- 10k-ohm resistor from pin to GND (pull-down)

**Status LEDs (Active HIGH with current-limiting resistors):**
- LED Green -> D0 (GPIO16)
- LED Yellow -> D3 (GPIO0)
- LED Red -> D8 (GPIO15)
- Each LED: Anode to pin through 220-330 ohm resistor, cathode to GND

**Pins Used: 9 of 16**

**Available for Future Expansion:**
- A0 (Analog input)
- TX, RX (Serial, if not debugging)
- D4 (Built-in LED, intentionally unused per user request)

## Wiring Diagrams

### Breadboard Setup (400-point recommended)

**Power Rails:**
- Connect 3V3 pin to positive rail (red)
- Connect GND pin to negative rail (blue/black)
- Optionally connect 5V to separate positive rail if needed

**OLED Display:**
1. Insert OLED on left side of breadboard
2. VCC -> Positive rail (3V3)
3. GND -> Negative rail
4. SCL -> D1 (direct jumper wire)
5. SDA -> D2 (direct jumper wire)

**Buttons (for each of 3 buttons):**
1. Place button across center divider of breadboard
2. One leg -> Connect to 3V3 rail
3. Other leg -> Connect to D5/D6/D7 pin on WeMos
4. Same leg (to pin) -> Connect 10k-ohm resistor to GND rail
5. Result: Pull-down configuration, button press = HIGH signal

**LEDs (for each of 3 LEDs):**
1. Anode (long leg, +) -> Connect to 220-330 ohm resistor
2. Resistor other end -> Connect to D0/D3/D8 pin on WeMos
3. Cathode (short leg, -) -> Connect to GND rail

**WeMos D1 Mini Placement:**
- Straddle center divider of breadboard
- Ensures pins on both sides are accessible
- Takes ~15 rows of breadboard space

### Direct Wiring (No Breadboard)

**OLED to WeMos:**
- 4 direct jumper wires: VCC->3V3, GND->GND, SCL->D1, SDA->D2

**Each Button:**
- Solder or use female-to-male jumpers
- Button leg 1 -> 3V3 (may need to share 3V3 pin with wire splitter)
- Button leg 2 -> GPIO pin (D5/D6/D7)
- Resistor from GPIO pin to GND

**Each LED:**
- Solder resistor to LED anode
- Resistor -> GPIO pin (D0/D3/D8)
- LED cathode -> GND

**Note:** Direct wiring is more compact but harder to troubleshoot/modify. Breadboard recommended for initial development.

## Development Environment Setup

### Arduino IDE Installation

1. Download Arduino IDE 2.x from https://www.arduino.cc/en/software
2. Install and launch Arduino IDE

### ESP8266 Board Manager

1. Open Arduino IDE -> File -> Preferences
2. Add to "Additional Board Manager URLs":
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Go to Tools -> Board -> Boards Manager
4. Search "esp8266" and install "esp8266 by ESP8266 Community"
5. Select board: Tools -> Board -> ESP8266 Boards -> "LOLIN(WEMOS) D1 R2 & mini"

### Board Configuration

- **Board:** "LOLIN(WEMOS) D1 R2 & mini"
- **Upload Speed:** 921600 (fast) or 115200 (more reliable)
- **CPU Frequency:** 80 MHz (default) or 160 MHz (faster, more power)
- **Flash Size:** "4MB (FS:2MB OTA:~1019KB)"
- **Port:** Select the COM port where D1 Mini is connected

### USB Driver

If the board is not detected:
- WeMos D1 Mini uses **CH340** USB-to-serial chip
- Download driver from: https://www.wemos.cc/en/latest/ch340_driver.html
- Install and restart Arduino IDE

### Required Libraries

Install via Arduino IDE -> Tools -> Manage Libraries:

1. **Adafruit SSD1306** (by Adafruit) - OLED display driver
2. **Adafruit GFX Library** (by Adafruit) - Graphics primitives
3. **ESP8266WiFi** (built-in with ESP8266 board package)
4. **Time** (by Michael Margolis) - Time handling and NTP sync
5. **EEPROM** (built-in) - Data persistence

### Build & Upload Commands

**Compile (Verify):**
- Click "Verify" button ( icon) or Sketch -> Verify/Compile
- Checks for syntax errors without uploading

**Upload to Device:**
- Click "Upload" button (-> icon) or Sketch -> Upload
- Compiles and flashes to WeMos D1 Mini
- Device must be connected via USB

**Serial Monitor:**
- Tools -> Serial Monitor (or Ctrl+Shift+M)
- Set baud rate: **115200**
- Use for debugging output

**Serial Plotter:**
- Tools -> Serial Plotter
- Can visualize timer values over time

## Project Structure

```
dog-potty-tracker/
 src/
    main.ino              # Main Arduino sketch with setup() and loop()
    config.h              # Pin definitions and constants
    secrets.h             # WiFi credentials (NOT in git)
    secrets.h.example     # Template for WiFi setup (committed)
    TimerManager.h        # Timer state and logic
    TimerManager.cpp
    ButtonHandler.h       # Button debouncing, press detection, long-press
    ButtonHandler.cpp
    DisplayManager.h      # OLED rendering and view rotation
    DisplayManager.cpp
    WiFiManager.h         # WiFi connection, NTP sync, reconnection
    WiFiManager.cpp
    LEDController.h       # LED status logic
    LEDController.cpp
    Storage.h             # EEPROM persistence
    Storage.cpp
 docs/
    wiring.md             # Detailed wiring diagrams
    troubleshooting.md    # Common issues and solutions
 .gitignore                # Excludes secrets.h
 README.md                 # Project documentation
 CLAUDE.md                 # This file
```

## WiFi Configuration

### Setup Process

1. Copy `src/secrets.h.example` to `src/secrets.h`
2. Edit `src/secrets.h` with your WiFi credentials:
   ```cpp
   const char* WIFI_SSID = "YourNetworkName";
   const char* WIFI_PASSWORD = "YourPassword";
   ```
3. `secrets.h` is automatically ignored by `.gitignore`

### .gitignore Contents

```
src/secrets.h
*.swp
*.swo
.DS_Store
```

### WiFi Behavior

**Core Principle: Timers always work, WiFi is optional**

WiFi is only needed for:
- NTP time synchronization (to show actual timestamps)
- Future logging features (ThingSpeak, Google Sheets, etc.)

Without WiFi:
- Device still functions fully
- Shows elapsed timers only (no timestamps)
- All buttons and LEDs work normally

### Connection States

**Connected & Time Synced:** 
- Display shows actual timestamps (e.g., "1:30 PM")
- Current time displayed
- View rotation works between elapsed time and timestamps

**Connected, Time NOT Synced:** ->
- WiFi connected but NTP hasn't synced yet
- Show elapsed timers only
- Display: "Syncing time..." briefly

**Disconnected, Time WAS Synced:** =->L
- WiFi lost but NTP time was previously obtained
- ESP8266 internal clock continues (->1-2 sec/hour drift)
- Display: "No WiFi" instead of current time
- Background reconnection attempts

**Never Connected:** L
- WiFi never connected since boot
- Only show elapsed timers
- Display: "No WiFi - Times only"

### Reconnection Strategy

**Exponential Backoff:**
- Attempt 1: 1 second wait
- Attempt 2: 2 seconds wait
- Attempt 3: 4 seconds wait
- Attempt 4: 8 seconds wait
- Attempt 5: 16 seconds wait
- Attempt 6: 32 seconds wait
- Attempt 7+: 60 seconds wait (capped)

**Behavior:**
- Non-blocking reconnection (runs in background)
- Buttons and timers continue working during reconnection
- Display shows "No WiFi" status
- On successful reconnection: Re-sync NTP time immediately

### Display During WiFi Loss

**View A - Elapsed Time (5 seconds):**
```

OUT: 2h 15m ago 
PEE: 0h 45m ago 
POO: 1h 30m ago 
[No WiFi]        -> Instead of current time

```

**View B - Timestamps (5 seconds):**
```

OUT: 2h 15m ago 
PEE: 0h 45m ago 
POO: 1h 30m ago 
[No WiFi]        -> Same as View A when disconnected

```

When WiFi is disconnected, timestamp view is not shown since timestamps are unreliable.

## Core Features

### Three Timer System

**Timer 1: Outside**
- Tracks time since dog last went outside
- Reset by: Pressing Outside button
- Also reset by: Pressing Pee or Poop buttons (since dog went outside to do those)

**Timer 2: Last Pee**
- Tracks time since dog last peed
- Reset by: Pressing Pee button
- Also reset by: Pressing Poop button if combined event

**Timer 3: Last Poop**
- Tracks time since dog last pooped
- Reset by: Pressing Poop button

### Button Logic

**Button 1 (Outside) - Single Press:**
- Action: Log "went outside" event
- Resets: Outside timer only
- Use case: Took dog out but didn't see them go

**Button 2 (Pee) - Single Press:**
- Action: Log "pee" event
- Resets: Pee timer AND Outside timer
- Use case: Dog peed (implies they went outside)

**Button 3 (Poop) - Single Press:**
- Action: Log "poop" event
- Resets: Poop timer AND Outside timer
- Use case: Dog pooped (implies they went outside)

**Any Button - Long Press (3 seconds):**
- Action: Reset only that specific timer
- Resets: Individual timer to 00h 00m
- Use case: Correct mistakes or manual reset
- Visual feedback: Display shows "Resetting [Timer Name]..."

### Button Debouncing

**Implementation requirements:**
- Debounce delay: 50ms minimum
- Long-press threshold: 3000ms (3 seconds)
- Multi-press detection not needed (separate buttons for each action)

**State machine:**
1. Button idle (waiting for press)
2. Button pressed (debounce timer starts)
3. Debounce complete (check if still pressed)
4. Long-press detection (check if held for 3 seconds)
5. Button release (trigger action)

### Display System

**Two alternating views, auto-rotate every 5 seconds:**

**View A - Elapsed Time (5 seconds):**
```

OUT: 2h 15m ago   -> Simple text labels
PEE: 0h 45m ago   -> No emojis/icons
POO: 1h 30m ago   -> Yellow section (top 16px)
[3:45 PM]         -> Current time, Blue section (bottom 48px)

```

**View B - Timestamps (5 seconds):**
```

OUT: 1:30 PM      -> Actual time of event
PEE: 3:00 PM      -> Requires NTP sync
POO: 2:15 PM      -> Yellow section (top 16px)
[3:45 PM]         -> Current time, Blue section (bottom 48px)

```

**Display Rotation:**
- Automatic rotation every 5 seconds
- Non-blocking timer (use millis())
- Smooth transition (clear and redraw)
- Only show View B if NTP time is synced

**Text Rendering:**
- Use Adafruit_GFX built-in fonts
- No emoji support (monochrome bitmap display)
- Simple ASCII text labels: "OUT", "PEE", "POO"
- Font size: Small/medium for readability

**Time Format:**
- Elapsed: "Xh Ym ago" (e.g., "2h 15m ago")
- Timestamp: "H:MM AM/PM" (e.g., "3:45 PM")
- Handle edge cases: "0h 05m ago", "Just now" (< 1 min)

### LED Status Logic

**Three LEDs: Green, Yellow, Red**

**Status Determination (checked every loop):**

LEDs are based on the **Pee timer only** (Outside and Poop timers do not affect LED status).

**Green LED:**
- ON: Pee timer < 90 minutes (default good state)
- OFF: Yellow or Red condition is active

**Yellow LED (Warning):**
- ON: Pee timer > 90 minutes
- OFF: Pee timer below warning threshold
- Overrides: Green LED turns off when Yellow is on

**Red LED (Urgent):**
- ON: Pee timer > 3 hours (180 minutes)
- OFF: Pee timer below urgent threshold
- Overrides: Green and Yellow LEDs turn off when Red is on

**Priority:**
1. Check Red condition first (highest priority)
2. If Red is active: Red ON, Yellow OFF, Green OFF
3. Else check Yellow condition
4. If Yellow is active: Yellow ON, Red OFF, Green OFF
5. Else default: Green ON, Yellow OFF, Red OFF

**Implementation pseudocode:**
```cpp
if (peeTimer > 180) {  // 3 hours in minutes
    // RED - Urgent
    setLED(RED, HIGH);
    setLED(YELLOW, LOW);
    setLED(GREEN, LOW);
} else if (peeTimer > 90) {  // 90 minutes
    // YELLOW - Warning
    setLED(RED, LOW);
    setLED(YELLOW, HIGH);
    setLED(GREEN, LOW);
} else {
    // GREEN - All good
    setLED(RED, LOW);
    setLED(YELLOW, LOW);
    setLED(GREEN, HIGH);
}
```

**LED Behavior:**
- Solid (not blinking) for simplicity
- Immediate update when timer thresholds crossed
- LEDs turn off during night mode (11pm-5am)

### Night Mode (11pm - 5am)

**Automatic Behavior:**
- At 11:00 PM: Display turns off, LEDs turn off
- At 5:00 AM: Display turns on, LEDs resume normal operation
- Requires NTP time sync to function
- If no WiFi/time: Night mode disabled (always on)

**Button Behavior During Night Mode:**
- Button press: Wakes display AND logs event simultaneously
- Display shows confirmation briefly
- Screen stays awake for 10 seconds
- After 10 seconds: Display turns back off automatically
- Timers continue running in background

**Implementation:**
```cpp
bool isNightMode() {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    int hour = t->tm_hour;
    return (hour >= 23 || hour < 5);  // 11pm to 5am
}

// In main loop
if (isNightMode() && !temporaryWake) {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    setAllLEDs(LOW);
}

// On button press during night mode
if (isNightMode() && buttonPressed) {
    logEvent(button);  // Log the event
    temporaryWake = true;
    wakeUntil = millis() + 10000;  // 10 seconds
    display.ssd1306_command(SSD1306_DISPLAYON);
}

// Check if temporary wake expired
if (temporaryWake && millis() > wakeUntil) {
    temporaryWake = false;
    display.ssd1306_command(SSD1306_DISPLAYOFF);
}
```

**Display Behavior:**
- No gradual dimming (instant on/off)
- Display power management via SSD1306 commands
- OLED lifetime preservation (turns off when not needed)

### Data Persistence

**What to Save:**
- All three timer start timestamps (epoch time)
- Last known good WiFi connection state
- Display view preference (optional)

**When to Save:**
- On button press (immediate save after timer update)
- Every 5 minutes (automatic backup)
- Before deep sleep (if implemented)
- After NTP time sync (save timestamps)

**EEPROM Structure:**
```cpp
struct PersistentData {
    uint32_t outsideTimestamp;   // Unix epoch time
    uint32_t peeTimestamp;
    uint32_t poopTimestamp;
    uint32_t lastSaveTime;
    uint8_t checksum;            // Data integrity check
};
```

**EEPROM Wear Leveling:**
- ESP8266 EEPROM is actually flash memory (limited write cycles)
- Write only on actual changes (not every loop)
- Batch writes when possible
- ~100,000 write cycles per sector

**Data Restoration on Boot:**
1. Read EEPROM data
2. Verify checksum
3. If valid: Restore timer timestamps
4. If invalid: Initialize to zero (first boot or corrupted data)
5. Wait for NTP sync before showing timestamps

## Code Architecture

### Main Loop Structure

**setup():**
```cpp
void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);

    // Initialize hardware
    initPins();
    initDisplay();
    initButtons();
    initLEDs();

    // Restore saved data
    loadFromEEPROM();

    // Start WiFi connection (non-blocking)
    WiFiManager.begin(WIFI_SSID, WIFI_PASSWORD);

    // Display startup message
    displayStartup();
}
```

**loop():**
```cpp
void loop() {
    // Update current time
    unsigned long currentMillis = millis();

    // Handle WiFi (non-blocking reconnection)
    WiFiManager.update();

    // Check for button presses
    ButtonHandler.update();

    // Update timer calculations
    TimerManager.update();

    // Update LED status based on timers
    LEDController.update(TimerManager.getTimers());

    // Update display (handle view rotation)
    DisplayManager.update(currentMillis, TimerManager.getTimers());

    // Check night mode
    if (isNightMode()) {
        handleNightMode();
    }

    // Periodic EEPROM save (every 5 minutes)
    if (shouldSaveToEEPROM(currentMillis)) {
        saveToEEPROM();
    }

    // Small delay to prevent overwhelming the loop
    delay(10);
}
```

### Non-Blocking Design

**Critical: Never use blocking delays in main loop**

**Bad example:**
```cpp
delay(5000);  // Blocks everything for 5 seconds
```

**Good example:**
```cpp
unsigned long previousMillis = 0;
const long interval = 5000;

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        // Do periodic task
    }
}
```

**Why this matters:**
- Buttons must be responsive
- WiFi reconnection happens in background
- Display updates smoothly
- Timers count accurately

### Timer Management

**Class: TimerManager**

**Responsibilities:**
- Track three timer start times (epoch timestamps)
- Calculate elapsed time for each timer
- Reset individual or multiple timers
- Provide formatted strings for display

**Key methods:**
```cpp
class TimerManager {
public:
    void reset(Timer timer);              // Reset specific timer
    void resetMultiple(Timer* timers);    // Reset multiple timers
    unsigned long getElapsed(Timer timer); // Get elapsed time in seconds
    String getElapsedFormatted(Timer timer); // "2h 15m ago"
    String getTimestampFormatted(Timer timer); // "1:30 PM"
    time_t getTimestamp(Timer timer);     // Raw epoch time

private:
    time_t outsideStart;
    time_t peeStart;
    time_t poopStart;
};
```

**Timer representation:**
- Store as epoch timestamp (Unix time, seconds since 1970-01-01)
- Calculate elapsed: current_time - start_time
- Format for display using Time library

### Button Handler

**Class: ButtonHandler**

**Responsibilities:**
- Read button states
- Debounce button presses
- Detect short press vs long press
- Call appropriate callback functions

**Key methods:**
```cpp
class ButtonHandler {
public:
    void update();  // Call every loop iteration
    void setCallback(Button button, ButtonCallback callback);
    void setLongPressCallback(Button button, ButtonCallback callback);

private:
    void checkButton(Button button);
    bool isPressed(uint8_t pin);
    unsigned long lastDebounceTime[3];
    bool buttonState[3];
    bool lastButtonState[3];
    unsigned long buttonPressTime[3];
};
```

**Debounce algorithm:**
1. Read button pin
2. If state changed: Start debounce timer
3. Wait debounce delay (50ms)
4. Re-read button pin
5. If state consistent: Register as valid press
6. Track press duration for long-press detection

### Display Manager

**Class: DisplayManager**

**Responsibilities:**
- Render current view to OLED
- Handle view rotation (every 5 seconds)
- Format text and layout
- Handle night mode display state

**Key methods:**
```cpp
class DisplayManager {
public:
    void begin();  // Initialize display
    void update(unsigned long currentMillis, TimerData timers);
    void showStartup();
    void showButtonFeedback(String message);
    void setNightMode(bool enabled);

private:
    void renderElapsedView(TimerData timers);
    void renderTimestampView(TimerData timers);
    void rotateView(unsigned long currentMillis);
    Adafruit_SSD1306 display;
    DisplayView currentView;
    unsigned long lastViewSwitch;
};
```

**Rendering optimization:**
- Only redraw when data changes
- Use display.clearDisplay() before rendering
- Call display.display() after all drawing complete
- Use smallest font that's readable

**Layout example:**
```cpp
void renderElapsedView(TimerData timers) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Line 1: Outside timer
    display.setCursor(0, 0);
    display.print("OUT: ");
    display.print(timers.outsideElapsed);

    // Line 2: Pee timer
    display.setCursor(0, 16);
    display.print("PEE: ");
    display.print(timers.peeElapsed);

    // Line 3: Poop timer
    display.setCursor(0, 32);
    display.print("POO: ");
    display.print(timers.poopElapsed);

    // Line 4: Current time or status
    display.setCursor(0, 48);
    display.print("[");
    display.print(getCurrentTimeString());
    display.print("]");

    display.display();
}
```

### WiFi Manager

**Class: WiFiManager**

**Responsibilities:**
- Connect to WiFi on boot
- Handle disconnections
- Implement exponential backoff reconnection
- Sync time via NTP
- Provide connection status

**Key methods:**
```cpp
class WiFiManager {
public:
    void begin(const char* ssid, const char* password);
    void update();  // Call every loop iteration
    bool isConnected();
    bool isTimeSynced();
    void syncTime();

private:
    void attemptReconnect();
    void updateReconnectBackoff();
    unsigned long nextReconnectAttempt;
    unsigned int reconnectAttemptCount;
    bool timeSynced;
};
```

**NTP Time Sync:**
```cpp
void syncTime() {
    // Configure NTP with timezone offset
    configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    // -5 * 3600 = EST (adjust for your timezone)

    // Wait for time sync (non-blocking check)
    time_t now = time(nullptr);
    if (now > 1000000000) {  // Valid time
        timeSynced = true;
        Serial.println("Time synced!");
    }
}
```

**Exponential backoff:**
```cpp
void updateReconnectBackoff() {
    reconnectAttemptCount++;
    unsigned long backoffTime = min(pow(2, reconnectAttemptCount) * 1000, 60000);
    // Cap at 60 seconds
    nextReconnectAttempt = millis() + backoffTime;
}
```

### LED Controller

**Class: LEDController**

**Responsibilities:**
- Evaluate timer values against thresholds
- Set LED states (on/off)
- Handle night mode (all LEDs off)

**Key methods:**
```cpp
class LEDController {
public:
    void begin();  // Initialize LED pins
    void update(TimerData timers);
    void setNightMode(bool enabled);
    void test();  // Test all LEDs (startup sequence)

private:
    void setLED(LED led, bool state);
    void evaluateStatus(TimerData timers);
    bool nightMode;
};
```

**Status evaluation:**
```cpp
void evaluateStatus(TimerData timers) {
    if (nightMode) {
        setLED(GREEN, LOW);
        setLED(YELLOW, LOW);
        setLED(RED, LOW);
        return;
    }

    unsigned long peeMinutes = timers.peeElapsed / 60;
    unsigned long poopMinutes = timers.poopElapsed / 60;

    if (peeMinutes > 150 || poopMinutes > 300) {
        // Red - Urgent
        setLED(RED, HIGH);
        setLED(YELLOW, LOW);
        setLED(GREEN, LOW);
    } else if (peeMinutes > 90 || poopMinutes > 150) {
        // Yellow - Warning
        setLED(RED, LOW);
        setLED(YELLOW, HIGH);
        setLED(GREEN, LOW);
    } else {
        // Green - All good
        setLED(RED, LOW);
        setLED(YELLOW, LOW);
        setLED(GREEN, HIGH);
    }
}
```

### Storage Manager

**Class: Storage**

**Responsibilities:**
- Save timer data to EEPROM
- Load timer data from EEPROM
- Calculate and verify checksums
- Handle data corruption

**Key methods:**
```cpp
class Storage {
public:
    void begin();
    void save(TimerData timers);
    bool load(TimerData& timers);
    bool isValid();

private:
    uint8_t calculateChecksum(PersistentData* data);
    PersistentData data;
};
```

**Checksum calculation:**
```cpp
uint8_t calculateChecksum(PersistentData* data) {
    uint8_t checksum = 0;
    uint8_t* bytes = (uint8_t*)data;
    for (size_t i = 0; i < sizeof(PersistentData) - 1; i++) {
        checksum ^= bytes[i];  // XOR all bytes
    }
    return checksum;
}
```

## Configuration Constants

**config.h should contain:**

```cpp
#ifndef CONFIG_H
#define CONFIG_H

// Pin Definitions
#define PIN_OLED_SCL D1
#define PIN_OLED_SDA D2
#define PIN_BTN_OUTSIDE D5
#define PIN_BTN_PEE D6
#define PIN_BTN_POOP D7
#define PIN_LED_GREEN D0
#define PIN_LED_YELLOW D3
#define PIN_LED_RED D8

// OLED Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C  // Or 0x3D, check with I2C scanner

// Button Configuration
#define DEBOUNCE_DELAY 50        // milliseconds
#define LONG_PRESS_DURATION 3000 // milliseconds

// Display Configuration
#define VIEW_ROTATION_INTERVAL 5000  // milliseconds
#define NIGHT_MODE_WAKE_DURATION 10000  // milliseconds

// Timer Thresholds (in minutes)
#define YELLOW_PEE_THRESHOLD 90      // 1.5 hours
#define YELLOW_POOP_THRESHOLD 150    // 2.5 hours
#define RED_PEE_THRESHOLD 150        // 2.5 hours
#define RED_POOP_THRESHOLD 300       // 5 hours

// Night Mode Configuration
#define NIGHT_MODE_START_HOUR 23  // 11 PM
#define NIGHT_MODE_END_HOUR 5     // 5 AM

// EEPROM Configuration
#define EEPROM_SIZE 512
#define EEPROM_ADDRESS 0
#define EEPROM_SAVE_INTERVAL 300000  // 5 minutes in milliseconds

// NTP Configuration
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define TIMEZONE_OFFSET -5  // EST, adjust for your timezone
#define DST_OFFSET 0        // Daylight saving time offset

// WiFi Configuration
#define WIFI_CONNECT_TIMEOUT 10000  // milliseconds
#define WIFI_RECONNECT_MAX_BACKOFF 60000  // 1 minute cap

#endif
```

## Testing & Debugging

### Serial Debugging

**Enable verbose logging:**
```cpp
#define DEBUG 1

#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif
```

**Useful debug output:**
- Button press events
- Timer values every 10 seconds
- WiFi connection status changes
- NTP sync status
- EEPROM save/load operations
- LED state changes

### Component Testing

**Test OLED Display:**
```cpp
void testDisplay() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Display Test");
    display.display();
}
```

**Test Buttons:**
```cpp
void testButtons() {
    DEBUG_PRINTLN("Button Test Mode");
    DEBUG_PRINTLN("Press each button...");

    if (digitalRead(PIN_BTN_OUTSIDE) == HIGH) {
        DEBUG_PRINTLN("Outside button pressed");
    }
    // Repeat for other buttons
}
```

**Test LEDs:**
```cpp
void testLEDs() {
    digitalWrite(PIN_LED_GREEN, HIGH);
    delay(500);
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_YELLOW, HIGH);
    delay(500);
    digitalWrite(PIN_LED_YELLOW, LOW);
    digitalWrite(PIN_LED_RED, HIGH);
    delay(500);
    digitalWrite(PIN_LED_RED, LOW);
}
```

**I2C Scanner (find OLED address):**
```cpp
void scanI2C() {
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            DEBUG_PRINT("I2C device found at 0x");
            DEBUG_PRINTLN(address, HEX);
        }
    }
}
```

### Common Issues

**OLED not displaying:**
- Check I2C address (0x3C or 0x3D) - use I2C scanner
- Verify wiring: SDA->D2, SCL->D1
- Ensure VCC connected to 3V3 (not 5V, though both should work)
- Check if Adafruit_SSD1306 library installed

**Button not responding:**
- Verify pull-down resistor (10k-> to GND)
- Check button wiring: one leg to 3V3, other to GPIO
- Test with multimeter: should read 0V when not pressed, 3.3V when pressed
- Try increasing debounce delay

**LEDs not lighting:**
- Check polarity: anode (+) to GPIO through resistor, cathode (-) to GND
- Verify resistor value (220-330->)
- Test LED directly: connect anode to 3V3, cathode to GND (should light)
- Check if correct pin defined in config.h

**WiFi not connecting:**
- Verify credentials in secrets.h
- Check 2.4GHz WiFi (ESP8266 doesn't support 5GHz)
- Monitor Serial output for connection status
- Try increasing WIFI_CONNECT_TIMEOUT

**Time not syncing:**
- Verify WiFi is connected first
- Check NTP server accessibility (firewall?)
- Ensure timezone offset is correct
- May take 1-2 seconds on first sync

**EEPROM data corrupted:**
- Check checksum verification logic
- EEPROM may be blank on first boot (expected)
- Use EEPROM.begin(EEPROM_SIZE) in setup()
- Call EEPROM.commit() after EEPROM.write()

## Future Enhancements

### Planned Features (Not Yet Implemented)

**WiFi Data Logging:**
- Log events to ThingSpeak for historical tracking
- Export data to Google Sheets via IFTTT webhook
- Local CSV file storage on SPIFFS filesystem
- View trends and patterns over time

**Web Interface:**
- Built-in web server on ESP8266
- Access from phone/computer on same WiFi
- View current status and history
- Configure settings without re-uploading code
- Export data as CSV

**Multiple Dog Support:**
- Track up to 3 dogs separately
- Cycle between dogs with 4th button or button combo
- Display current dog name
- Individual timers per dog stored in EEPROM

**Advanced Alerts:**
- Buzzer for audio alerts when timer exceeds threshold
- Configurable alert schedules
- Smart alerts based on learned patterns
- IFTTT/Webhooks for phone notifications

**Better Time Display:**
- Show day of week (Mon, Tue, etc.)
- 12-hour vs 24-hour format toggle
- Configurable date format
- Timezone auto-detection

**Battery Operation:**
- LiPo battery + TP4056 charging module
- Deep sleep modes for power saving
- Wake on button press via external interrupt
- Battery level indicator on display

**Enclosure:**
- 3D printed case design
- Wall mountable or desk stand
- Waterproof buttons
- Professional appearance

**Analytics:**
- Average time between events
- Pattern detection (specific times of day)
- Health tracking (frequency trends)
- Export reports

### Development Roadmap

**Phase 1: Core Functionality** (Current)
-  Basic timer tracking
-  OLED display with view rotation
-  LED status indicators
-  Button input with debouncing
-  Data persistence (EEPROM)
-  WiFi + NTP time sync
-  Night mode

**Phase 2: Enhanced Features**
- WiFi data logging (ThingSpeak)
- Web interface for viewing history
- Improved error handling and recovery
- OTA (Over-The-Air) firmware updates

**Phase 3: Advanced Features**
- Multiple dog support
- Audio alerts (buzzer)
- Mobile app integration
- Pattern analysis and predictions

**Phase 4: Hardware Improvements**
- 3D printed enclosure design
- Battery operation with charging
- PCB design (move off breadboard)
- Production-ready build

## References & Resources

**ESP8266 Documentation:**
- Official Docs: https://arduino-esp8266.readthedocs.io/
- Pinout Reference: https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
- WeMos D1 Mini: https://www.wemos.cc/en/latest/d1/d1_mini.html

**Library Documentation:**
- Adafruit SSD1306: https://github.com/adafruit/Adafruit_SSD1306
- Adafruit GFX: https://github.com/adafruit/Adafruit-GFX-Library
- ESP8266WiFi: https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html
- Time Library: https://github.com/PaulStoffregen/Time

**Tutorials:**
- ESP8266 + OLED: https://randomnerdtutorials.com/esp8266-0-96-inch-oled-display-with-arduino-ide/
- NTP Time Sync: https://randomnerdtutorials.com/esp8266-nodemcu-date-time-ntp-client-server-arduino/
- EEPROM Usage: https://arduino-esp8266.readthedocs.io/en/latest/libraries.html#eeprom

**Community:**
- Arduino Forum: https://forum.arduino.cc/
- ESP8266 Community: https://www.esp8266.com/
- Reddit r/esp8266: https://www.reddit.com/r/esp8266/

## License

This project is intended for personal use. Modify and enhance as needed.

---

**Last Updated:** 2025-10-22
**Target Platform:** WeMos D1 Mini (ESP8266 ESP-12F)
**Arduino IDE Version:** 2.x
**ESP8266 Board Package:** 3.x
