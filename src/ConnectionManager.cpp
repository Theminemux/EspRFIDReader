#include "ConnectionManager.h"
#include <HTTPClient.h>

void ConnectionManager::AddConnection(String name, String ip)
{
    connectedDevices[name] = ip;
}

String ConnectionManager::GetIp(String name)
{
    if (connectedDevices.find(name) != connectedDevices.end()) {
        return connectedDevices[name];
    }
    return "";
}

void ConnectionManager::LogInToOrangepi()
{
    HTTPClient http;
    String loginUrl = orangepiIp + "/api/login/?device=" + deviceName;
    Serial.print("[HTTP] Login request: ");
    Serial.println(loginUrl);
    http.begin(loginUrl);
    int httpCode = http.GET();

    Serial.print("[HTTP] Response code (login): ");
    Serial.println(httpCode);
    if (httpCode <= 0) {
        Serial.print("[HTTP] Login transport error: ");
        Serial.println(http.errorToString(httpCode));
        return;
    } 
    if (httpCode != HTTP_CODE_OK) {
        Serial.println("[HTTP] Login failed (status != 200)");
        http.end();
        return;
    }
    String response = http.getString();
    Serial.print("[HTTP] Login response: ");
    Serial.println(response);
    http.end();
}

void ConnectionManager::TryGetCarIp()
{
  HTTPClient http;
  // Ask orangepi for car ip address
  String carIpUrl = orangepiIp + "/api/getip/?device=" + carName;
  Serial.print("[HTTP] Request car IP from: ");
  Serial.println(carIpUrl);
  http.begin(carIpUrl);
  int httpCode = http.GET();

  Serial.print("[HTTP] Response code (car IP): ");
  Serial.println(httpCode);
  if (httpCode <= 0) {
    Serial.print("[HTTP] Car IP transport error: ");
    Serial.println(http.errorToString(httpCode));
    return;
  }
  if (httpCode != HTTP_CODE_OK) {
    Serial.println("[HTTP] Failed to get car IP from orangepi. body empty, restarting...");
    http.end();
    return;
  }
  String carResponse = http.getString();
  http.end();
  Serial.print("[HTTP] Car response: ");
  Serial.println(carResponse);

  connectedDevices[carName] = carResponse;
}