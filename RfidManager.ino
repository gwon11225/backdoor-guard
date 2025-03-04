#include <SPI.h>
#include <MFRC522.h>

MFRC522 rfid(SS_PIN, RST_PIN);
byte nuidPICC[4];
byte verifiedCardNuid[5][4] {
  {175, 120, 115, 55}
};

void rfidStart() {
  SPI.begin();
  rfid.PCD_Init();
}

bool isCorrectCard(byte nuid[4]) {
  for (int cardIndex = 0; cardIndex < 5; cardIndex++) {
    if (
      verifiedCardNuid[cardIndex][0] == nuid[0] ||
      verifiedCardNuid[cardIndex][1] == nuid[1] ||
      verifiedCardNuid[cardIndex][2] == nuid[2] ||
      verifiedCardNuid[cardIndex][3] == nuid[3]
    ) {
      return true;
    }
  }
  return false;
}

bool isValidRFID() {
  bool isContactCard = rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial();
  bool isCorrectCardNuid = false;
  if (isContactCard) {
    nuidPICC[0] = rfid.uid.uidByte[0];
    nuidPICC[1] = rfid.uid.uidByte[1];
    nuidPICC[2] = rfid.uid.uidByte[2];
    nuidPICC[3] = rfid.uid.uidByte[3];

    isCorrectCardNuid = isCorrectCard(nuidPICC);
  }

  return isCorrectCardNuid;
}
