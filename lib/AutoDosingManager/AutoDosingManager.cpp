#include "AutoDosingManager.h"
#include <EEPROM.h>
#include <time.h>

static time_t toTimeT(uint32_t t) {
    return static_cast<time_t>(t);
}

AutoDosingManager::AutoDosingManager(PumpController& p, DisplayManager& d, const Config& config)
    : pump(p), display(d), eepromConfig(config) {
    scheduleMeta.enabled = true;
    scheduleMeta.totalDailyVolume = config.defaultVolume;
    scheduleMeta.dayStartHour = startHour;
    scheduleMeta.dayEndHour = endHour;
    scheduleMeta.lastDosingTime = 0;
    scheduleMeta.nextDosingTime = 0;
    scheduleMeta.lastDoseVolume = 0;
    scheduleMeta.totalDosesDay = 0;
    scheduleMeta.totalDosesNight = 0;
    generateWeightedSchedule(slots, scheduleMeta.totalDailyVolume, startHour, endHour, percent1, percent2);
}

void AutoDosingManager::begin() {
    AUTO_DOSING_LOG("Initializing Auto Dosing Manager");
    loadState();
}

void AutoDosingManager::enable() {
    AUTO_DOSING_LOG("Enabling auto dosing");
    scheduleMeta.enabled = true;
    resetDailyVolume();
}

void AutoDosingManager::disable() {
    AUTO_DOSING_LOG("Disabling auto dosing");
    scheduleMeta.enabled = false;
}

void AutoDosingManager::setDailyVolume(float volume) {
    AUTO_DOSING_LOG("Setting daily volume to %.2f ml", volume);
    scheduleMeta.totalDailyVolume = volume;
    resetDailyVolume();
}


void AutoDosingManager::loadState() {
    AUTO_DOSING_LOG("Loading state from EEPROM");
    
    bool enabled = false;
    float storedDailyVolume = eepromConfig.defaultVolume;
    uint32_t lastDoseTime = 0;
    float storedTotalDosed = 0;
    
    EEPROM.get(eepromConfig.enabledAddr, enabled);
    EEPROM.get(eepromConfig.volumeAddr, storedDailyVolume);
    EEPROM.get(eepromConfig.lastTimeAddr, lastDoseTime);
    EEPROM.get(eepromConfig.totalDosedAddr, storedTotalDosed);
    
    // Set values with validation
    scheduleMeta.enabled = enabled;
    scheduleMeta.totalDailyVolume = (!isnan(storedDailyVolume) && storedDailyVolume > 0) ? storedDailyVolume : eepromConfig.defaultVolume;
    scheduleMeta.lastDosingTime = lastDoseTime;
    totalDosedVolume = (!isnan(storedTotalDosed) && storedTotalDosed >= 0) ? storedTotalDosed : 0;
    AUTO_DOSING_LOG("Loaded: enabled=%d, daily=%.2f, last=%lu, total=%.2f",
                    scheduleMeta.enabled, scheduleMeta.totalDailyVolume,
                    scheduleMeta.lastDosingTime, totalDosedVolume);
}

void AutoDosingManager::saveState() {
    AUTO_DOSING_LOG("Saving state to EEPROM");
    
    EEPROM.put(eepromConfig.enabledAddr, scheduleMeta.enabled);
    EEPROM.put(eepromConfig.volumeAddr, scheduleMeta.totalDailyVolume);
    EEPROM.put(eepromConfig.lastTimeAddr, scheduleMeta.lastDosingTime);
    EEPROM.put(eepromConfig.totalDosedAddr, totalDosedVolume);
    EEPROM.commit();
}

void AutoDosingManager::printStatus() const {
    AUTO_DOSING_LOG("=== Auto Dosing Status ===");
    AUTO_DOSING_LOG("Enabled: %d", scheduleMeta.enabled);
    AUTO_DOSING_LOG("Daily Volume: %.2f ml", scheduleMeta.totalDailyVolume);
    AUTO_DOSING_LOG("Total Dosed Today: %.2f ml", totalDosedVolume);
    AUTO_DOSING_LOG("Last Dose: %.2f ml", scheduleMeta.lastDoseVolume);
}

void AutoDosingManager::printSchedule() const {
    AUTO_DOSING_LOG("=== Dosing Schedule ===");
    AUTO_DOSING_LOG("Total Daily: %.2f ml", scheduleMeta.totalDailyVolume);
}

void AutoDosingManager::logDosingEvent(float volume, bool success) {
    AUTO_DOSING_LOG("=== Dosing Event ===");
    AUTO_DOSING_LOG("Time: %lu", time(nullptr));
    AUTO_DOSING_LOG("Volume: %.2f ml", volume);
    AUTO_DOSING_LOG("Success: %d", success);
    AUTO_DOSING_LOG("Total Today: %.2f ml", totalDosedVolume);
    AUTO_DOSING_LOG("Remaining: %.2f ml", getRemainingDailyVolume());
    // Day/Night period logging removed
}