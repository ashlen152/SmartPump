#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <BluetoothSerial.h>

class BluetoothManager {
private:
  BluetoothSerial SerialBT;
  const char* _deviceName;  // Name for the ESP32 Bluetooth device
  bool _connected;
  const int TIMEOUT_MS = 5000;  // 5 seconds timeout for responses

public:
  BluetoothManager(const char* deviceName = "ESP32_BT");
  ~BluetoothManager();
  bool connect();
  bool isConnected();
  void disconnect();
  
  // HTTP-like methods adapted for Bluetooth
  bool get(const char* path, String& response);
  bool post(const char* path, const char* contentType, const char* body, String& response);
  bool put(const char* path, const char* contentType, const char* body, String& response);
  bool del(const char* path, String& response);
  
  // Legacy method
  bool checkApiHealth();
};

#endif