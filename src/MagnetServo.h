#pragma once

#define SERVO_PIN 4

#include <Arduino.h>
#include <ESP32Servo.h>

class MagnetServo 
{
private:
    Servo servo;
    bool isUp = true;
public:
    MagnetServo(Servo servo) : servo(servo) {}

    void begin();
    void MoveUp();
    void MoveDown();
    bool IsUp();
};