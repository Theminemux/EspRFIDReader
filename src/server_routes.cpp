#include "server_routes.h"
#include "servo_control.h"
#include "rfid_reader.h"
#include <Arduino.h>

void handleConnectionCheck() 
{
  Serial.println("HTTP: Connection check called");
  extern WebServer server;
  server.send(200);
}

void handleStatus()
{
  extern WebServer server;
  server.send(200, "application/json", buildStatusJson());
}

void handleStart()
{
  extern WebServer server;

  resetDriveProgress();
  String responseBody = "";
  if (!sendCarControlRequest("/api/start", responseBody)) {
    setDriveStatus(0);
    server.send(502, "text/plain", "start_failed");
    return;
  }

  setDriveStatus(1);
  server.send(200, "text/plain", "start");
}

void handleStop()
{
  extern WebServer server;

  resetDriveProgress();
  setDriveStatus(0);

  String responseBody = "";
  if (!sendCarControlRequest("/api/stopp", responseBody)) {
    server.send(502, "text/plain", "stopp_failed");
    return;
  }

  server.send(200, "text/plain", "stopp");
}

void handlePause()
{
  extern WebServer server;

  setDriveStatus(2);
  lastCardData = "";

  String responseBody = "";
  if (!sendCarControlRequest("/api/stopp", responseBody)) {
    server.send(502, "text/plain", "pause_failed");
    return;
  }

  server.send(200, "text/plain", "pause");
}

void handleResume()
{
  extern WebServer server;

  if (getDriveStatus() != 2) {
    server.send(409, "text/plain", "not_paused");
    return;
  }

  String responseBody = "";
  if (!sendCarControlRequest("/api/start", responseBody)) {
    server.send(502, "text/plain", "resume_failed");
    return;
  }

  lastCardData = "";
  setDriveStatus(1);
  server.send(200, "text/plain", "resume");
}

void handleNewJson()
{
  extern WebServer server;

  String value = "";
  if (server.args() > 0) {
    value = server.arg(0);
    if (value.length() == 0) {
      String firstArgName = server.argName(0);
      if (firstArgName.length() > 0) {
        value = firstArgName;
      }
    }
  }

  if (value.length() == 0 && server.hasArg("string")) {
    value = server.arg("string");
  }

  if (value.length() == 0 && server.hasArg("value")) {
    value = server.arg("value");
  }

  value.trim();
  rfidObjectToCollect = value;

  Serial.print("HTTP: newjson stored collect object: ");
  Serial.println(rfidObjectToCollect);

  server.send(200, "text/plain", rfidObjectToCollect);
}

void registerRoutes(WebServer &server)
{
  server.on("/servo/servo_up", HTTP_GET, handleServoUp);
  server.on("/servo/servo_down", HTTP_GET, handleServoDown);
  server.on("/servo/status", HTTP_GET, handleServoStatus);
  server.on("/api/checkconnection", HTTP_GET, handleConnectionCheck);
  server.on("/api/newjson", HTTP_GET, handleNewJson);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/start", HTTP_GET, handleStart);
  server.on("/api/stopp", HTTP_GET, handleStop);
  server.on("/api/pause", HTTP_GET, handlePause);
  server.on("/api/resume", HTTP_GET, handleResume);
}
