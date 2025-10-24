#include "Storage.h"

Storage::Storage() {
}

void Storage::begin() {
  EEPROM.begin(EEPROM_SIZE);
  DEBUG_PRINTLN("Storage initialized");
}

void Storage::save(TimerManager* timerManager) {
  // Populate data structure
  data.outsideTimestamp = (uint32_t)timerManager->getTimestamp(TIMER_OUTSIDE);
  data.peeTimestamp = (uint32_t)timerManager->getTimestamp(TIMER_PEE);
  data.poopTimestamp = (uint32_t)timerManager->getTimestamp(TIMER_POOP);
  data.lastSaveTime = (uint32_t)time(nullptr);

  // Calculate checksum
  data.checksum = calculateChecksum(&data);

  // Write to EEPROM
  EEPROM.put(EEPROM_ADDRESS, data);
  EEPROM.commit();

  DEBUG_PRINTLN("Storage: Data saved to EEPROM");
  DEBUG_PRINT("  Outside: ");
  DEBUG_PRINTLN(data.outsideTimestamp);
  DEBUG_PRINT("  Pee: ");
  DEBUG_PRINTLN(data.peeTimestamp);
  DEBUG_PRINT("  Poop: ");
  DEBUG_PRINTLN(data.poopTimestamp);
}

bool Storage::load(TimerManager* timerManager) {
  // Read from EEPROM
  EEPROM.get(EEPROM_ADDRESS, data);

  // Verify checksum
  uint8_t expectedChecksum = calculateChecksum(&data);
  if (data.checksum != expectedChecksum) {
    DEBUG_PRINTLN("Storage: Checksum mismatch - data corrupted or first boot");
    return false;
  }

  // Restore timer timestamps
  timerManager->setTimestamp(TIMER_OUTSIDE, (time_t)data.outsideTimestamp);
  timerManager->setTimestamp(TIMER_PEE, (time_t)data.peeTimestamp);
  timerManager->setTimestamp(TIMER_POOP, (time_t)data.poopTimestamp);

  DEBUG_PRINTLN("Storage: Data loaded from EEPROM");
  DEBUG_PRINT("  Outside: ");
  DEBUG_PRINTLN(data.outsideTimestamp);
  DEBUG_PRINT("  Pee: ");
  DEBUG_PRINTLN(data.peeTimestamp);
  DEBUG_PRINT("  Poop: ");
  DEBUG_PRINTLN(data.poopTimestamp);
  DEBUG_PRINT("  Last save: ");
  DEBUG_PRINTLN(data.lastSaveTime);

  return true;
}

bool Storage::isValid() {
  // Read from EEPROM
  EEPROM.get(EEPROM_ADDRESS, data);

  // Verify checksum
  uint8_t expectedChecksum = calculateChecksum(&data);
  return data.checksum == expectedChecksum;
}

uint8_t Storage::calculateChecksum(PersistentData* data) {
  uint8_t checksum = 0;
  uint8_t* bytes = (uint8_t*)data;

  // XOR all bytes except the checksum byte itself
  for (size_t i = 0; i < sizeof(PersistentData) - 1; i++) {
    checksum ^= bytes[i];
  }

  return checksum;
}
