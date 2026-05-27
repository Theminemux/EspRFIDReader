#include "rfid_reader.h"
#include "http_helper.h"
#include "servo_control.h"
#include <Arduino.h>

namespace {
bool parsePositionTag(const String &cardData, int &x, int &y, int &lv) {
  String tag = cardData;
  tag.trim();

  if (tag.length() == 0 || tag.startsWith("OBJ")) {
    return false;
  }

  lv = 0;
  int offset = 0;
  if (tag.charAt(0) == '1') {
    lv = 1;
    offset = 1;
  }

  if (tag.length() <= offset + 1) {
    return false;
  }

  const char column = toupper(tag.charAt(offset));
  if (column < 'A' || column > 'J') {
    return false;
  }

  const String rowText = tag.substring(offset + 1);
  for (size_t i = 0; i < rowText.length(); ++i) {
    if (!isDigit(rowText.charAt(i))) {
      return false;
    }
  }

  const int row = rowText.toInt();
  if (row <= 0) {
    return false;
  }

  x = column - 'A';
  y = row + 1;
  return true;
}

bool parseObjectTag(const String &cardData, String &objectName) {
  String tag = cardData;
  tag.trim();

  if (!tag.startsWith("OBJ")) {
    return false;
  }

  if (tag.length() != 5) {
    return false;
  }

  const char group = toupper(tag.charAt(3));
  const char slot = tag.charAt(4);

  if (group != 'G' && group != 'R' && group != 'B') {
    return false;
  }

  if (slot < '1' || slot > '4') {
    return false;
  }

  objectName = tag;
  return true;
}
} // namespace

void printCardInfo() {
  Serial.println("--- Tag detected ---");
  Serial.print("UID: ");
  Serial.println();
  Serial.print("UID size: "); Serial.println(mfrc522.uid.size);
  Serial.print("SAK: 0x"); Serial.println(mfrc522.uid.sak, HEX);
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.print("PICC Type: "); Serial.println(mfrc522.PICC_GetTypeName(piccType));
}

String GetCardData(){
  String data = "";
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  byte trailerBlock = 7;
  
  if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid)) == MFRC522::STATUS_OK) {
    const int maxBlocks = 3;
    uint8_t allbuf[16 * maxBlocks];
    memset(allbuf, 0, sizeof(allbuf));
    int pos = 0;
    
    for (int b = 0; b < maxBlocks; ++b) {
      byte blockAddr = 4 + b;
      byte buffer[18];
      byte size = sizeof(buffer);
      MFRC522::StatusCode status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
      
      if (status == MFRC522::STATUS_OK) {
        for (int i = 0; i < 16; ++i) {
          allbuf[pos++] = buffer[i];
        }
      }
    }
    
    int len = pos;
    while (len > 0 && allbuf[len-1] == 0) len--;
    
    if (len > 0) {
      data = String((const char*)allbuf).substring(0, len);
    }
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  return data;
}

void NewCardDetected(String cardData)
{
  String json = "{\"rfid_reader\":\"new_card\",\"data\":\"" + cardData + "\"}";

  Serial.println("New card detected with data: " + cardData);

  if (rfidObjectToCollect.length() > 0 && cardData == rfidObjectToCollect) {
    Serial.println("[RFID] Matched collect object, triggering local action");
    triggerCollectSequence();
  }

  if (getDriveStatus() != 1) {
    Serial.println("[RFID] Drive not active, skip route progression");
  }

  int x = 0;
  int y = 0;
  int lv = 0;
  if (parsePositionTag(cardData, x, y, lv)) {
    Serial.print("[RFID] Parsed position x=");
    Serial.print(x);
    Serial.print(" y=");
    Serial.print(y);
    Serial.print(" lv=");
    Serial.println(lv);
    updateCurrentPosition(x, y, lv);
    handleTurnSequenceMatch(x, y, lv);
  }

  String objectName = "";
  if (parseObjectTag(cardData, objectName)) {
    Serial.print("[RFID] Parsed object tag: ");
    Serial.println(objectName);
    recordCollectedObject(objectName);
  }

  if (orangepiIp.length() > 0) {
    String orangepiBase = orangepiIp;
    if (!orangepiBase.startsWith("http://") && !orangepiBase.startsWith("https://")) {
      orangepiBase = "http://" + orangepiBase;
    }
    String orangepiUrl = orangepiBase + "/api/rfidscan";
    SendJsonPost("orangepi", orangepiUrl, json);
  } else {
    Serial.println("[HTTP] orangepiIp is empty, skip send to orangepi");
  }
}
