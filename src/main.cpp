#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "AppInterface.h"
#include "RfidTagCollector.h"
#include "RfidSensor.h"

// Servo-Pin (anpassen, falls du einen anderen nutzt)
#define SERVO_PIN 4

const String ssid = "rescuerobotcar";
const String password = "mint2025";
const String deviceName = "rescuecar-esp32";
const String carName = "rescuecar";
const String orangepiIp = "http://192.168.10.10";

String carIp = "";
String lastCardData = ""; 

WebServer server(80);
Servo gateServo;
AppInterface api(server, gateServo);
RfidObjectCollector tagCollector;
RfidSensor sensor;

bool SendJsonPost(const String& targetName, const String& url, const String& json)
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("[HTTP] WiFi not connected, skip send to ");
    Serial.println(targetName);
    return false;
  }

  for (int attempt = 1; attempt <= 2; attempt++) {
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

    if (!http.begin(url)) {
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

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println("[SETUP] Booting ESP32...");

  // login to wifi
  Serial.print("[WIFI] Connecting to SSID: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("[WIFI] Connected. IP address: ");
  Serial.println(WiFi.localIP());

  // Servo initialisieren
  gateServo.setPeriodHertz(50);       // Standard-Servo: 50 Hz
  gateServo.attach(SERVO_PIN);        // Servo-Pin
  gateServo.write(0);                 // Startposition: zu

  api.initHttpHandlers();
  sensor.begin();
  Serial.println("[SETUP] HTTP server started");

  // Log in to orangepi
  HTTPClient http;
  String registerUrl = orangepiIp + "/api/register/?device=" + deviceName;
  Serial.print("[HTTP] Register request: ");
  Serial.println(registerUrl);
  http.begin(registerUrl);
  int httpCode = http.GET();

  Serial.print("[HTTP] Response code (register): ");
  Serial.println(httpCode);
  if (httpCode <= 0) {
    Serial.print("[HTTP] Register transport error: ");
    Serial.println(http.errorToString(httpCode));
  } else {
    Serial.print("[HTTP] Register response body: ");
    Serial.println(http.getString());
  }

  if (httpCode <= 0) {
    Serial.println("[HTTP] Registration failed (transport), restarting...");
    http.end();
    ESP.restart();
  }
  if (httpCode != HTTP_CODE_OK) {
    Serial.println("[HTTP] Registration failed (status != 200), restarting...");
    http.end();
    ESP.restart();
  }
  http.end();

  // Ask orangepi for car ip address
  String carIpUrl = orangepiIp + "/api/getip/?device=" + carName;
  Serial.print("[HTTP] Request car IP from: ");
  Serial.println(carIpUrl);
  http.begin(carIpUrl);
  httpCode = http.GET();

  Serial.print("[HTTP] Response code (car IP): ");
  Serial.println(httpCode);
  if (httpCode <= 0) {
    Serial.print("[HTTP] Car IP transport error: ");
    Serial.println(http.errorToString(httpCode));
  }

  if (httpCode <= 0) {
    Serial.println("[HTTP] Failed to get car IP from orangepi, restarting...");
    http.end();
    ESP.restart();
  }
  if (httpCode != HTTP_CODE_OK) {
    Serial.println("[HTTP] Failed to get car IP from orangepi. body empty, restarting...");
    http.end();
    ESP.restart();
  }
  String carResponse = http.getString();
  Serial.print("[HTTP] Car response: ");
  Serial.println(carResponse);

  carResponse.trim();
  carIp = carResponse;

  Serial.print("[SETUP] Car IP: ");
  Serial.println(carIp);
  http.end();
  // After successful registration / connection to OrangePi, move servo up
  gateServo.write(0);

  Serial.println("[SETUP] Setup complete, waiting for RFID cards...");
}

void loop() {
  api.handleClients();
  sensor.triggerLoop();
}
