#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include <Arduino.h>
#include <time.h>

enum Timer {
  TIMER_OUTSIDE = 0,
  TIMER_PEE = 1,
  TIMER_POOP = 2
};

class TimerManager {
public:
  TimerManager();

  // Reset specific timer to current time
  void reset(Timer timer);

  // Reset multiple timers at once
  void resetOutside();
  void resetPee();
  void resetPoop();

  // Get elapsed time in seconds
  unsigned long getElapsed(Timer timer);

  // Get formatted elapsed time string (e.g., "2h 15m ago")
  String getElapsedFormatted(Timer timer);

  // Get formatted timestamp string (e.g., "1:30 PM")
  String getTimestampFormatted(Timer timer);

  // Get raw epoch timestamp
  time_t getTimestamp(Timer timer);

  // Set timer to specific timestamp (for loading from EEPROM)
  void setTimestamp(Timer timer, time_t timestamp);

  // Check if time is synced
  bool isTimeSynced();

private:
  time_t outsideStart;
  time_t peeStart;
  time_t poopStart;

  // Helper function to format elapsed time
  String formatElapsed(unsigned long seconds);
};

#endif
