#pragma once
#include <WebServer.h>
#include <ESP32Servo.h>

class AppInterface
{
private:
    WebServer& server;
    Servo& gateServo;
    bool isServoUp = true;
public: 
    AppInterface(WebServer& srv, Servo& servo);

    void handleServoUp();
    void handleServoDown();
    void handleServoStatus();
    void handleStatus();
    void handleConnectionCheck();
    void initHttpHandlers();
    void handleClients();
};