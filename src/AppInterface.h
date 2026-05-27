#pragma once
#include <WebServer.h>
#include <ESP32Servo.h>

class AppInterface
{
private:
    WebServer& server;
    Servo& gateServo;
public: 
    AppInterface(WebServer& srv, Servo& servo);

    void handleServoUp();
    void handleServoDown();
    void handleStatus();
    void handleConnectionCheck();
    void initHttpHandlers();
    void handleClients();
};