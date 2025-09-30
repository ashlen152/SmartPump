void handleWiFi(unsigned long currentTime, unsigned long &lastWiFiRetryTime);
#pragma once
extern DisplayManager &display;
extern PumpController pump;
extern WiFiManager wifi;
void syncData();
