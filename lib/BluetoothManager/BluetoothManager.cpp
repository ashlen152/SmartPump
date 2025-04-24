#include "BluetoothManager.h"

BluetoothManager::BluetoothManager(const char* deviceName) {
  _deviceName = deviceName;
  _connected = false;
  SerialBT.begin(_deviceName);  // Initialize Bluetooth with the given name
  Serial.println("Bluetooth device started, pair with it now.");
}

BluetoothManager::~BluetoothManager() {
  disconnect();
}

bool BluetoothManager::connect() {
  // For Bluetooth Classic, "connect" means waiting for a pairing
  Serial.println("Waiting for Bluetooth pairing...");
  unsigned long startTime = millis();
  
  while (!SerialBT.hasClient() && (millis() - startTime) < TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  
  if (SerialBT.hasClient()) {
    Serial.println("");
    Serial.println("Bluetooth connected!");
    _connected = true;
    return true;
  } else {
    Serial.println("");
    Serial.println("Bluetooth connection timed out or failed");
    _connected = false;
    return false;
  }
}

bool BluetoothManager::isConnected() {
  _connected = SerialBT.hasClient();  // Update status
  return _connected;
}

void BluetoothManager::disconnect() {
  SerialBT.disconnect();
  _connected = false;
  Serial.println("Bluetooth disconnected");
}

bool BluetoothManager::get(const char* path, String& response) {
  if (!isConnected()) {
    Serial.println("Cannot perform GET: Not connected to Bluetooth");
    return false;
  }

  Serial.print("GET ");
  Serial.println(path);
  SerialBT.print("GET ");
  SerialBT.println(path);

  unsigned long startTime = millis();
  response = "";
  while ((millis() - startTime) < TIMEOUT_MS) {
    if (SerialBT.available()) {
      response = SerialBT.readStringUntil('\n');
      Serial.print("Response: ");
      Serial.println(response);
      return true;
    }
    delay(10);
  }
  
  Serial.println("GET timed out");
  return false;
}

bool BluetoothManager::post(const char* path, const char* contentType, const char* body, String& response) {
  if (!isConnected()) {
    Serial.println("Cannot perform POST: Not connected to Bluetooth");
    return false;
  }

  Serial.print("POST ");
  Serial.println(path);
  SerialBT.print("POST ");
  SerialBT.print(path);
  SerialBT.print(" ");
  SerialBT.print(contentType);
  SerialBT.print(" ");
  SerialBT.println(body);

  unsigned long startTime = millis();
  response = "";
  while ((millis() - startTime) < TIMEOUT_MS) {
    if (SerialBT.available()) {
      response = SerialBT.readStringUntil('\n');
      Serial.print("Response: ");
      Serial.println(response);
      return true;
    }
    delay(10);
  }
  
  Serial.println("POST timed out");
  return false;
}

bool BluetoothManager::put(const char* path, const char* contentType, const char* body, String& response) {
  if (!isConnected()) {
    Serial.println("Cannot perform PUT: Not connected to Bluetooth");
    return false;
  }

  Serial.print("PUT ");
  Serial.println(path);
  SerialBT.print("PUT ");
  SerialBT.print(path);
  SerialBT.print(" ");
  SerialBT.print(contentType);
  SerialBT.print(" ");
  SerialBT.println(body);

  unsigned long startTime = millis();
  response = "";
  while ((millis() - startTime) < TIMEOUT_MS) {
    if (SerialBT.available()) {
      response = SerialBT.readStringUntil('\n');
      Serial.print("Response: ");
      Serial.println(response);
      return true;
    }
    delay(10);
  }
  
  Serial.println("PUT timed out");
  return false;
}

bool BluetoothManager::del(const char* path, String& response) {
  if (!isConnected()) {
    Serial.println("Cannot perform DELETE: Not connected to Bluetooth");
    return false;
  }

  Serial.print("DELETE ");
  Serial.println(path);
  SerialBT.print("DELETE ");
  SerialBT.println(path);

  unsigned long startTime = millis();
  response = "";
  while ((millis() - startTime) < TIMEOUT_MS) {
    if (SerialBT.available()) {
      response = SerialBT.readStringUntil('\n');
      Serial.print("Response: ");
      Serial.println(response);
      return true;
    }
    delay(10);
  }
  
  Serial.println("DELETE timed out");
  return false;
}

bool BluetoothManager::checkApiHealth() {
  String response;
  return get("/api/health", response);
}