#include <DisplayManager.h>
#include "DisplayUpdater.h"
#include <DisplayManager.h>
#include "PumpController.h"
#include <WiFiManager.h>
#include <AutoDosingManager.h>
extern DisplayManager &display;
extern PumpController pump;
extern WiFiManager wifi;
extern AutoDosingManager autoDosing;
extern DisplayManager::PumpMode currentMode;

void updateDisplayStatus() {
  float displayValue = pump.getSpeed();
  display.updateStatus(pump.isEnabled(), displayValue, currentMode,
                       currentMode == DisplayManager::PumpMode::DOSING ? wifi.getCurrentTime() : nullptr,
                       autoDosing.isEnabled(), nullptr);
}
