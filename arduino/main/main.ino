#include <MFRC522.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <RtcDS1302.h>
#include <time.h>

#define RX_PIN 6
#define TX_PIN 7

#define SS_PIN 10
#define RST_PIN 9

#define SOLENOID 8
#define CLOSE_BUTTON 3

JsonDocument doc;
MFRC522 mfrc(SS_PIN, RST_PIN);
ThreeWire myWire(4, 5, 2);
RtcDS1302<ThreeWire> rtc(myWire);
SoftwareSerial raspberry(RX_PIN, TX_PIN);

String res = "";
byte accessTimeCount = 0;

typedef struct {
  byte dow;
  const char* startTime;
  const char* endTime;
} AccessTime;

byte accessCardArray[100][4] = {
  { 156, 115, 244, 47 }
};

AccessTime accessDateTime[20] = {
  { 4, "09:50:00", "19:10:00" }
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  raspberry.begin(9600);
  SPI.begin();
  mfrc.PCD_Init();
  rtc.Begin();

  RtcDateTime complied = RtcDateTime(__DATE__, __TIME__);
  rtc.SetDateTime(complied);

  pinMode(SOLENOID, OUTPUT);
  pinMode(CLOSE_BUTTON, INPUT);

  digitalWrite(SOLENOID, HIGH);
}

void loop() {
  key_update();
  
  if (digitalRead(CLOSE_BUTTON) == HIGH) {
    digitalWrite(SOLENOID, HIGH);
  }
  
  if ( !mfrc.PICC_IsNewCardPresent() || !mfrc.PICC_ReadCardSerial() ) {
    delay(1000);
    return;
  }

  byte cardkey[4] = { 
    mfrc.uid.uidByte[0], 
    mfrc.uid.uidByte[1], 
    mfrc.uid.uidByte[2], 
    mfrc.uid.uidByte[3] 
  };
  Serial.println("card");
  if (isAccessCard(cardkey) && isAccessTime()) {
    // open the door logic
    Serial.println("open the door");
    digitalWrite(SOLENOID, LOW);
    delay(1000);
  }
  
  delay(100);
}

void key_update() {
  if (raspberry.available()) {
    while (raspberry.available() > 0) {
      res += char(raspberry.read());
    }

    if (res.startsWith("\r")) {
      for (int i = 0; i < 20; i++) {
        accessDateTime[i] = {};
      }
      Serial.println("access time count is reset");
      accessTimeCount = 0;
    }

    if (res.endsWith("\n")) {
      Serial.println(res);

      int first = res.indexOf(",");
      int second = res.indexOf(",", first + 1);
      int length = res.length();

      byte dow = byte(res.substring(0, first).toInt());
      char startTime[9]; char endTime[9];
      res.substring(first + 1, second).toCharArray(startTime, 9);
      res.substring(second + 1, length).toCharArray(startTime, 9);

      accessDateTime[accessTimeCount] = { dow, endTime, startTime };

      accessTimeCount ++;

      Serial.println(dow);
      
      res = "";
    }
  }
}

bool isAccessCard(byte uid[]) {
  for (int i = 0; i < 100; i++) {
    if (
      accessCardArray[i][0] == uid[0] &&
      accessCardArray[i][1] == uid[1] &&
      accessCardArray[i][2] == uid[2] &&
      accessCardArray[i][3] == uid[3]
    ) {
      return true;
    }
    return false;
  }
}

bool isAccessTime() {
  for (int i = 0; i < 20; i++) {
    if (isInTimeRange(accessDateTime[i].startTime, accessDateTime[i].endTime, accessDateTime[i].dow)) {
      return true;
    }
  }
  return false;
}

bool isInTimeRange(const char* start, const char* endtime, char week) {
  RtcDateTime now = rtc.GetDateTime();

  int startHour, startMin, startSec;
  int endHour, endMin, endSec;
  sscanf(start, "%d:%d:%d", &startHour, &startMin, &startSec);
  sscanf(endtime, "%d:%d:%d", &endHour, &endMin, &endSec);

  int currentTotalSec = now.Hour() * 3600 + now.Minute() * 60 + now.Second();
  int startTotalSec = startHour * 3600 + startMin * 60 + startSec;
  int endTotalSec = endHour * 3600 + endMin * 60 + endSec;

  if (
    currentTotalSec >= startTotalSec &&
    currentTotalSec <= endTotalSec &&
    now.DayOfWeek() == week
  ) {
    return true;
  }
  return false;
}