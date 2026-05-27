#pragma once
#include <WebServer.h>
#include <ESP32Servo.h>
#include "MagnetServo.h"

class AppInterface
{
private:
    WebServer& server;
    MagnetServo& gateServo;
public: 
    AppInterface(WebServer& server, MagnetServo& gateServo) : server(server), gateServo(gateServo) {};

    void handleServoUp();
    void handleServoDown();
    void handleServoStatus();
    void handleStatus();
    void handleConnectionCheck();
    void initHttpHandlers();
    void handleClients();
};