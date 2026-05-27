#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include <WebServer.h>

extern Servo gateServo;
extern bool servoIsUp;

void setServoUp();
void setServoDown();
void handleServoUp();
void handleServoDown();
void handleServoStatus();
