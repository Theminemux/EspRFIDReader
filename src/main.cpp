#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include <WebServer.h>

#include <ESP32Servo.h>

// RFID
#define RST_PIN 19
#define SDA_PIN 16
#define SCK_PIN 17
#define MOSI_PIN 5
#define MISO_PIN 18

//ESP to ESP communication
#define ESP_TURN_LEFT 0
#define ESP_TURN_RIGHT 2 
#define ESP_COLLECT 22
#define ESP_BYPASS 15

// Servo-Pin (anpassen, falls du einen anderen nutzt)
#define SERVO_PIN 4

const String ssid = "rescuerobotcar";
const String password = "mint2025";
const String deviceName = "rescuecar-esp32";
const String carName = "rescuecar";
String orangepiIp = "192.168.10.10";

constexpr unsigned long kRegisterRetryMs = 5000;
constexpr unsigned long kCarIpRetryMs = 5000;
constexpr unsigned long kCollectPulseMs = 3000;

String carIp = "";
String lastCardData = ""; 
String rfidObjectToCollect = "";

struct PositionState {
  int x;
  int y;
  int lv;
  bool valid;
};

struct CollectedObject {
  String objectName;
  PositionState position;
};

constexpr size_t kMaxCollectedObjects = 32;
constexpr int kDriveStatusStopped = 0;
constexpr int kDriveStatusDriving = 1;
constexpr int kDriveStatusPaused = 2;
constexpr int kDriveStatusFinished = 3;

PositionState currentPosition = {0, 0, 0, false};
CollectedObject collectedObjects[kMaxCollectedObjects];
size_t collectedObjectCount = 0;
int driveStatus = kDriveStatusStopped;

struct TurnEntry {
  int8_t x;
  int8_t y;
  int8_t lv;
  int8_t exitCode;
};

constexpr TurnEntry kTurnSequence[] = {
  {0, 6, 0, 1},
  {2, 6, 0, 0},
  {7, 6, 0, -1},
  {7, 3, 0, -1},
  {2, 3, 0, -1},
  {2, 6, 0, 1},
  {0, 6, 0, 1},
  {0, 1, 0, 1},
  {8, 1, 0, 1},
  {8, 3, 0, 0},
  {8, 4, 0, 2},
  {7, 3, 0, 0},
  {8, 3, 0, -1},
  {2, 3, 0, -1},
  {2, 6, 0, 0},
  {2, 9, 0, -1},
  {9, 9, 0, -1},
  {9, 3, 1, -1},
  {1, 3, 1, -1},
  {1, 5, 1, 2},
};

constexpr size_t kTurnSequenceLength = sizeof(kTurnSequence) / sizeof(kTurnSequence[0]);
constexpr unsigned long kTurnPulseMs = 150;

MFRC522 mfrc522(SDA_PIN, RST_PIN);
WebServer server(80);
Servo gateServo;
// Servo state: true = up, false = down
bool servoIsUp = false;
bool registrationStarted = false;
bool registrationDone = false;
bool carIpLookupDone = false;
unsigned long lastRegisterAttempt = 0;
unsigned long lastCarIpAttempt = 0;
bool collectSequenceActive = false;
unsigned long collectSequenceStartedAt = 0;
size_t currentTurnIndex = 0;

struct PulseState {
  uint8_t pin;
  bool active;
  unsigned long startedAt;
};

PulseState turnPulses[] = {
  {ESP_TURN_LEFT, false, 0},
  {ESP_TURN_RIGHT, false, 0},
  {ESP_BYPASS, false, 0},
};

void setServoUp();
void setServoDown();
bool httpGetOnce(const String &url, String &response, const char *label);

void setDriveStatus(int newStatus) {
  driveStatus = newStatus;
  Serial.print("[STATUS] Drive status set to ");
  Serial.println(driveStatus);
}

int getDriveStatus() {
  return driveStatus;
}

void resetDriveProgress() {
  currentPosition = {0, 0, 0, false};
  collectedObjectCount = 0;
  currentTurnIndex = 0;
  collectSequenceActive = false;
  collectSequenceStartedAt = 0;
  lastCardData = "";

  for (PulseState &pulse : turnPulses) {
    pulse.active = false;
    digitalWrite(pulse.pin, LOW);
  }

  digitalWrite(ESP_COLLECT, LOW);
  setServoUp();

  Serial.println("[STATUS] Drive progress reset");
}

bool sendCarControlRequest(const String &path, String &responseBody) {
  if (carIp.length() == 0) {
    Serial.println("[HTTP] carIp is empty, skip car control request");
    responseBody = "";
    return false;
  }

  String base = carIp;
  if (!base.startsWith("http://") && !base.startsWith("https://")) {
    base = "http://" + base;
  }

  String url = base + path;
  return httpGetOnce(url, responseBody, "Car control");
}

