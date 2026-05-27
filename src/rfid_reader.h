#pragma once
#include <Arduino.h>
#include <MFRC522.h>

extern MFRC522 mfrc522;
extern String carIp;
extern String orangepiIp;
extern String lastCardData;
extern String rfidObjectToCollect;

void setDriveStatus(int newStatus);
int getDriveStatus();
void resetDriveProgress();
bool sendCarControlRequest(const String &path, String &responseBody);
void updateCurrentPosition(int x, int y, int lv);
bool recordCollectedObject(const String &objectName);
String buildStatusJson();

void printCardInfo();
String GetCardData();
void NewCardDetected(String cardData);
void triggerCollectSequence();
bool handleTurnSequenceMatch(int x, int y, int lv);
