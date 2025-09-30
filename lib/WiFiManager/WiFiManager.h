#ifndef WIFIMANAGER_H  // Include guard
#define WIFIMANAGER_H

#include <WiFi.h>
#include <DisplayManager.h>
#include <ArduinoHttpClient.h>
#include <time.h>

class WiFiManager {
private:
  const char* _ssid;
  const char* _password;
  const int MAX_ATTEMPTS = 20;
  const String _serverAddress = "192.168.68.108";
  const int _port = 3000;
  const int HTTP_TIMEOUT = 1000; // Timeout for HTTP requests
  const int MIN_RSSI = -80; // Minimum RSSI for a good connection
  WiFiClient wifiClient;               // WiFi client for HTTP
  HttpClient* httpClient = nullptr;    // Pointer to HttpClient, initialized later
  
  unsigned long lastTimeSync = 0;      // Last time we synced with NTP
  unsigned long lastTimeUpdate = 0;    // Last time we updated the local time string
  time_t lastSyncedTime = 0;          // The time we got from last NTP sync
  const int TIME_SYNC_INTERVAL = 3600000; // Sync with NTP every hour (in ms)
  const int TIME_UPDATE_INTERVAL = 1000;  // Update displayed time every second (in ms)
  bool timeInitialized = false;        // Flag to track if time was ever initialized

public:
  WiFiManager(const char* ssid, const char* password);
  ~WiFiManager();  // Destructor to clean up

  bool connect();
  bool isConnected();
  void disconnect();
  int getSignalStrength();
  
  // HTTP Methods
  bool get(const char* path, String& response);
  bool post(const char* path, const char* contentType, const char* body, String& response);
  bool put(const char* path, const char* contentType, const char* body, String& response);
  bool del(const char* path, String& response);

  bool checkApiHealth();
  void configureTime(const char* ntpServer = "pool.ntp.org", const char* timezone = "UTC");
  const char* getCurrentTime();

private:
  static char timeStr[9];  // HH:MM:SS + null terminator
};

#endif