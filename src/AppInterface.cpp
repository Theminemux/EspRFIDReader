#include <Arduino.h>
#include "AppInterface.h"

void AppInterface::initHttpHandlers()
{
  server.on("/servo/servo_up", HTTP_GET, std::bind(&AppInterface::handleServoUp, this));
  server.on("/servo/servo_down", HTTP_GET, std::bind(&AppInterface::handleServoDown, this));
  server.on("/servo/status", HTTP_GET, std::bind(&AppInterface::handleServoStatus, this));
  server.on("/status", HTTP_GET, std::bind(&AppInterface::handleStatus, this));
  server.on("/api/checkconnection", HTTP_GET, std::bind(&AppInterface::handleConnectionCheck, this));
  server.begin();
}

void AppInterface::handleClients()
{
  server.handleClient();
}

void AppInterface::handleServoUp()
{
  Serial.println("HTTP: servo_up called");
  gateServo.MoveUp();
  server.send(200, "text/plain", "servo_up");
}

void AppInterface::handleServoDown()
{
  Serial.println("HTTP: servo_down called");
  gateServo.MoveDown();
  server.send(200, "text/plain", "servo_down");
}

void AppInterface::handleServoStatus()
{
  Serial.println("HTTP: servo_status called");
  String status = gateServo.IsUp() ? "1" : "0";
  server.send(200, "text/plain", status);
}

void AppInterface::handleStatus()
{
  Serial.println("HTTP: servo_status called");
  
  server.send(200, "text/plain");
}

void AppInterface::handleConnectionCheck() 
{
  Serial.println("HTTP: Connection check called");
  server.send(200);
}