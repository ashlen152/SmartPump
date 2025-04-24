#include "WiFiManager.h"

WiFiManager::WiFiManager(const char *ssid, const char *password)
{
  _ssid = ssid;
  _password = password;

  httpClient = nullptr;
}

WiFiManager::~WiFiManager()
{
  disconnect();
}

bool WiFiManager::connect()
{
  WiFi.begin(_ssid, _password);

  DisplayManager::getInstance().showText("Connecting to WiFi...");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < MAX_ATTEMPTS)
  {
    delay(500);
    // Create a string with a number of dots equal to attempts % 4 (cycle 0-3)

    String dots = ".";
    for (int i = 0; i < (attempts % 4) + 1; i++)
    {
      dots += ".";
    }

    std::vector<String> lines = {"Connecting", dots};
    DisplayManager::getInstance().showText(lines);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    if (httpClient == nullptr)
    {
      httpClient = new HttpClient(wifiClient, _serverAddress.c_str(), _port);
      if (httpClient == nullptr)
      {
        Serial.println("Failed to create HttpClient");
        return false;
      }
      httpClient->setTimeout(HTTP_TIMEOUT);
    }

    int rssi = getSignalStrength();
    String signalIndicator = "Signal: " + String(rssi) + " dBm";
    String signalStatus = (rssi < MIN_RSSI) ? "Weak Signal" : "Good Signal";
    std::vector<String> lines = {"", "Connected!", "IP: " + String(WiFi.localIP()), signalIndicator, signalStatus};
    DisplayManager::getInstance().showText(lines);
    sleep(2);
    return true;
  }
  else
  {
    DisplayManager::getInstance().showText("Failed to connect to WiFi");
    sleep(1);
    return false;
  }
}

bool WiFiManager::isConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::disconnect()
{
  WiFi.disconnect();
  if (httpClient != nullptr)
  {
    delete httpClient;
    httpClient = nullptr;
  }
  DisplayManager::getInstance().showText("Disconnected from WiFi");
  sleep(1);
}

int WiFiManager::getSignalStrength()
{
  if (isConnected())
  {
    int rssi = WiFi.RSSI();
    Serial.print("Signal Strength: ");
    Serial.println(rssi);
    return rssi;
  }
  else
  {
    Serial.println("Not connected to WiFi");
    return -1;
  }
}

// GET request
bool WiFiManager::get(const char *path, String &response)
{
  if (!isConnected())
  {
    Serial.println("Cannot perform GET: Not connected to WiFi");
    return false;
  }

  Serial.print("GET ");
  Serial.println(path);

  unsigned long startTime = millis(); // Start the timeout timer

  httpClient->beginRequest();
  httpClient->get(path);
  httpClient->endRequest();

  // Wait for the response with a timeout
  while (!httpClient->available() && (millis() - startTime) < HTTP_TIMEOUT)
  {
    delay(10); // Small delay to avoid busy-waiting
  }

  if ((millis() - startTime) >= HTTP_TIMEOUT)
  {
    Serial.println("GET request timed out");
    return false;
  }

  int httpCode = httpClient->responseStatusCode();
  response = httpClient->responseBody();

  if (httpCode > 0 && httpCode < 400)
  {
    Serial.print("Response Code: ");
    Serial.println(httpCode);
    Serial.print("Response Body: ");
    Serial.println(response);
    return true; // Add this return to stop further execution
  }
  else
  {
    Serial.print("GET failed with code: ");
    Serial.println(httpCode);
    return false;
  }
}

// POST request
bool WiFiManager::post(const char *path, const char *contentType, const char *body, String &response)
{
  if (!isConnected())
  {
    Serial.println("Cannot perform POST: Not connected to WiFi");
    return false;
  }

  Serial.print("POST ");
  Serial.println(path);

  unsigned long startTime = millis(); // Start the timeout timer

  httpClient->beginRequest();
  httpClient->post(path, contentType, body);
  httpClient->endRequest();

  // Wait for the response with a timeout
  while (!httpClient->available() && (millis() - startTime) < HTTP_TIMEOUT)
  {
    delay(10); // Small delay to avoid busy-waiting
  }

  if ((millis() - startTime) >= HTTP_TIMEOUT)
  {
    Serial.println("POST request timed out");
    return false;
  }

  int httpCode = httpClient->responseStatusCode();
  response = httpClient->responseBody();

  if (httpCode > 0 && httpCode < 400) // Success codes (2xx and 3xx)
  {
    Serial.print("HTTP Code: ");
    Serial.println(httpCode);
    Serial.print("Response: ");
    Serial.println(response);
    return true;
  }
  else
  {
    Serial.print("POST failed with code: ");
    Serial.println(httpCode);
    return false;
  }
}

// PUT request
bool WiFiManager::put(const char *path, const char *contentType, const char *body, String &response)
{
  if (!isConnected())
  {
    Serial.println("Cannot perform PUT: Not connected to WiFi");
    return false;
  }

  Serial.print("PUT ");
  Serial.println(path);

  unsigned long startTime = millis(); // Start the timeout timer

  httpClient->beginRequest();
  httpClient->put(path, contentType, body);
  httpClient->endRequest();

  // Wait for the response with a timeout
  while (!httpClient->available() && (millis() - startTime) < HTTP_TIMEOUT)
  {
    delay(10); // Small delay to avoid busy-waiting
  }

  if ((millis() - startTime) >= HTTP_TIMEOUT)
  {
    Serial.println("PUT request timed out");
    return false;
  }

  int httpCode = httpClient->responseStatusCode();
  response = httpClient->responseBody();

  if (httpCode > 0 && httpCode < 400) // Success codes (2xx and 3xx)
  {
    Serial.print("HTTP Code: ");
    Serial.println(httpCode);
    Serial.print("Response: ");
    Serial.println(response);
    return true;
  }
  else
  {
    Serial.print("PUT failed with code: ");
    Serial.println(httpCode);
    return false;
  }
}

// DELETE request
bool WiFiManager::del(const char *path, String &response)
{
  if (!isConnected())
  {
    Serial.println("Cannot perform DELETE: Not connected to WiFi");
    return false;
  }

  Serial.print("DELETE ");
  Serial.println(path);

  unsigned long startTime = millis(); // Start the timeout timer

  httpClient->beginRequest();
  httpClient->del(path);
  httpClient->endRequest();

  // Wait for the response with a timeout
  while (!httpClient->available() && (millis() - startTime) < HTTP_TIMEOUT)
  {
    delay(10); // Small delay to avoid busy-waiting
  }

  if ((millis() - startTime) >= HTTP_TIMEOUT)
  {
    Serial.println("DELETE request timed out");
    return false;
  }

  int httpCode = httpClient->responseStatusCode();
  response = httpClient->responseBody();

  if (httpCode > 0 && httpCode < 400) // Success codes (2xx and 3xx)
  {
    Serial.print("HTTP Code: ");
    Serial.println(httpCode);
    Serial.print("Response: ");
    Serial.println(response);
    return true;
  }
  else
  {
    Serial.print("DELETE failed with code: ");
    Serial.println(httpCode);
    return false;
  }
}

// Legacy health check method
bool WiFiManager::checkApiHealth()
{
  String response;
  return get("/api/health", response);
}