void updateCurrentPosition(int x, int y, int lv) {
  currentPosition.x = x;
  currentPosition.y = y;
  currentPosition.lv = lv;
  currentPosition.valid = true;
  Serial.print("[STATUS] Current position updated to x=");
  Serial.print(x);
  Serial.print(" y=");
  Serial.print(y);
  Serial.print(" lv=");
  Serial.println(lv);
}

bool recordCollectedObject(const String &objectName) {
  if (!currentPosition.valid) {
    Serial.print("[STATUS] Skip collected object without known position: ");
    Serial.println(objectName);
    return false;
  }

  if (collectedObjectCount >= kMaxCollectedObjects) {
    Serial.println("[STATUS] Collected object buffer full, dropping entry");
    return false;
  }

  collectedObjects[collectedObjectCount].objectName = objectName;
  collectedObjects[collectedObjectCount].position = currentPosition;
  collectedObjectCount++;

  Serial.print("[STATUS] Stored collected object: ");
  Serial.println(objectName);
  return true;
}

String buildStatusJson() {
  auto escapeJson = [](const String &value) -> String {
    String escaped;
    escaped.reserve(value.length() + 8);
    for (size_t i = 0; i < value.length(); ++i) {
      const char c = value.charAt(i);
      if (c == '\\' || c == '"') {
        escaped += '\\';
      }
      escaped += c;
    }
    return escaped;
  };

  const PositionState position = currentPosition.valid ? currentPosition : PositionState{0, 0, 0, true};

  String json;
  json.reserve(256 + collectedObjectCount * 80);
  json += "{\"position\":{\"x\":";
  json += position.x;
  json += ",\"y\":";
  json += position.y;
  json += ",\"lv\":";
  json += position.lv;
  json += "},\"status\":";
  json += driveStatus;
  json += ",\"collect_object\":\"";
  json += escapeJson(rfidObjectToCollect);
  json += "\",\"collected_objects\":[";

  for (size_t i = 0; i < collectedObjectCount; ++i) {
    if (i > 0) {
      json += ",";
    }
    json += "{\"object_name\":\"";
    json += escapeJson(collectedObjects[i].objectName);
    json += "\",\"position\":{\"x\":";
    json += collectedObjects[i].position.x;
    json += ",\"y\":";
    json += collectedObjects[i].position.y;
    json += ",\"lv\":";
    json += collectedObjects[i].position.lv;
    json += "}}";
  }

  json += "]}";
  return json;
}

void triggerPinPulse(uint8_t pin) {
  for (PulseState &pulse : turnPulses) {
    if (pulse.pin == pin) {
      pulse.active = true;
      pulse.startedAt = millis();
      digitalWrite(pin, HIGH);
      return;
    }
  }
}

void updatePinPulses() {
  for (PulseState &pulse : turnPulses) {
    if (!pulse.active) {
      continue;
    }

    if (millis() - pulse.startedAt >= kTurnPulseMs) {
      digitalWrite(pulse.pin, LOW);
      pulse.active = false;
    }
  }
}

void triggerTurnAction(int exitCode) {
  if (exitCode == 0) {
    Serial.println("[TURN] exit=0, no GPIO signal");
    return;
  }

  if (exitCode == 1) {
    Serial.println("[TURN] exit=1, pulsing ESP_TURN_RIGHT");
    triggerPinPulse(ESP_TURN_RIGHT);
    return;
  }

  if (exitCode == -1) {
    Serial.println("[TURN] exit=-1, pulsing ESP_TURN_LEFT");
    triggerPinPulse(ESP_TURN_LEFT);
    return;
  }

  if (exitCode == 2) {
    Serial.println("[TURN] exit=2, pulsing ESP_BYPASS");
    triggerPinPulse(ESP_BYPASS);
    return;
  }

  Serial.print("[TURN] Unknown exit code: ");
  Serial.println(exitCode);
}

bool handleTurnSequenceMatch(int x, int y, int lv) {
  if (currentTurnIndex >= kTurnSequenceLength) {
    return false;
  }

  const TurnEntry &expected = kTurnSequence[currentTurnIndex];
  if (expected.x != x || expected.y != y || expected.lv != lv) {
    Serial.print("[TURN] Waiting for next sequence entry at index ");
    Serial.println(currentTurnIndex);
    return false;
  }

  Serial.print("[TURN] Matched sequence index ");
  Serial.println(currentTurnIndex);
  triggerTurnAction(expected.exitCode);
  currentTurnIndex++;

  if (currentTurnIndex >= kTurnSequenceLength) {
    Serial.println("[TURN] Sequence complete");
  }

  return true;
}

