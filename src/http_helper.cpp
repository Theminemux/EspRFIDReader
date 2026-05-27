#include "http_helper.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>

bool SendJsonPost(const String& targetName, const String& url, const String& json)
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("[HTTP] WiFi not connected, skip send to ");
    Serial.println(targetName);
    return false;
  }

  for (int attempt = 1; attempt <= 2; attempt++) {
    WiFiClient client;
    HTTPClient http;
    http.setConnectTimeout(3000);
    http.setTimeout(5000);
    http.setReuse(false);

    Serial.print("[HTTP] POST ");
    Serial.print(targetName);
    Serial.print(" attempt ");
    Serial.print(attempt);
    Serial.print(" -> ");
    Serial.println(url);

    if (!http.begin(client, url)) {
      Serial.print("[HTTP] begin() failed for ");
      Serial.println(targetName);
      return false;
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Connection", "close");

    int httpCode = http.POST(json);
    if (httpCode == HTTP_CODE_OK) {
      Serial.print("Data sent to ");
      Serial.print(targetName);
      Serial.println(" successfully");
      http.end();
      return true;
    }

    if (httpCode <= 0) {
      Serial.print("Failed to send data to ");
      Serial.print(targetName);
      Serial.print(", HTTP code: ");
      Serial.println(httpCode);
      Serial.print("[HTTP] Transport error: ");
      Serial.println(http.errorToString(httpCode));
    } else {
      Serial.print("Failed to send data to ");
      Serial.print(targetName);
      Serial.print(", HTTP status: ");
      Serial.println(httpCode);
      String responseBody = http.getString();
      if (responseBody.length() > 0) {
        Serial.print("[HTTP] Response body: ");
        Serial.println(responseBody);
      }
    }

    http.end();
    delay(150);
  }

  return false;
}
