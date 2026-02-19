#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

// Pins
#define RST_PIN 19
#define SDA_PIN 16
#define SCK_PIN 17
#define MOSI_PIN 5
#define MISO_PIN 18

String lastCardData = ""; 

MFRC522 mfrc522(SDA_PIN, RST_PIN);

/*void printHex(const uint8_t *buffer, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (buffer[i] < 0x10);
    Serial.print(buffer[i], HEX);
    if (i + 1 < len);
  }
}*/

void printCardInfo() {
  //Serial.println("--- Tag detected ---");
  //Serial.print("UID: ");
  //printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
  //Serial.println();
  //Serial.print("UID size: "); Serial.println(mfrc522.uid.size);
  //Serial.print("SAK: 0x"); Serial.println(mfrc522.uid.sak, HEX);
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  //Serial.print("PICC Type: "); Serial.println(mfrc522.PICC_GetTypeName(piccType));
}

String GetCardData(){
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

void NewCardDetected(String cardData){
  Serial.println("{\"rfid_reader\":\"new_card\",\"data\":\"" + cardData + "\"}");
  // JSON looks like this: {"rfid_reader":"new_card","data":"CardDataHere"}
}

void setup() {
  Serial.begin(9600);
  delay(100);
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SDA_PIN);
  mfrc522.PCD_Init();
  //Serial.println("MFRC522 initialized");
}

void loop() {
  // Read RFID cards continuously
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    printCardInfo();
    String cardData = GetCardData();
    if (cardData.length() > 0) {
      //Serial.print("Card Data: ");
      //Serial.println(cardData);
      if (cardData != lastCardData) {
        //Serial.println("New card detected with different data!");

        NewCardDetected(cardData);

        lastCardData = cardData;
      } else {
        //Serial.println("Same card detected again.");
      }
    } else {
      //Serial.println("No data read from card.");
    }
    delay(100); // Wait 100 milliseconds before next read
  }
}