bool httpGetOnce(const String &url, String &response, const char *label) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("[HTTP] WiFi not connected, skip ");
    Serial.println(label);
    response = "";
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  http.setConnectTimeout(3000);
  http.setTimeout(1500);
  http.setReuse(false);

  Serial.print("[HTTP] ");
  Serial.print(label);
  Serial.print(" -> ");
  Serial.println(url);

  if (!http.begin(client, url)) {
    Serial.print("[HTTP] begin() failed for ");
    Serial.println(label);
    response = "";
    return false;
  }

  const int httpCode = http.GET();
  Serial.print("[HTTP] ");
  Serial.print(label);
  Serial.print(" response code: ");
  Serial.println(httpCode);

  if (httpCode == HTTP_CODE_OK) {
    response = http.getString();
    http.end();
    return true;
  }

  if (httpCode <= 0) {
    Serial.print("[HTTP] ");
    Serial.print(label);
    Serial.print(" transport error: ");
    Serial.println(http.errorToString(httpCode));
  } else {
    Serial.print("[HTTP] ");
    Serial.print(label);
    Serial.println(" returned non-200 status");
  }

  response = "";
  http.end();
  return false;
}

void triggerCollectSequence() {
  if (collectSequenceActive) {
    Serial.println("[COLLECT] Sequence already active, skip retrigger");
    return;
  }

  collectSequenceActive = true;
  collectSequenceStartedAt = millis();
  digitalWrite(ESP_COLLECT, HIGH);
  setServoDown();
  Serial.println("[COLLECT] Signal high and servo down");
}

void updateCollectSequence() {
  if (!collectSequenceActive) {
    return;
  }

  if (millis() - collectSequenceStartedAt < kCollectPulseMs) {
    return;
  }

  digitalWrite(ESP_COLLECT, LOW);
  setServoUp();
  collectSequenceActive = false;
  Serial.println("[COLLECT] Signal low and servo up");
}

bool shouldScanRfid() {
  return driveStatus == kDriveStatusDriving;
}

// Module includes
#include "http_helper.h"
#include "servo_control.h"
#include "rfid_reader.h"
#include "server_routes.h"

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println("[SETUP] Booting ESP32...");
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SDA_PIN);
  mfrc522.PCD_Init();
  Serial.println("[SETUP] MFRC522 initialized");

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
  Serial.print("[WIFI] Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("[WIFI] Subnet: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("[WIFI] RSSI: ");
  Serial.println(WiFi.RSSI());
  WiFi.setSleep(false);
  delay(3000);

  // Servo initialisieren
  gateServo.setPeriodHertz(50);       // Standard-Servo: 50 Hz
  gateServo.attach(SERVO_PIN);        // Servo-Pin
  gateServo.write(0);                 // Startposition: zu
  pinMode(ESP_COLLECT, OUTPUT);
  digitalWrite(ESP_COLLECT, LOW);
  pinMode(ESP_TURN_LEFT, OUTPUT);
  pinMode(ESP_TURN_RIGHT, OUTPUT);
  pinMode(ESP_BYPASS, OUTPUT);
  digitalWrite(ESP_TURN_LEFT, LOW);
  digitalWrite(ESP_TURN_RIGHT, LOW);
  digitalWrite(ESP_BYPASS, LOW);

  registerRoutes(server);
  server.begin();
  Serial.println("[SETUP] HTTP server started");

  Serial.print("[HTTP] Orangepi IP: ");
  Serial.println(orangepiIp);
  lastRegisterAttempt = millis() - kRegisterRetryMs;
  lastCarIpAttempt = millis() - kCarIpRetryMs;

  Serial.println("[SETUP] Setup complete, waiting for RFID cards...");
}

void attemptOrangePiStartup() {
  if (!registrationDone && millis() - lastRegisterAttempt >= kRegisterRetryMs) {
    lastRegisterAttempt = millis();
    String registerUrl = "http://" + orangepiIp + "/api/register/?device=" + deviceName;
    String registerResponse = "";
    if (httpGetOnce(registerUrl, registerResponse, "Register")) {
      registrationDone = true;
      Serial.print("[HTTP] Register response body: ");
      Serial.println(registerResponse);
    }
  }

  if (registrationDone && !carIpLookupDone && millis() - lastCarIpAttempt >= kCarIpRetryMs) {
    lastCarIpAttempt = millis();
    String carIpUrl = "http://" + orangepiIp + "/api/getip/?device=" + carName;
    String carResponse = "";
    if (httpGetOnce(carIpUrl, carResponse, "Car IP")) {
      carResponse.trim();
      carIp = carResponse;
      carIpLookupDone = carIp.length() > 0;
      Serial.print("[HTTP] Car response: ");
      Serial.println(carResponse);
      Serial.print("[SETUP] Car IP: ");
      Serial.println(carIp);
    }
  }
}

void loop() {
  attemptOrangePiStartup();
  server.handleClient();
  updatePinPulses();
  updateCollectSequence();
  if (!shouldScanRfid()) {
    return;
  }
  // Read RFID cards continuously
  const bool cardPresent = mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial();
  if (cardPresent) {
    printCardInfo();
    String cardData = GetCardData();
    if (cardData.length() > 0) {
      Serial.print("Card Data: ");
      Serial.println(cardData); 
      if (cardData != lastCardData) {
        NewCardDetected(cardData);
        lastCardData = cardData;
      } else {
        Serial.println("Same card detected again.");
      }
    } else {
      Serial.println("No data read from card.");
    }
  } else if (lastCardData.length() > 0) {
    lastCardData = "";
  }
}