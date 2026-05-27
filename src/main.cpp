#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <MFRC522.h>
#include <WebServer.h>
#include "AppInterface.h"
#include "RfidTagCollector.h"
#include "RfidSensor.h"
#include "ConnectionManager.h"
#include "MagnetServo.h"

const String ssid = "rescuerobotcar";
const String password = "mint2025";

// Initialize hardware and software components to avoid copying them in the loop
MFRC522 mfrc522(SDA_PIN, RST_PIN);
ConnectionManager connectionManager;
WebServer server(80);
Servo gateServo;
RfidObjectCollector tagCollector;

MagnetServo magnetServo(gateServo);
AppInterface api(server, magnetServo);
RfidSensor sensor(mfrc522, connectionManager);

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

  connectionManager.AddConnection("esp32", WiFi.localIP().toString());

  api.initHttpHandlers();
  Serial.println("[SETUP] HTTP server started");
  sensor.begin();
  Serial.println("[SETUP] RFID sensor initialized");

  Serial.println("[SETUP] Setup complete, waiting for RFID cards or HTTP requests...");
}

void loop() {
  api.handleClients();
  sensor.triggerLoop();
}
