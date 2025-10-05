#include "WiFiSync.h"
#include <DisplayManager.h>
#include <ArduinoJson.h>
// #include "PumpController.h"
#include <WiFiManager.h>
#include <WifiConfig.h>
#include <Config.h>

void syncData()
{
  DisplayManager &display = DisplayManager::getInstance();
  WiFiManager &wifi = WiFiManager::getInstance();
  JsonDocument doc;
  int rssi = wifi.getSignalStrength();
  // doc["pumpId"] = ID_PERISTALTIC_STEPPER;
  // doc["stepsPerML"] = pump.getStepsPerML();
  // doc["stepsPerSecond"] = pump.getSpeedStep();
  // doc["currentSpeed"] = pump.getSpeed();
  doc["rssi"] = rssi;
  display.setSignalStrength(rssi);
  String jsonData;
  serializeJson(doc, jsonData);
  String response;
  // if (wifi.post(PUMP_SETTINGS_API, "application/json", jsonData.c_str(), response)) {
  //   Serial.println("Sync Ok");
  // } else {
  //   Serial.println("Sync failed");
  // }
}

void handleWiFi(unsigned long currentTime, unsigned long &lastWiFiRetryTime)
{

  DisplayManager &display = DisplayManager::getInstance();
  WiFiManager &wifi = WiFiManager::getInstance();
  if (!wifi.isConnected() && currentTime - lastWiFiRetryTime >= WIFI_RETRY_INTERVAL)
  {
    Serial.println("Attempting WiFi Connect...");
    if (wifi.connect(ssid, password))
    {
      display.setSignalStrength(wifi.getSignalStrength());
      display.showText("Syncing Time...");
      wifi.configureTime("asia.pool.ntp.org", "ICT-7");
      display.showText("WiFi Connected");
    }
    else
    {
      display.showText("WiFi Failed");
    }
    lastWiFiRetryTime = currentTime;
  }
  // Server health check (if needed, can be expanded)
  if (!wifi.isConnected() && currentTime - lastWiFiRetryTime >= WIFI_RETRY_INTERVAL)
  {
    Serial.println("Server Health Check Failed");
    display.showText("Server Down");
    lastWiFiRetryTime = currentTime;
  }
}