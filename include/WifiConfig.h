#ifndef WIFICONFIG_H
#define WIFICONFIG_H

constexpr const char *ssid = "SofaHome";
constexpr const char *password = "Sofa@123";

constexpr const char *ID_PERISTALTIC_STEPPER = "pump-1";
constexpr const char *PUMP_SETTINGS_API = "/api/pump-settings";
constexpr const char *PUMP_BY_ID_API = "/api/pump-settings/getById"; // API endpoint for get current settings

constexpr const int WIFI_RETRY_INTERVAL = 5000; // ms
constexpr const int SYNC_INTERVAL = 180000;     // ms

#endif
