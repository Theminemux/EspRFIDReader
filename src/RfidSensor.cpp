#include "RfidSensor.h"

void RfidSensor::begin()
{
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SDA_PIN);
    mfrc522.PCD_Init();
}

void RfidSensor::triggerLoop()
{
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
      return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
      return;
  }

  String rawData = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
      rawData += String(mfrc522.uid.uidByte[i], HEX);
  }
  rawData.toUpperCase();

  String json;
  if (rawData.startsWith("OBJ")) {
    json = "{\"tag_type\":0,\"data\":\"" + rawData + "\"}";
  } else {
    json = "{\"tag_type\":1,\"data\":\"" + rawData + "\"}";
  }
  Serial.println("New card detected with data: " + rawData);

  String carIp = connectionManager.GetIp(carName);
  if (carIp.length() > 0) {
    String carUrl = "http://" + carIp + "/sensors/rfidupdate";
    httpRequests.SendJsonPost("car", carUrl, json);
  } else {
    Serial.println("[HTTP] carIp is empty, skip send to car");
  }
  httpRequests.SendJsonPost("orangepi", orangepiIp + "/api/rfidscan", json);
}

void RfidSensor::printCardInfo() {
  Serial.println("--- Tag detected ---");
  Serial.print("UID: ");
  Serial.println();
  Serial.print("UID size: "); Serial.println(mfrc522.uid.size);
  Serial.print("SAK: 0x"); Serial.println(mfrc522.uid.sak, HEX);
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.print("PICC Type: "); Serial.println(mfrc522.PICC_GetTypeName(piccType));
}

String RfidSensor::GetCardData(){
  String data = "";
  // Read blocks 4-6 (sector 1 data blocks) and convert to UTF-8 string
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
    
    // Convert to String, trim trailing zeros
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