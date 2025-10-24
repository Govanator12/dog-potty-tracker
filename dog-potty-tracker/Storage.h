#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <time.h>
#include "config.h"
#include "TimerManager.h"

// Data structure for EEPROM storage
struct PersistentData {
  uint32_t outsideTimestamp;   // Unix epoch time
  uint32_t peeTimestamp;
  uint32_t poopTimestamp;
  uint32_t lastSaveTime;
  uint8_t checksum;            // Data integrity check
};

class Storage {
public:
  Storage();

  // Initialize EEPROM
  void begin();

  // Save timer data to EEPROM
  void save(TimerManager* timerManager);

  // Load timer data from EEPROM
  bool load(TimerManager* timerManager);

  // Check if EEPROM data is valid
  bool isValid();

private:
  PersistentData data;

  // Calculate checksum for data integrity
  uint8_t calculateChecksum(PersistentData* data);
};

#endif
