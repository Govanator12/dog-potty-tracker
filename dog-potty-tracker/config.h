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
#define OLED_ADDRESS 0x3C  // Or 0x3D, check with I2C scanner if display doesn't work

// Button Configuration
#define DEBOUNCE_DELAY 50        // milliseconds
#define LONG_PRESS_DURATION 3000 // milliseconds (3 seconds)

// Display Configuration
#define VIEW_ROTATION_INTERVAL 5000  // milliseconds (5 seconds)
#define NIGHT_MODE_WAKE_DURATION 10000  // milliseconds (10 seconds)

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
#define TIMEZONE_OFFSET -5  // EST, adjust for your timezone (PST = -8, CST = -6, MST = -7, EST = -5)
#define DST_OFFSET 0        // Daylight saving time offset (usually 0 or 1)

// WiFi Configuration
#define WIFI_CONNECT_TIMEOUT 10000  // milliseconds (10 seconds)
#define WIFI_RECONNECT_MAX_BACKOFF 60000  // 1 minute cap

// IFTTT Notifications Configuration
#define IFTTT_NOTIFICATION_COOLDOWN 3600000  // 1 hour in milliseconds (prevent spam)
#define NOTIFICATION_QUIET_START_HOUR 23     // 11 PM - don't send notifications
#define NOTIFICATION_QUIET_END_HOUR 7        // 7 AM - resume notifications

// Debug Configuration
#define DEBUG 1  // Set to 0 to disable debug output

#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

#endif
