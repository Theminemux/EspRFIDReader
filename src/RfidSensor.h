#pragma once

// Pins
#define RST_PIN 19
#define SDA_PIN 16
#define SCK_PIN 17
#define MOSI_PIN 5
#define MISO_PIN 18

#include <MFRC522.h>

class RfidSensor
{
private:
    MFRC522 mfrc522;
public:
    RfidSensor();

    void begin();
    void triggerLoop();
    void printCardInfo();
    String GetCardData();
};