#include "servo_control.h"
#include <Arduino.h>

void setServoUp()
{
  Serial.println("Action: setServoUp()");
  gateServo.writeMicroseconds(0); // Tor auf
  servoIsUp = true;
}

void setServoDown()
{
  Serial.println("Action: setServoDown()");
  gateServo.write(2600); // Tor zu
  servoIsUp = false;
}

void handleServoUp()
{
  Serial.println("HTTP: servo_up called");
  setServoUp();
  // server is passed by registerRoutes; handler uses global server via WebServer::send
  // We just send response using WebServer::client APIs via a simple global call in main.cpp
  // But WebServer expects a function with no args; it uses the global instance to send.
  // We'll rely on the global `server` instance and call server.send from the declaration in main.cpp.
  extern WebServer server;
  server.send(200, "text/plain", "servo_up");
}

void handleServoDown()
{
  Serial.println("HTTP: servo_down called");
  setServoDown();
  extern WebServer server;
  server.send(200, "text/plain", "servo_down");
}

void handleServoStatus()
{
  Serial.println("HTTP: servo_status called");
  String resp = servoIsUp ? "1" : "0";
  extern WebServer server;
  server.send(200, "text/plain", resp);
}
