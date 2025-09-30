#pragma once
extern DisplayManager &display;
extern PumpController pump;
extern WiFiManager wifi;
extern AutoDosingManager autoDosing;
extern DisplayManager::PumpMode currentMode;
void updateDisplayStatus();
