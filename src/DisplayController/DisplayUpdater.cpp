#include "DisplayUpdater.h"
#include <DisplayManager.h>
#include <WiFiManager.h>

static DisplayManager &display = DisplayManager::getInstance();
static WiFiManager &wifi = WiFiManager::getInstance();

void updateDisplayStatus()
{

  // Example: fill DisplayContext with real data from your system
  DisplayContext ctx;
  ctx.pumpEnabled = false; // Replace with pump.isEnabled()
  ctx.value = 0.0f;        // Replace with pump.getSpeed() or other value
  ctx.currentTime = wifi.getCurrentTime();
  ctx.autodosingEnabled = true; // Replace with autoDosing.isEnabled()
  ctx.nextSchedule = nullptr;   // Replace with next dosing schedule if available

  display.setContextNormal(ctx.pumpEnabled, ctx.value, ctx.currentTime, ctx.autodosingEnabled, ctx.nextSchedule);
  display.updateDisplayState();
}
