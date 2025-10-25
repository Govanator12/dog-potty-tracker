#include "TimerManager.h"

TimerManager::TimerManager() {
  // Initialize all timers to current time
  time_t now = time(nullptr);
  outsideStart = now;
  peeStart = now;
  poopStart = now;
}

void TimerManager::reset(Timer timer) {
  time_t now = time(nullptr);
  switch (timer) {
    case TIMER_OUTSIDE:
      outsideStart = now;
      break;
    case TIMER_PEE:
      peeStart = now;
      break;
    case TIMER_POOP:
      poopStart = now;
      break;
  }
}

void TimerManager::resetOutside() {
  reset(TIMER_OUTSIDE);
}

void TimerManager::resetPee() {
  reset(TIMER_PEE);
}

void TimerManager::resetPoop() {
  reset(TIMER_POOP);
}

unsigned long TimerManager::getElapsed(Timer timer) {
  time_t now = time(nullptr);
  time_t start;

  switch (timer) {
    case TIMER_OUTSIDE:
      start = outsideStart;
      break;
    case TIMER_PEE:
      start = peeStart;
      break;
    case TIMER_POOP:
      start = poopStart;
      break;
    default:
      return 0;
  }

  // Handle case where time hasn't been synced yet
  if (now < 1000000000 || start < 1000000000) {
    return 0;
  }

  return (unsigned long)(now - start);
}

String TimerManager::formatElapsed(unsigned long seconds) {
  if (seconds < 60) {
    return "0h 00m ago";  // Use consistent format instead of "Just now"
  }

  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  minutes = minutes % 60;

  String result = "";
  result += hours;
  result += "h ";
  if (minutes < 10) result += "0";
  result += minutes;
  result += "m ago";

  return result;
}

String TimerManager::getElapsedFormatted(Timer timer) {
  unsigned long elapsed = getElapsed(timer);
  return formatElapsed(elapsed);
}

String TimerManager::getTimestampFormatted(Timer timer) {
  time_t timestamp = getTimestamp(timer);

  // Check if time is valid
  if (timestamp < 1000000000) {
    return "--:--";
  }

  struct tm* timeinfo = localtime(&timestamp);

  // Format as 12-hour time with AM/PM
  int hour = timeinfo->tm_hour;
  bool isPM = hour >= 12;
  if (hour > 12) hour -= 12;
  if (hour == 0) hour = 12;

  char buffer[10];
  snprintf(buffer, sizeof(buffer), "%d:%02d %s",
           hour, timeinfo->tm_min, isPM ? "PM" : "AM");

  return String(buffer);
}

time_t TimerManager::getTimestamp(Timer timer) {
  switch (timer) {
    case TIMER_OUTSIDE:
      return outsideStart;
    case TIMER_PEE:
      return peeStart;
    case TIMER_POOP:
      return poopStart;
    default:
      return 0;
  }
}

void TimerManager::setTimestamp(Timer timer, time_t timestamp) {
  switch (timer) {
    case TIMER_OUTSIDE:
      outsideStart = timestamp;
      break;
    case TIMER_PEE:
      peeStart = timestamp;
      break;
    case TIMER_POOP:
      poopStart = timestamp;
      break;
  }
}

bool TimerManager::isTimeSynced() {
  time_t now = time(nullptr);
  return now > 1000000000;  // Valid time (after year 2001)
}
