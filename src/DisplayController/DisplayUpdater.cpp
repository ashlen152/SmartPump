/**
 * @file DisplayUpdater.cpp
 * @brief Populates DisplayContext with system state and triggers display refresh.
 *
 * Feeds NORMAL state context with real system values:
 *   - pumpEnabled: from PumpController::getIsEnable()
 *   - value: from PumpController::getSpeed()
 *   - currentTime: from WiFiManager NTP time
 *   - autodosingEnabled: from AutoDosingManager::isEnabled() ✓ IMPLEMENTED
 *   - nextSchedule: from AutoDosingManager::getNextDosingTime() ✓ IMPLEMENTED
 */

#include "DisplayUpdater.h"
#include <DisplayManager.h>
#include <WiFiManager.h>
#include <PumpController.h>
#include <AutoDosingManager.h>
#include <time.h>

static DisplayManager &display = DisplayManager::getInstance();
static WiFiManager &wifi = WiFiManager::getInstance();
static PumpController &pump = PumpController::getInstance();

void updateDisplayStatus()
{
  AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
  
  // Fill DisplayContext with real data from system
  DisplayContext ctx;
  ctx.pumpEnabled = pump.getIsEnable();
  ctx.value = pump.getSpeed();
  ctx.currentTime = wifi.getCurrentTime();
  ctx.autodosingEnabled = autoDosing.isEnabled();  // FIXED: real auto-dosing state
  
  // Format next schedule time (Phase 3 Sprint 5: Show PAUSED if paused)
  static char nextScheduleStr[16] = "-----";
  if (autoDosing.isPaused()) {
    uint32_t remaining = autoDosing.getPauseRemaining();
    if (remaining == 0xFFFFFFFF) {
      snprintf(nextScheduleStr, sizeof(nextScheduleStr), "PAUSED");
    } else if (remaining > 3600) {
      snprintf(nextScheduleStr, sizeof(nextScheduleStr), "P:%dh", remaining / 3600);
    } else if (remaining > 60) {
      snprintf(nextScheduleStr, sizeof(nextScheduleStr), "P:%dm", remaining / 60);
    } else {
      snprintf(nextScheduleStr, sizeof(nextScheduleStr), "P:%ds", remaining);
    }
    ctx.nextSchedule = nextScheduleStr;
  } else {
    uint32_t nextTime = autoDosing.getNextDosingTime();
    if (nextTime > 0 && autoDosing.isEnabled()) {
      time_t t = nextTime;
      struct tm* timeinfo = localtime(&t);
      snprintf(nextScheduleStr, sizeof(nextScheduleStr), "%02d:%02d", 
               timeinfo->tm_hour, timeinfo->tm_min);
      ctx.nextSchedule = nextScheduleStr;  // FIXED: real next schedule
    } else {
      ctx.nextSchedule = "-----";
    }
  }

  display.setContextNormal(ctx.pumpEnabled, ctx.value, ctx.currentTime, ctx.autodosingEnabled, ctx.nextSchedule);
  display.updateDisplayState();
}
